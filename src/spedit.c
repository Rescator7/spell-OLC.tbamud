#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "spells.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "interpreter.h"
#include "modify.h"
#include "screen.h"
#include "spedit.h"

void cleanup_olc (struct descriptor_data *d, byte cleanup_type);
extern const char *apply_types[];
extern const char *affected_bits[];
extern const char *pc_class_types[];
extern const char *spell_flags[];
extern const char *position_types[];
extern struct descriptor_data *descriptor_list;
int is_abbrev (const char *arg1, const char *arg2);
int formula_interpreter (struct char_data *self, struct char_data *vict, int spell_vnum,
                         int syserr, char *cmd, int *rts_code);
struct str_spells *list_spells = NULL;
int last_serial = 0;

char *UNDEF_SPELL = "Undefined";

char *get_spell_name (int vnum) 
{
 struct str_spells *ptr = NULL;

 for (ptr = list_spells; ptr; ptr = ptr->next)
   if (ptr->serial == vnum)
     return (ptr->name);
 return (UNDEF_SPELL);
}

/* will convert a string to upper case... but, not stuff in quote " " */
void strxupr (char *str) {
  int i, quote = 0;

  for (i=0; i<strlen(str); i++)
    if (str[i] == '\"') 
      quote ^= 1;
    else
      if (!quote)
        str[i] = toupper(str[i]); 
}

char *IS_SPELL_OLCING (int serial) 
{
 struct descriptor_data *q;

 for (q = descriptor_list; q; q = q->next)
   if ((q->connected == CON_SPEDIT) && 
       OLC_NUM(q) == serial) 
     return (GET_NAME(q->character));
 return NULL;
}

int find_spell_by_serial (struct descriptor_data *d, int serial)
{
  struct str_spells *ptr;

  for (ptr = list_spells; ptr; ptr = ptr->next)
    if (ptr->serial == serial) {
      free(OLC_STORAGE(d));
      OLC_STORAGE(d) = strdup(ptr->name);
      return serial;
    }
  return 0;
}

int find_spell_by_name (struct descriptor_data *d, char *name) 
{
  struct str_spells *ptr;

  for (ptr = list_spells; ptr; ptr = ptr->next)
    if (is_abbrev(name, ptr->name)) {
      free(OLC_STORAGE(d));
      OLC_STORAGE(d) = strdup(ptr->name); // we nedds the full name, not just an abbreviation
      return ptr->serial;
    }
  return 0;
}

char *spedit_listflags (int flags) {
  char buf[2048];

  int i;

  if (flags == 0)
    strcpy (buf, "NONE");
  else {
    buf[0] = '\0';
    for (i=0; i<NUM_SPELL_FLAGS; i++)
      if (flags & (1 << i))
        sprintf (buf, "%s%s ", buf, spell_flags [i]);
  }
  return (strdup (buf));
}

void spedit_assign_menu (struct descriptor_data *d) {
  char buf[2048];

  int i;  
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  strcpy (buf, "\r\n");
  for (i=0; i<NUM_CLASSES; i++)
    sprintf (buf, "%s%s%d%s) Name    : %s%s %s(%s%3d%s) \r\n   %sPrac    : %s%s\r\n   "
                  "%sMana    : %s%s\r\n",
                  buf, 
                  grn, i + 1, nrm, yel, 
                  Q->assign[i].class_num != -1 ? 
                     pc_class_types [Q->assign[i].class_num] : "<N/A>", 
                  nrm, cyn, Q->assign[i].level, nrm,
                  nrm, cyn, Q->assign[i].num_prac ? Q->assign[i].num_prac : "<N/A>",
                  nrm, cyn, Q->assign[i].num_mana ? Q->assign[i].num_mana : "<N/A>");
  sprintf (buf, "%s\r\n%sEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_ASSIGN_MENU;
}

void spedit_apply_menu (struct descriptor_data *d) {
  char buf[2048];
  char buf1[2048];

  int i;
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  strcpy (buf, "\r\n");
  for (i=0; i<6; i++) {
    buf1[0] = '\0';
    if (Q->applies[i].appl_num < NUM_APPLIES)
      sprintf (buf1, "   %sModifier : %s%s\r\n", nrm, cyn, Q->applies[i].modifier ?
                     Q->applies[i].modifier : "<N/A>");
    sprintf (buf, "%s%s%d%s) Name     : %s%s\r\n%s   %sDuration : %s%s\r\n", 
                buf, grn, i + 1, nrm, yel,
                Q->applies[i].appl_num != -1 ? 
                Q->applies[i].appl_num >= NUM_APPLIES ? affected_bits [Q->applies[i].appl_num
                    - NUM_APPLIES] : apply_types [Q->applies[i].appl_num] : "<N/A>", 
                buf1,
                nrm, cyn, Q->applies[i].duration ? Q->applies[i].duration : "<N/A>");
  }
  sprintf (buf, "%s\r\n%sEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_APPLY_MENU;
}

void spedit_immune_menu (struct descriptor_data *d) {
  char buf[2048];

  int i;
  char *name;
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  strcpy (buf, "\r\n");
  for (i=0; i<6; i++) {
    name = get_spell_name (Q->protfrom[i].prot_num);
    sprintf (buf, "%s%s%d%s) Name     : %s%s %s(%s%d%s)\r\n   %sDuration : %s%s\r\n%s   Resist %% : %s%s\r\n", 
                buf, grn, i + 1, nrm, yel, name,
                nrm, cyn, Q->protfrom[i].prot_num, nrm,
                nrm, cyn, Q->protfrom[i].duration ? Q->protfrom[i].duration : "<N/A>",
                nrm, cyn, Q->protfrom[i].resist   ? Q->protfrom[i].resist : "<N/A>");
  }
  sprintf (buf, "%s\r\n%sEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_IMMUNE_MENU;
}

void spedit_minpos_menu (struct descriptor_data *d) {
  char buf[2048];

  int i;

  sprintf (buf, "%s\r\n-- POSITION :    \r\n", nrm);
  for (i=0; i < NUM_CHAR_POSITION; i++)
    sprintf (buf, "%s%s%2d%s) %s%-16s%s", buf, grn, i + 1, nrm, yel, 
                   position_types [i], (i + 1) % 4 ? "" : "\r\n" );
  sprintf (buf, "%s%s\r\n\r\nEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_GET_MINPOS;
}

void spedit_flags_menu (struct descriptor_data *d) {
  char buf[2048];

  int i;
  char *spellflags;

  spellflags = spedit_listflags (OLC_SPELL(d)->flags);
  sprintf (buf, "%s\r\n-- FLAGS :     %s%s\r\n", nrm, cyn, spellflags);
  for (i=0; i < NUM_SPELL_FLAGS; i++)
    sprintf (buf, "%s%s%2d%s) %s%-15s%s", buf, grn, i + 1, nrm, yel,
                   spell_flags [i], (i + 1) % 4 ? "" : "\r\n" );
  sprintf (buf, "%s%s\r\nEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  free (spellflags);
  OLC_MODE(d) = SPEDIT_SHOW_SPELL_FLAGS;
}

void spedit_choose_apply (struct descriptor_data *d) {
  char buf[2048];

  int i, cpt;

  sprintf (buf, "%s\r\n-- APPLIES : \r\n", nrm);
  for (i=0; i < NUM_APPLIES; i++)
    sprintf (buf, "%s%s%2d%s) %s%-15s%s", buf, grn, i + 1, nrm, yel, 
                   apply_types [i], (i + 1) % 4 ? "" : "\r\n" );
  sprintf (buf, "%s\r\n\r\n%s-- AFFECTS : \r\n", buf, nrm);
  cpt = i; 
  for (i=cpt; i < cpt + NUM_AFF_FLAGS + NUM_CLASSES; i++)
    sprintf (buf, "%s%s%2d%s) %s%-15s%s", buf, grn, i + 1, nrm, yel, affected_bits [i - NUM_APPLIES],
                  (i + 1) % 4 ? "" : "\r\n");
  sprintf (buf, "%s%s\r\n\r\nEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_APPLY;
}

void spedit_assignement_menu (struct descriptor_data *d) {
  char buf[2048];

  int i;
  
  sprintf (buf, "%s\r\n-- CLASSES : \r\n", nrm);
  for (i=0; i < NUM_CLASSES; i++)
    sprintf (buf, "%s%s%2d%s) %s%-13s%s", buf, grn, i + 1, nrm, yel, pc_class_types [i],
                  (i + 1) % 5 ? "" : "\r\n");  
  sprintf (buf, "%s%s\r\n\r\nEnter choice (0 to quit, -1 to remove) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_ASSIGNEMENT;
}

void spedit_main_menu (struct descriptor_data *d) {
  char buf[2048];

  char *spellflags;
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  spellflags = spedit_listflags (Q->flags);

  get_char_colors (d->character);

  bool prog = Q->function != NULL;

#if defined (CLEAR_SCREEN)
  send_to_char (d->character, "[H[J");
#endif         

  sprintf (buf, "%s-- Serial number     : [%s%5d%s]    %sT%s)ype : [%s%-6s%s]    "
                "%sSpec_Prog : %s%s\r\n"
                "%sS%s) Status            : %s%s\r\n"  
                "%s1%s) Name              : %s%s\r\n"
                "%s2%s) Min position      : %s%s\r\n"
                "%s3%s) Spell FLAGS       : %s%s\r\n"
                "%s4%s) Damages           : %s%s %s(%s%4d%s)\r\n"
                "%s5%s) Pulse delay       : %s%s\r\n"
                "%s6%s) Effectiveness %%   : %s%s\r\n"
                "%s7%s) Menu -> Protection from\r\n"
                "%s8%s) Menu -> Applies\r\n"
                "%s9%s) Menu -> Script\r\n" 
                "%sA%s) Menu -> Assignement\r\n"
                "%sQ%s) Quit\r\n\r\n"
                "%sEnter choice : ",
                 nrm, cyn, OLC_NUM(d), nrm, prog ? red : grn, nrm, cyn, (Q->type == SPELL) ? "SPELL" : "SKILL", nrm,
                 nrm, yel, Q->function ? "Yes" : "No", 
                 grn, nrm, yel, (Q->status == available) ? "Available" : "Unavailable",
                 prog ? red : grn, nrm, yel, Q->name ? Q->name : "Undefined", 
                 prog ? red : grn, nrm, cyn, ((Q->min_pos >= 0) && (Q->min_pos < NUM_CHAR_POSITION)) ? 
                              position_types [Q->min_pos] : "<BUGGED>",   
                 prog ? red : grn, nrm, cyn, spellflags,
                 prog ? red : grn, nrm, cyn, Q->damages ? Q->damages : "<N/A>", nrm, cyn, Q->max_dam, nrm,  
                 prog ? red : grn, nrm, cyn, Q->delay ? Q->delay : "<N/A>",
                 prog ? red : grn, nrm, cyn, Q->effectiveness ? Q->effectiveness : "<N/A>",
                 prog ? red : grn, nrm, 
                 prog ? red : grn, nrm,  
                 prog ? red : grn, nrm,
                 prog ? red : grn, nrm,
                 grn, nrm,
                 nrm);
  send_to_char (d->character, "%s", buf);
  free (spellflags);
  OLC_MODE (d) = SPEDIT_MAIN_MENU;
}

void spedit_free_spell (struct str_spells *spell) {
  int i;

  if (!spell)
    return;

  if (spell->name)          free (spell->name);
  if (spell->damages)       free (spell->damages);
  if (spell->effectiveness) free (spell->effectiveness);
  if (spell->delay)         free (spell->delay);
  if (spell->script)        free (spell->script);
  for (i=0; i<NUM_CLASSES; i++) {
    if (spell->protfrom[i].duration) 
      free (spell->protfrom[i].duration);
    if (spell->protfrom[i].resist)
      free (spell->protfrom[i].resist);
    if (spell->applies[i].modifier)
      free (spell->applies[i].modifier);
    if (spell->applies[i].duration)
      free (spell->applies[i].duration);
    if (spell->assign[i].num_prac)
      free (spell->assign[i].num_prac);
    if (spell->assign[i].num_mana)
      free (spell->assign[i].num_mana);
  }
  free (spell);
}

void spedit_free_memory() {
  struct str_spells *q = list_spells, *n;

  while (q) {
    n = q->next;
    spedit_free_spell(q);
    q = n;
  }
}

void spedit_save_internally (struct str_spells *spell) 
{
 struct str_spells *i, *p = NULL;

 for (i = list_spells; i; p = i, i = i->next)
   if (i->serial >= spell->serial)
     break;

 spell->saved = TRUE;

 if (i && (i->serial == spell->serial)) {
   i->status = spell->status;
   return;
 }

 if (p)
   p->next = spell;
 else
   list_spells = spell;
 
  spell->next = i;
}

void spedit_init_new_spell (struct str_spells *spell)
{
 int i;

 spell->saved    = FALSE;
 spell->next     = NULL;
 spell->status   = unavailable;
 spell->type     = 'P';
 spell->name     = strdup ("Undefined");
 spell->flags    = 0;
 spell->min_pos  = 0;
 spell->max_dam  = 0;
 spell->effectiveness = NULL;
 spell->damages  = NULL;
 spell->delay    = NULL;
 spell->script   = NULL;
 spell->wear_off = NULL;
 for (i=0; i<6; i++) {
   spell->protfrom[i].prot_num = -1;
   spell->protfrom[i].duration = NULL;
   spell->protfrom[i].resist   = NULL;
   spell->applies[i].appl_num  = -1;
   spell->applies[i].modifier  = NULL;
   spell->applies[i].duration  = NULL;
 }

 for (i=0; i<NUM_CLASSES; i++) {
   spell->assign[i].class_num  = -1;
   spell->assign[i].level      = 0;
   spell->assign[i].num_prac   = NULL;
   spell->assign[i].num_mana   = NULL;
 }
 spell->function               = NULL;
}

void spedit_create_spell (struct descriptor_data *d)
{
 struct str_spells *q = NULL;

 int serial = 0;

 for (q = list_spells; q; q = q->next) {
   serial = q->serial;

   if (q->serial && (q->serial == OLC_NUM(d)))
     break;
 }

 if (!OLC_NUM(d))
   while (IS_SPELL_OLCING(++serial));

   OLC_NUM(d) = serial;

 if (q) 
   OLC_SPELL(d) = q;
 else {
   CREATE (OLC_SPELL(d), struct str_spells, 1);
   OLC_SPELL(d)->serial = OLC_NUM(d);
   spedit_init_new_spell (OLC_SPELL(d));
 }
}

void boot_spells (void)
{
 char buf[MAX_STRING_LENGTH + 1] = "";
 char buf1[MAX_STRING_LENGTH + 1] = "";

 FILE *fp;
 int  fct, d1, err = 0, save = 0;
 int  ret;
 struct str_spells *Q = NULL;

 if ((fp=fopen(SPELL_FILE, "r")) == NULL) {
    mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT: Can't boot spells.");
    return;
 }

 while (!feof(fp)) {
    ret = fscanf (fp, "%d ", &fct);
    if (!save && (fct != 1)) {
      mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: attemp to assign value to empty Q");
      abort();
    }
    switch (fct) {
      case 1 : if (save == 1) 
                 spedit_save_internally (Q); 
               else  
                 save = 1;
               CREATE (Q, struct str_spells, 1);
               spedit_init_new_spell (Q);
               ret = fscanf (fp, "%c %d %d %d %d %d\n", &Q->type, &Q->serial, &Q->status,
                             &Q->flags, &Q->min_pos, &Q->max_dam);
               break;
      case 2 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0'; 
                 Q->name = strdup (buf);
               }
               break;
      case 4 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0'; 
                 Q->damages = strdup (buf);
               }
               break;
      case 35 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0'; 
                  Q->effectiveness = strdup (buf);
                }
                break; 
      case 36 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0'; 
                  Q->delay = strdup (buf);
                }
                break;
      case 37 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  if (!Q->script)
                    Q->script = strdup (buf); 
                  else {
                    sprintf (buf1, "%s%s", Q->script, buf); 
                    free (Q->script);
                    Q->script = strdup (buf1);
                  } 
                }   
                break;
      case 99 : break;    
      default : if ((fct > 16) && (fct < 23)) {
                  if (!fgets (buf, MAX_STRING_LENGTH, fp)) {
                    mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: protection from read error!");
                  }
                  sscanf (buf, "%d %s\r\n", &Q->protfrom [fct - 17].prot_num, buf1);
                  if (*buf1) Q->protfrom [fct - 17].duration = strdup(buf1);
                  if (!fgets (buf, MAX_STRING_LENGTH, fp)) {
                    mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: protection from read error!");
                  }
                  sscanf (buf, "%d %s\r\n", &d1, buf1);
                  if (*buf1) Q->protfrom [fct - 17].resist = strdup(buf1); 
                } else
                if ((fct > 22) && (fct < 29)) {
                  ret = fscanf (fp, "%d ", &Q->applies[fct - 23].appl_num);
                  if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                    buf[strlen(buf)-1] = '\0'; 
                    Q->applies[fct - 23].modifier = strdup (buf);
                  } 
                  ret = fscanf (fp, "%d ", &d1);
                  if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                    buf[strlen(buf)-1] = '\0';
                    Q->applies[fct - 23].duration = strdup (buf);
                  } 
                } else
                if ((fct > 28) && (fct < 33)) {
                  if (!fgets (buf, MAX_STRING_LENGTH, fp)) {
                    mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: assign read error!");
                  }
                  sscanf (buf, "%d %d %s\r\n", &Q->assign [fct - 29].class_num,
                               &Q->assign [fct - 29].level, buf1);
                  if (*buf1) Q->assign [fct - 29].num_prac = strdup (buf1);
                  ret = fscanf (fp, "%d ", &d1);
                  if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                    buf[strlen(buf)-1] = '\0'; 
                    Q->assign [fct - 29].num_mana = strdup (buf);
                  }
                } else 
                    if (err++ > 9) {
                      mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: program abort too much errors.");
                      abort();
                    } else
                       mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: invalide code in database.");
    } 
 }

 if (save == 1) 
   spedit_save_internally (Q); 
 else
   mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: No spells available!");
}

void spedit_save_to_disk (void)
{
 char buf[2048];
 char buf1[2048];

 FILE *fp;
 struct str_spells *r;
 int i;
 char *p;

 sprintf (buf, "cp %s %s.bak", SPELL_FILE, SPELL_FILE);
 if (system (buf) == -1) {
   mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: SPEDIT: Failed to create spell file backup.");
 }

 if ((fp=fopen(SPELL_FILE, "w")) == NULL) {
   mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: SPEDIT: Can't save spells to the database.");
   return;
 }
 for (r = list_spells; r; r = r->next) {
   sprintf (buf, "01 %c %d %d %d %d %d\n",
                  r->type, r->serial, r->status, r->flags, r->min_pos, r->max_dam);
   if (r->name)
     sprintf (buf, "%s02 %s\n", buf, r->name);

   /* code 03 is FREE */

   if (r->damages)
     sprintf (buf, "%s04 %s\n", buf, r->damages);

   /* 5 6 7 8 9 10 11 12 13 14 15 16 Are FREE*/

   /* 17 18 19 20 21 22 */
   for (i=0; i<6; i++)
     if (r->protfrom[i].prot_num != -1) 
       sprintf (buf, "%s%d %d %s\n00 %s\n", buf, i + 17,
                r->protfrom[i].prot_num, r->protfrom[i].duration, r->protfrom[i].resist);
   /* 23 24 25 26 27 28 */
   for (i=0; i<6; i++)
     if (r->applies[i].appl_num != -1)
       sprintf (buf, "%s%d %d %s\n00 %s\n", 
                buf, i + 23, r->applies[i].appl_num, 
                r->applies[i].modifier, r->applies[i].duration);
   /* 29 30 31 32  */
   for (i=0; i<NUM_CLASSES; i++)
     if (r->assign[i].class_num != -1)
       sprintf (buf, "%s%d %d %d %s\n00 %s\n", 
                buf, i + 29, r->assign[i].class_num,
                r->assign[i].level, r->assign[i].num_prac,
                r->assign[i].num_mana);
   /* 33 34 are free */

   /* 35 */
   if (r->effectiveness)
     sprintf (buf, "%s35 %s\n", buf, r->effectiveness);
   /* 36 */
   if (r->delay)
     sprintf (buf, "%s36 %s\n", buf, r->delay);
   /* 37 */
   if (r->script) {
     strcpy (buf1, r->script); 
     p = strtok (buf1, "\n");
     sprintf (buf, "%s37 %s\n", buf, p);
     while ((p=strtok('\0', "\n"))) 
       sprintf (buf, "%s37 %s\n", buf, p);
   }
   fprintf (fp, "%s", buf);
 }
 fprintf (fp, "99");
 fflush (fp);
 fclose (fp);
}

int spedit_setup2 (struct descriptor_data *d)
{
 /* Send the OLC message to the players in the same room as the builder. */
 act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
 SET_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);

 spedit_create_spell (d);
 spedit_main_menu (d);

 return 1;
}

int spedit_setup (struct descriptor_data *d)
{
 char buf[2048];

 char *str;
 int serial;
 
 if (OLC_STORAGE(d)) {
   if (is_number(OLC_STORAGE(d))) 
     serial = find_spell_by_serial(d, atoi(OLC_STORAGE(d))); 
   else
     serial = find_spell_by_name(d, OLC_STORAGE(d));

   if (!serial) {
     send_to_char (d->character, "that spell could not be found!\r\n");
     cleanup_olc (d, CLEANUP_ALL);
     return 0;
   } else {
       if ((str = IS_SPELL_OLCING (serial))) {
         sprintf (buf, "This spell '%s' (serial: %d) is already edited by %s.\r\n", OLC_STORAGE(d), serial, str);
         send_to_char (d->character, "%s", buf);  
         cleanup_olc (d, CLEANUP_ALL);
         return 0;
       }

       OLC_NUM(d) = serial;
       sprintf (buf, "Do you want to edit '%s' (serial: %d)? (y/n): ", OLC_STORAGE(d), serial);
       send_to_char (d->character, "%s", buf);  
       OLC_MODE(d) = SPEDIT_CONFIRM_EDIT;
       return 1;
     }
 }

 OLC_NUM(d) = 0;
 spedit_setup2(d);
 return 1;
}

void spedit_parse (struct descriptor_data *d, char *arg) {
  char buf[2048];

  int x = 0, value, rts_code = 0;

  switch (OLC_MODE(d)) {
    case SPEDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
          case 'y' :
          case 'Y' : if (!OLC_SPELL(d)->saved)
                       spedit_save_internally(OLC_SPELL(d));
                     spedit_save_to_disk();
                     sprintf (buf, "OLC: %s edits spells %d.", GET_NAME(d->character), OLC_NUM(d));
                     mudlog (CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "%s", buf);
                     OLC_SPELL(d) = NULL;
                     cleanup_olc (d, CLEANUP_ALL);
                     send_to_char (d->character, "Spell saved to disk.\r\n");
                     spedit_save_to_disk();
                     break;
          case 'n' :
          case 'N' : if (!OLC_SPELL(d)->saved) {
                       spedit_free_spell (OLC_SPELL(d));
                       OLC_SPELL(d) = NULL;
                     }
                     else
                       send_to_char (d->character, "Spell has been updated.\r\n");
                     cleanup_olc (d, CLEANUP_ALL);
                     break;
          default  : send_to_char (d->character, "Invalid choice!\r\nDo you want to save this spell to disk? : ");
                     break;
        }
        return;
    case SPEDIT_GET_TYPE : OLC_SPELL(d)->type = (atoi(arg) == 1) ? SKILL : SPELL;
                           break;  
    case SPEDIT_GET_NUMMANA : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE,
                                      arg, &rts_code);
                              if (!rts_code) {
                               if (OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana)
                                 free (OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana);
                               OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana =
                                  (arg && *arg) ? strdup (arg) : strdup("0");
                               spedit_assign_menu(d);
                             } else 
                                 send_to_char (d->character, "Num mana : ");
                             return;
    case SPEDIT_GET_NAME   : if (OLC_SPELL(d)->name) free (OLC_SPELL(d)->name);
                             OLC_SPELL(d)->name = 
                               (arg && *arg) ? strdup(arg) : strdup("Undefined");
                             break;
    case SPEDIT_GET_PROTDUR: value = formula_interpreter (d->character, 
                                     d->character, OLC_NUM(d), FALSE,
                                     arg, &rts_code);
                             if (!rts_code) { 
                               if (OLC_SPELL(d)->protfrom[OLC_VAL(d)].duration)
                                 free (OLC_SPELL(d)->protfrom[OLC_VAL(d)].duration);
                               OLC_SPELL(d)->protfrom[OLC_VAL(d)].duration = 
                                 (arg && *arg) ? strdup (arg) : strdup("0");
                               send_to_char (d->character, "Resist %% : ");
                               OLC_MODE(d) = SPEDIT_GET_RESIST;
                             } else 
                                 send_to_char (d->character, "Duration : ");
                             return;
    case SPEDIT_GET_SPELL_NUM : OLC_SPELL(d)->protfrom[OLC_VAL(d)].prot_num = atoi(arg);
                                send_to_char (d->character, "Duration : ");
                                OLC_MODE(d) = SPEDIT_GET_PROTDUR;
                                return; 
    case SPEDIT_GET_NUMPRAC: value = formula_interpreter (d->character, 
                                     d->character, OLC_NUM(d), FALSE,
                                     arg, &rts_code);
                             if (!rts_code) {  
                               if (OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac)
                                 free (OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac);
                                OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac = 
                                  (arg && *arg) ? strdup (arg) : strdup("0"); 
                                send_to_char (d->character, "Num mana : ");
                                OLC_MODE(d) = SPEDIT_GET_NUMMANA;
                             } else 
                                 send_to_char (d->character, "NUM Practice : ");
                             return;
    case SPEDIT_GET_LEVEL  : OLC_SPELL(d)->assign[OLC_VAL(d)].level = atoi(arg);
                             send_to_char (d->character, "Num Practice : ");
                             OLC_MODE (d) = SPEDIT_GET_NUMPRAC;
                             return; 
    case SPEDIT_GET_MODIF  : value = formula_interpreter (d->character,
                                     d->character, OLC_NUM(d), FALSE,
                                     arg, &rts_code);
                             if (!rts_code) {  
                               if (OLC_SPELL(d)->applies[OLC_VAL(d)].modifier) 
                                 free (OLC_SPELL(d)->applies[OLC_VAL(d)].modifier);
                               OLC_SPELL(d)->applies[OLC_VAL(d)].modifier = 
                                 (arg && *arg) ? strdup(arg) : strdup("0"); 
                               send_to_char (d->character, "Duration : ");
                               OLC_MODE (d) = SPEDIT_GET_APPLDUR;
                              } else 
                                  send_to_char (d->character, "Modifier : ");
                              return;
    case SPEDIT_GET_APPLDUR : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE, 
                                      arg, &rts_code);
                              if (!rts_code) {
                                if (OLC_SPELL(d)->applies[OLC_VAL(d)].duration)
                                  free (OLC_SPELL(d)->applies[OLC_VAL(d)].duration);
                                OLC_SPELL(d)->applies[OLC_VAL(d)].duration = 
                                  (arg && *arg) ? strdup (arg) : strdup("0");
                                spedit_choose_apply (d);
                              } else 
                                  send_to_char (d->character, "Duration : ");
                              return;
    case SPEDIT_GET_MAXDAM  : OLC_SPELL(d)->max_dam = atoi(arg);
                              break;
    case SPEDIT_GET_DAMAGES : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE,
                                      arg, &rts_code);
                              if (!rts_code) {
                                if (OLC_SPELL(d)->damages)
                                  free (OLC_SPELL(d)->damages);
                                OLC_SPELL(d)->damages = 
                                  (arg && *arg) ? strdup (arg) : NULL;
                                send_to_char (d->character, "Max damages : ");
                                OLC_MODE(d) = SPEDIT_GET_MAXDAM;
                              } else 
                                  send_to_char (d->character, "Damages : ");
                              return; 
    case SPEDIT_GET_EFFECTIVENESS : 
                              value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE,
                                      arg, &rts_code);
                              if (!rts_code) {
                                if (OLC_SPELL(d)->effectiveness)
                                  free (OLC_SPELL(d)->effectiveness);
                                OLC_SPELL(d)->effectiveness = 
                                  (arg && *arg) ? strdup (arg) : NULL;
                                break;
                              } else 
                                  send_to_char (d->character, "%% of effectiveness : ");
                              return;
    case SPEDIT_GET_RESIST  : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE,
                                      arg, &rts_code);
                              if (!rts_code) {
                                if (OLC_SPELL(d)->protfrom[OLC_VAL(d)].resist)
                                  free (OLC_SPELL(d)->protfrom[OLC_VAL(d)].resist);
                                OLC_SPELL(d)->protfrom[OLC_VAL(d)].resist = 
                                  (arg && *arg) ? strdup (arg) : strdup("0");
                                spedit_immune_menu(d);
                              } else  
                                  send_to_char (d->character, "Resist %% : ");
                              return;
    case SPEDIT_GET_DELAY   : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE,
                                      arg, &rts_code);
                              if (!rts_code) {
                                if (OLC_SPELL(d)->delay)
                                  free (OLC_SPELL(d)->delay);
                                OLC_SPELL(d)->delay = 
                                  (arg && *arg) ? strdup (arg) : NULL;
                                break;
                              } else 
                                  send_to_char (d->character, "Passes (10 passes = 1 sec) : ");
                              return;  
    case SPEDIT_GET_STATUS  : if ((x = atoi(arg)) == available)
                                OLC_SPELL(d)->status = x;
                              else
                                OLC_SPELL(d)->status = unavailable;
                              break;  
    case SPEDIT_ASSIGN_MENU :
           if (!(x = atoi(arg))) break;
           else 
             if ((x > 0) && (x < 5)) {
               OLC_VAL (d) = x - 1;
               spedit_assignement_menu (d);
             } else {
                 send_to_char (d->character, "Invalid choice!\r\n");
                 spedit_assign_menu (d);
               }
           return;
    case SPEDIT_APPLY_MENU : 
           if (!(x = atoi(arg))) break;
           else 
             if ((x > 0) && (x < 7)) {
               OLC_VAL(d) = x - 1;  
               spedit_choose_apply (d);
             } else {
                 send_to_char (d->character, "Invalid choice!\r\n");
                 spedit_apply_menu (d);
               } 
           return; 
    case SPEDIT_SHOW_APPLY : 
           if (!(x = atoi(arg))) 
             spedit_apply_menu (d);
           else
             if ( (x < 0) || (x > NUM_APPLIES + NUM_AFF_FLAGS + NUM_CLASSES) ) {
               send_to_char (d->character, "Invalid choice!\r\n");
               spedit_choose_apply (d);
             } else {
                  OLC_SPELL(d)->applies[OLC_VAL(d)].appl_num = x - 1;
                  if (x <= NUM_APPLIES) {
                    send_to_char (d->character, "Modifier : ");
                    OLC_MODE(d) = SPEDIT_GET_MODIF;
                  } else {
                      if (OLC_SPELL(d)->applies[OLC_VAL(d)].modifier)
                        free (OLC_SPELL(d)->applies[OLC_VAL(d)].modifier);
                      OLC_SPELL(d)->applies[OLC_VAL(d)].modifier = strdup("0"); 
                      send_to_char (d->character, "Duration : ");
                      OLC_MODE (d) = SPEDIT_GET_APPLDUR;
                    }
               }
           return; 
    case SPEDIT_SHOW_ASSIGNEMENT :
           if (!(x = atoi (arg))) 
             spedit_assign_menu (d);
           else if ( (x < -2) || (x > NUM_CLASSES) ) {
                  send_to_char (d->character, "Invalid choice!\r\n");
                  spedit_assignement_menu (d);
                }
                else 
                   if (x == -1) {
                     OLC_SPELL(d)->assign[OLC_VAL(d)].class_num = -1;
                     OLC_SPELL(d)->assign[OLC_VAL(d)].level = 0;
                     if (OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac)
                       free (OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac);
                     OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac = NULL;
                     if (OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana)
                       free (OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana);
                     OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana = NULL;
                     spedit_assign_menu(d);
                   } else {
                       OLC_SPELL(d)->assign[OLC_VAL(d)].class_num = x - 1;
                       send_to_char (d->character, "Level : ");
                       OLC_MODE(d) = SPEDIT_GET_LEVEL;
                     }
           return;
    case SPEDIT_GET_MINPOS :
         if (!(x = atoi(arg))) break;
         else
           if ( (x < 0) || (x > NUM_CHAR_POSITION) ) {
             send_to_char (d->character, "Invalid choice!\r\n");
             spedit_minpos_menu (d);
             return;
           }   
           else
             OLC_SPELL(d)->min_pos = x - 1;
         break;   
    case SPEDIT_SHOW_SPELL_FLAGS : 
         if (!(x = atoi (arg))) break;
         else 
            if ( (x < 0) || (x > NUM_SPELL_FLAGS) ) 
              send_to_char (d->character, "Invalid choice!\r\n");
            else
              OLC_SPELL(d)->flags ^= (1 << (x - 1));
         spedit_flags_menu (d);
         return;
    case SPEDIT_IMMUNE_MENU :
         if (!(x = atoi (arg))) break;
         else 
           if ( (x < 0) || (x > 6) ) {
             send_to_char (d->character, "Invalid choice!\r\n");
             spedit_immune_menu (d);
             return;
           }
         OLC_VAL(d) = x - 1;
         send_to_char (d->character, "Spell number : ");
         OLC_MODE(d) = SPEDIT_GET_SPELL_NUM;
         return;   
    case SPEDIT_CONFIRM_EDIT : 
         if ((*arg == 'y') || (*arg == 'Y')) { 
           send_to_char (d->character, "\r\n");
           spedit_setup2 (d);
         } else
             cleanup_olc (d, CLEANUP_ALL); 
         return; 
    case SPEDIT_MAIN_MENU :
        if (OLC_SPELL(d)->function && *arg != 'q' && *arg != 'Q' 
                                   && *arg != 's' && *arg != 'S') {
          send_to_char (d->character, "Invalid option!\r\n");
          break;
        }
        switch (*arg) {
          case 'q' :
          case 'Q' : if (OLC_VAL(d)) {
                       send_to_char (d->character, "Do you want to save this spell to disk? : ");
                       OLC_MODE(d) = SPEDIT_CONFIRM_SAVESTRING;
                     } else
                         cleanup_olc (d, CLEANUP_ALL); 
                     return;
          case 's' :
          case 'S' : if (GET_LEVEL(d->character) < LVL_IMPL) { 
                       send_to_char (d->character, "Only the implentors can set that!\r\n");
                       break;
                     } 
                     else {
                       send_to_char (d->character, "0-Unavailable, 1-Available\r\n\r\n"
                                                   "Enter choice : ");
                       OLC_MODE(d) = SPEDIT_GET_STATUS;
                       return; 
                     }
          case '1' : send_to_char (d->character, "Spell name : ");
                     OLC_MODE(d) = SPEDIT_GET_NAME;
                     return;
          case '2' : spedit_minpos_menu (d);
                     return;
          case '3' : spedit_flags_menu (d);
                     return;
          case '4' : send_to_char (d->character, "Damages : ");
                     OLC_MODE(d) = SPEDIT_GET_DAMAGES;
                     return; 
          case '5' : send_to_char (d->character, "Passes (10 passes = 1 sec) : ");
                     OLC_MODE(d) = SPEDIT_GET_DELAY;
                     return;
          case '6' : send_to_char (d->character, "%% of effectiveness : ");
                     OLC_MODE(d) = SPEDIT_GET_EFFECTIVENESS;
                     return;
          case '7' : spedit_immune_menu (d);
                     return; 
          case '8' : spedit_apply_menu (d);
                     return;
          case '9' : page_string (d, OLC_SPELL(d)->script, 1);
                     d->backstr  = OLC_SPELL(d)->script ?
                                   strdup(OLC_SPELL(d)->script) : NULL;
                     d->str      = &OLC_SPELL(d)->script;
                     d->max_str  = MAX_STRING_LENGTH;
                     d->mail_to  = 0;
                     OLC_VAL(d)  = 1;
                     return;
          case 'a' :
          case 'B' : spedit_assign_menu (d);
                     return;
          case 't' : 
          case 'T' : send_to_char (d->character, "\r\n0-Spell     1-Skill\r\n");
                     send_to_char (d->character, "Enter choice : ");
                     OLC_MODE(d) = SPEDIT_GET_TYPE;
                     return;
          default  : send_to_char (d->character, "Invalid option!\r\n"); 
        }
        break;
     default : return; 
  }
  OLC_VAL(d) = 1; 
  send_to_char (d->character, "\r\n");
  spedit_main_menu (d);
}

ACMD(do_spedit) {
  struct descriptor_data *d = ch->desc;

/* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

/* Give the descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_spedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  skip_spaces(&argument);
  if (*argument)
    OLC_STORAGE(d) = strdup(argument);
  else
    OLC_STORAGE(d) = NULL;
 
  STATE(d) = CON_SPEDIT;

  spedit_setup(ch->desc);
}

ACMD(do_splist) {
{
 char buf[MAX_STRING_LENGTH];
 int cpt = 0;
 size_t len = 0, tmp_len = 0;

 struct str_spells *ptr;
 
 len = snprintf(buf, sizeof(buf), 
 "Index Serial  Name                 Type     Spec_Prog    Available\r\n"
 "----- ------  ----                 ----     ---------    ---------\r\n");
 
 for (ptr = list_spells; ptr; ptr = ptr->next) {
   tmp_len = snprintf(buf+len, sizeof(buf)-len, "%s%4d%s) [%s%5d%s]%s %-20s%s %-5s    %s%-3s          %s%-3s\r\n",
             QGRN, ++cpt, QNRM, QGRN, ptr->serial, QNRM, 
             QCYN, ptr->name, QYEL, ptr->type == SPELL ? "SPELL" : "SKILL", 
             QNRM, ptr->function ? "Yes" : "No", 
             ptr->status == available ? QGRN : QRED, ptr->status == available ? "Yes" : "No"); 
   len += tmp_len;
   if (len > sizeof(buf))
     break;
  }

  page_string(ch->desc, buf, TRUE);
 }
}


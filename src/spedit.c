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

#define EMPTY_STR(str) ((str) ? (str) : "<empty>")
#define NULL_STR(str)  ((str) ? (strdup(str)) : NULL)

// There is DISPOSE(), but i don't needs the SYSERR:, and I don't NULL it either.
#define SAFE_FREE(str) if(str) free(str)

void cleanup_olc (struct descriptor_data *d, byte cleanup_type);
extern const char *apply_types[];
extern const char *affected_bits[];
extern const char *pc_class_types[];
extern const char *targ_flags[];
extern const char *mag_flags[];
extern const char *position_types[];
extern struct descriptor_data *descriptor_list;
int is_abbrev (const char *arg1, const char *arg2);
int formula_interpreter (struct char_data *self, struct char_data *vict, int spell_vnum,
                         int syserr, char *cmd, int *rts_code);
struct str_spells *list_spells = NULL;
int last_vnum = 0;

char *UNDEF_SPELL = "Undefined";

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

int IS_SPELL_CLASS(struct str_spells *spell, int class) {
  int i;

  for (i=0; i<NUM_CLASSES; i++)
    if (spell->assign[i].class_num == class)
      return 1;
  return 0;
}

char *IS_SPELL_OLCING (int vnum) 
{
 struct descriptor_data *q;

 for (q = descriptor_list; q; q = q->next)
   if ((q->connected == CON_SPEDIT) && 
       OLC_NUM(q) == vnum) 
     return (GET_NAME(q->character));
 return NULL;
}

int is_assign_set(struct str_spells *spell)
{
 int i;

 for (i=0; i<NUM_CLASSES; i++) 
   if (spell->assign[i].class_num != -1)
     return 1;
 return 0;
}
 
int is_prot_set(struct str_spells *spell)
{
 int i;

 for (i=0; i<NUM_CLASSES; i++) 
   if (spell->protfrom[i].prot_num != -1)
     return 1;
 return 0;
}

int is_apply_set(struct str_spells *spell)
{
 int i;

 for (i=0; i<NUM_CLASSES; i++) 
   if (spell->applies[i].appl_num != -1)
     return 1;
 return 0;
}

int is_summon_set(struct str_spells *spell)
{
}

int is_objects_set(struct str_spells *spell)
{
 int i;

 for (i=0; i<MAX_SPELL_OBJECTS; i++)
   if (spell->objects[i])
     return 1;
 return 0;
}

int is_dispel_set(struct str_spells *spell)
{
}

int is_points_set(struct str_spells *spell)
{
}

int is_messages_set(struct str_spells *spell)
{
  if (spell->messages.wear_off) return 1;
  if (spell->messages.to_self) return 1;
  if (spell->messages.to_vict) return 1;
  if (spell->messages.to_room) return 1;

  return 0;
}

int get_spell_apply(struct str_spells *spell, int pos)
{
 if ((pos < 0) || (pos > MAX_SPELL_AFFECTS - 1))
  return APPLY_NONE;

 if (spell->applies[pos].appl_num == -1)
   return APPLY_NONE;
 else
   return spell->applies[pos].appl_num; 
}

char *get_spell_name(int vnum)
{
 struct str_spells *ptr;

 for (ptr = list_spells; ptr; ptr = ptr->next)
   if (ptr->vnum == vnum)
     return ptr->name;
 return UNDEF_SPELL;
}

int find_spell_by_vnum (int vnum)
{
  struct str_spells *ptr;

  for (ptr = list_spells; ptr; ptr = ptr->next)
    if (ptr->vnum == vnum) 
      return vnum;
  return 0;
}

int find_skill_num (char *name) {
  struct str_spells *ptr;
  int first = -1; 
  int cpt = 0;

  for (ptr = list_spells; ptr; ptr = ptr->next) {
    // we search skill only
//    if (ptr->type != SKILL) continue;

    // exact match
    if (!str_cmp(name, ptr->name))
      return ptr->vnum;

    // we found a partial match
    if (is_abbrev(name, ptr->name)) {
       // count how many of them
       cpt++;
       if (first == -1) 
         // memorise the first match
         first = ptr->vnum;
    }
  }
  // only 1 match return it, otherwise -1
  return (cpt == 1) ? first : -1;
}

struct str_spells *get_spell_by_vnum(int vnum)
{
  struct str_spells *ptr;

  for (ptr = list_spells; ptr; ptr = ptr->next)
    if (ptr->vnum == vnum) 
      return ptr;
  return NULL;
}

int get_spell_level_by_vnum(int vnum, int class)
{
 int i;
 struct str_spells *spell = get_spell_by_vnum(vnum);

 if (spell) {
   for (i=0; i<NUM_CLASSES; i++)
     if (spell->assign[i].class_num == class)
       return spell->assign[i].level;
 }
 return 0;
}

struct str_spells *get_spell_by_name(char *name, char type)
{
  struct str_spells *ptr;

  for (ptr = list_spells; ptr; ptr = ptr->next)
    if (is_abbrev(name, ptr->name) && (ptr->type == type)) 
      return ptr;
  return NULL;
}

int find_spell_by_name (struct descriptor_data *d, char *name) 
{
  struct str_spells *ptr;

  int vnum = 0;

  if (OLC_SEARCH(d))
    ptr = OLC_SEARCH(d);
  else
    ptr = list_spells;

  while (ptr) {
    if (strstr(ptr->name, name)) {
      if (vnum == 0)
        vnum = ptr->vnum;
      else {
        OLC_SEARCH(d) = ptr; 
        return vnum;
      }
    }
    ptr = ptr->next;
  }
  OLC_SEARCH(d) = NULL;
  return vnum;
}

char *spedit_list_targ_flags (int flags) {
  char buf[2048];

  int i;

  if (flags == 0)
    strcpy (buf, "NONE");
  else {
    buf[0] = '\0';
    for (i=0; i<NUM_SPELL_FLAGS; i++)
      if (flags & (1 << i))
        sprintf (buf, "%s%s ", buf, targ_flags [i]);
  }
  return (strdup (buf));
}

char *spedit_list_mag_flags (int flags) {
  char buf[2048];

  int i;

  if (flags == 0)
    strcpy (buf, "NONE");
  else {
    buf[0] = '\0';
    for (i=0; i<NUM_MAG; i++)
      if (flags & (1 << i))
        sprintf (buf, "%s%s ", buf, mag_flags [i]);
  }
  return (strdup (buf));
}

void spedit_assign_menu (struct descriptor_data *d) {
  char buf[2048] = "\r\n";

  int i;  
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  for (i=0; i<NUM_CLASSES; i++)
    sprintf (buf, "%s%s%d%s) Name    : %s%s %s(%s%3d%s) \r\n   %sPrac    : %s%s\r\n   "
                  "%sMana    : %s%s\r\n",
                  buf, 
                  grn, i + 1, nrm, yel, 
                  Q->assign[i].class_num != -1 ? pc_class_types [Q->assign[i].class_num] : "<empty>",
                  nrm, cyn, Q->assign[i].level, nrm,
                  nrm, cyn, EMPTY_STR(Q->assign[i].num_prac),
                  nrm, cyn, EMPTY_STR(Q->assign[i].num_mana));
  sprintf (buf, "%s\r\n%sEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_ASSIGN_MENU;
}

void spedit_apply_menu (struct descriptor_data *d) {
  char buf[2048] = "\r\n";
  char buf1[2048];

  int i;
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  for (i=0; i<MAX_SPELL_AFFECTS; i++) {
    buf1[0] = '\0';
    if (Q->applies[i].appl_num < NUM_APPLIES)
      sprintf (buf1, "   %sModifier : %s%s\r\n", nrm, cyn, EMPTY_STR(Q->applies[i].modifier));
    sprintf (buf, "%s%s%d%s) Name     : %s%s\r\n%s   %sDuration : %s%s\r\n", 
                buf, grn, i + 1, nrm, yel,
                Q->applies[i].appl_num != -1 ? Q->applies[i].appl_num >= NUM_APPLIES ? 
                                               affected_bits [Q->applies[i].appl_num - NUM_APPLIES] : 
                                               apply_types [Q->applies[i].appl_num] : "<empty>", 
                buf1,
                nrm, cyn, EMPTY_STR(Q->applies[i].duration));
  }
  sprintf (buf, "%s\r\n%sEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_APPLY_MENU;
}

void spedit_protection_menu (struct descriptor_data *d) {
  char buf[2048] = "\r\n";

  int i;
  char *name;
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  for (i=0; i<MAX_SPELL_PROTECTIONS; i++) {
    name = get_spell_name (Q->protfrom[i].prot_num);
    sprintf (buf, "%s%s%d%s) Name     : %s%s %s(%s%d%s)\r\n   %sDuration : %s%s\r\n%s   Resist %% : %s%s\r\n", 
                buf, grn, i + 1, nrm, yel, name,
                nrm, cyn, Q->protfrom[i].prot_num, nrm,
                nrm, cyn, EMPTY_STR(Q->protfrom[i].duration),
                nrm, cyn, EMPTY_STR(Q->protfrom[i].resist));
  }
  sprintf (buf, "%s\r\n%sEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_PROTECTION_MENU;
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

void spedit_targ_flags_menu (struct descriptor_data *d) {
  char buf[2048];

  int i;
  char *str_targ_flags;

  str_targ_flags = spedit_list_targ_flags (OLC_SPELL(d)->targ_flags);
  sprintf (buf, "%s\r\n-- FLAGS :     %s%s\r\n", nrm, cyn, str_targ_flags);
  for (i=0; i < NUM_SPELL_FLAGS; i++)
    sprintf (buf, "%s%s%2d%s) %s%-15s%s", buf, grn, i + 1, nrm, yel,
                   targ_flags [i], (i + 1) % 4 ? "" : "\r\n" );
  sprintf (buf, "%s%s\r\nEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  free (str_targ_flags);
  OLC_MODE(d) = SPEDIT_SHOW_TARG_FLAGS;
}

void spedit_mag_flags_menu (struct descriptor_data *d) {
  char buf[2048];

  int i;
  char *str_mag_flags;

  str_mag_flags = spedit_list_mag_flags (OLC_SPELL(d)->mag_flags);
  sprintf (buf, "%s\r\n-- FLAGS :     %s%s\r\n", nrm, cyn, str_mag_flags);
  for (i=0; i < NUM_MAG; i++)
    sprintf (buf, "%s%s%2d%s) %s%-15s%s", buf, grn, i + 1, nrm, yel,
                   mag_flags [i], (i + 1) % 4 ? "" : "\r\n" );
  sprintf (buf, "%s%s\r\nEnter choice (0 to quit) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  free (str_mag_flags);
  OLC_MODE(d) = SPEDIT_SHOW_MAG_FLAGS;
}

void spedit_choose_apply (struct descriptor_data *d) {
  char buf[2048];

  int i, cpt;

  sprintf (buf, "%s\r\n-- APPLIES : \r\n", nrm);
  for (i=0; i < NUM_APPLIES; i++)
    sprintf (buf, "%s%s%2d%s) %s%-15s%s", buf, grn, i + 1, nrm, yel, 
                   apply_types [i], (i + 1) % 4 ? "" : "\r\n" );
  sprintf (buf, "%s\r\n\r\n%s-- AFFECTS : \r\n", buf, nrm);
  cpt = i + 1; 
  for (i=cpt; i < cpt + NUM_AFF_FLAGS; i++)
    sprintf (buf, "%s%s%2d%s) %s%-15s%s", buf, grn, i, nrm, yel, affected_bits [i - NUM_APPLIES],
                  (i - cpt + 1) % 4 ? "" : "\r\n");
  sprintf (buf, "%s%s\r\n\r\nEnter choice (0 to quit, 'r' to remove) : ", buf, nrm);
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
  sprintf (buf, "%s%s\r\n\r\nEnter choice (0 to quit, 'r' to remove) : ", buf, nrm);
  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_ASSIGNEMENT;
}

void spedit_show_warnings (struct descriptor_data *d) {
  char buf[2048] = "";

  if (OLC_SPELL(d)->status != available)
    strcat(buf, "Spell status is unavailable.\r\n");

  if (!is_assign_set(OLC_SPELL(d)))
    strcat(buf, "Spell is not assigned to any classes.\r\n");

  if (!OLC_SPELL(d)->effectiveness)
    strcat(buf, "Spell effectiveness is not set.\r\n");

  if (OLC_SPELL(d)->damages && !(OLC_SPELL(d)->mag_flags & MAG_DAMAGE))
    strcat(buf, "Magic flags: MAG_DAMAGE is required because damages is set.\r\n");

  if (OLC_SPELL(d)->damages && !(OLC_SPELL(d)->mag_flags & MAG_VIOLENT))
    strcat(buf, "Magic flags: MAG_VIOLENT is required because damages is set.\r\n");

  if (is_apply_set(OLC_SPELL(d)) && !(OLC_SPELL(d)->mag_flags & MAG_AFFECTS))
    strcat(buf, "Magic flags: MAG_AFFECTS is required because affects or applies are set.\r\n");

  if ((OLC_SPELL(d)->mag_flags & MAG_AFFECTS) && (OLC_SPELL(d)->mag_flags & MAG_UNAFFECTS))
    strcat(buf, "Magic flags: AFFECTS and UNAFFECTS are both set.\r\n");

  if (is_objects_set(OLC_SPELL(d)) && !(OLC_SPELL(d)->mag_flags & MAG_CREATIONS))
    strcat(buf, "Magic flags: MAG_CREATIONS is required because create objects is set.\r\n");

  if (*buf)
    send_to_char (d->character, "\r\n%s", buf);
}

void spedit_show_messages(struct descriptor_data *d) {
  char buf[2048];

  sprintf (buf, "\r\n1) Wear off  : %s\r\n"
                "2) To self   : %s\r\n"
                "3) To victim : %s\r\n"
                "4) To room   : %s\r\n" 
                "\r\nEnter choice (0 to quit) : ",
                EMPTY_STR(OLC_SPELL(d)->messages.wear_off),
                EMPTY_STR(OLC_SPELL(d)->messages.to_self),
                EMPTY_STR(OLC_SPELL(d)->messages.to_vict),
                EMPTY_STR(OLC_SPELL(d)->messages.to_room));

  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_MESSAGES;
}

void spedit_show_objects(struct descriptor_data *d) {
  char buf[2048];

  sprintf (buf, "\r\n1) Object : %s\r\n"
                "2) Object : %s\r\n"
                "3) Object : %s\r\n"
                "\r\nEnter choice (0 to quit) : ",
                EMPTY_STR(OLC_SPELL(d)->objects[0]),
                EMPTY_STR(OLC_SPELL(d)->objects[1]),
                EMPTY_STR(OLC_SPELL(d)->objects[2]));

  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_OBJECTS;
}

void spedit_main_menu (struct descriptor_data *d) {
  char buf[2048];

  char *str_targ_flags;
  char *str_mag_flags;
  struct str_spells *Q;

  Q = OLC_SPELL(d);
  str_targ_flags = spedit_list_targ_flags (Q->targ_flags);
  str_mag_flags = spedit_list_mag_flags (Q->mag_flags);

  get_char_colors (d->character);

  bool prog = Q->function != NULL;

#if defined (CLEAR_SCREEN)
  send_to_char (d->character, "[H[J");
#endif         

  sprintf (buf, "%s-- %s Number      : [%s%5d%s] %s%s%s\r\n"
                "%sT%s) Type              : [%s%-5s%s]\r\n"
                "%s1%s) %sStatus            : %s%s\r\n"  
                "%s2%s) Name              : %s%s\r\n"
                "%s3%s) Min position      : %s%s\r\n"
                "%s4%s) Target FLAGS      : %s%s\r\n"
                "%s5%s) Magic FLAGS       : %s%s\r\n"
                "%s6%s) Damages           : %s%s %s(%s%4d%s)\r\n"
                "%s7%s) Pulse delay       : %s%s\r\n"
                "%s8%s) %sEffectiveness %%   : %s%s\r\n"
                "%s9%s) %sMenu -> Points\r\n"
                "%sP%s) %sMenu -> Protection from\r\n"
                "%sA%s) %sMenu -> Applies & Affects\r\n"
                "%sD%s) %sMenu -> Dispells\r\n"
                "%sO%s) %sMenu -> Create objects\r\n"
                "%sX%s) %sMenu -> Summon mobiles\r\n"
                "%sS%s) %sMenu -> Script\r\n" 
                "%sC%s) %sMenu -> Classes\r\n"
                "%sM%s) %sMenu -> Messages\r\n"
                "%sW%s) Warnings\r\n"
                "%sQ%s) Quit\r\n\r\n"
                "%sEnter choice : ",
                 nrm, (Q->type == SPELL) ? "Spell" : "Skill", cyn, OLC_NUM(d), nrm, yel, Q->function ? "Special" : "", nrm,
                 prog ? red : grn, nrm, cyn, (Q->type == SPELL) ? "SPELL" : "SKILL", nrm,
                 grn, nrm, (Q->status == available) ? nrm : YEL, yel, (Q->status == available) ? "Available" : "Unavailable",
                 prog ? red : grn, nrm, yel, Q->name ? Q->name : "Undefined", 
                 prog ? red : grn, nrm, cyn, ((Q->min_pos >= 0) && (Q->min_pos < NUM_CHAR_POSITION)) ? 
                              position_types [Q->min_pos] : "<ILLEGAL>",   
                 prog ? red : grn, nrm, cyn, str_targ_flags,
                 prog ? red : grn, nrm, cyn, str_mag_flags,
                 prog ? red : grn, nrm, cyn, EMPTY_STR(Q->damages), nrm, cyn, Q->max_dam, nrm,  
                 prog ? red : grn, nrm, cyn, EMPTY_STR(Q->delay),
                 prog ? red : grn, nrm, Q->effectiveness ? nrm : YEL, cyn, EMPTY_STR(Q->effectiveness),
                 prog ? red : grn, nrm, is_points_set(Q) ? bln : nrm,
                 prog ? red : grn, nrm, is_prot_set(Q) ? bln : nrm,
                 prog ? red : grn, nrm, is_apply_set(Q) ? bln : nrm, 
                 prog ? red : grn, nrm, is_dispel_set(Q) ? bln : nrm,
                 prog ? red : grn, nrm, is_objects_set(Q) ? bln : nrm,
                 prog ? red : grn, nrm, is_summon_set(Q) ? bln :  nrm,
                 prog ? red : grn, nrm, Q->script ? bln : nrm,
                 prog ? red : grn, nrm, is_assign_set(Q) ? bln : YEL,
                 prog ? red : grn, nrm, is_messages_set(Q) ? bln : nrm,
                 prog ? red : grn, nrm,
                 grn, nrm,
                 nrm);
  send_to_char (d->character, "%s", buf);
  free (str_targ_flags);
  free (str_mag_flags);
  OLC_MODE (d) = SPEDIT_MAIN_MENU;
}

void spedit_empty_spell (struct str_spells *spell) {
  int i;

  SAFE_FREE(spell->name);
  SAFE_FREE(spell->damages);
  SAFE_FREE(spell->effectiveness);
  SAFE_FREE(spell->delay);
  SAFE_FREE(spell->script);

  SAFE_FREE(spell->messages.wear_off);
  SAFE_FREE(spell->messages.to_self);
  SAFE_FREE(spell->messages.to_vict);
  SAFE_FREE(spell->messages.to_room);

  for (i=0; i<MAX_SPELL_PROTECTIONS; i++) {
    SAFE_FREE(spell->protfrom[i].duration);
    SAFE_FREE(spell->protfrom[i].resist);
  }

  for (i=0; i<MAX_SPELL_AFFECTS; i++) {
    SAFE_FREE(spell->applies[i].modifier);
    SAFE_FREE(spell->applies[i].duration);
  }

  for (i=0; i<MAX_SPELL_OBJECTS; i++)
    SAFE_FREE(spell->objects[i]);

  for (i=0; i<NUM_CLASSES; i++) {
    SAFE_FREE(spell->assign[i].num_prac);
    SAFE_FREE(spell->assign[i].num_mana);
  }
}

void spedit_free_spell (struct str_spells *spell) {
  if (!spell)
    return;

  spedit_empty_spell(spell);

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

void spedit_copyover_spell (struct str_spells *from, struct str_spells *to)
{
  int i;

  spedit_empty_spell(to);

  to->status = from->status;
  to->type = from->type;
  to->name = NULL_STR(from->name);
  to->targ_flags = from->targ_flags;
  to->mag_flags = from->mag_flags;
  to->min_pos = from->min_pos;
  to->max_dam = from->max_dam;
  to->effectiveness = NULL_STR(from->effectiveness);
  to->damages = NULL_STR(from->damages);
  to->delay = NULL_STR(from->delay);
  to->script = NULL_STR(from->script);

  to->messages.wear_off = NULL_STR(from->messages.wear_off);
  to->messages.to_self = NULL_STR(from->messages.to_self);
  to->messages.to_vict = NULL_STR(from->messages.to_vict);
  to->messages.to_room = NULL_STR(from->messages.to_room);

  for (i=0; i<MAX_SPELL_PROTECTIONS; i++) {
    to->protfrom[i].prot_num = from->protfrom[i].prot_num;
    to->protfrom[i].duration = NULL_STR(from->protfrom[i].duration);
    to->protfrom[i].resist = NULL_STR(from->protfrom[i].resist);
  }

  for (i=0; i<MAX_SPELL_AFFECTS; i++) {
    to->applies[i].appl_num = from->applies[i].appl_num;
    to->applies[i].modifier = NULL_STR(from->applies[i].modifier);
    to->applies[i].duration = NULL_STR(from->applies[i].duration);
  }
 
  for (i=0; i<MAX_SPELL_OBJECTS; i++)
    to->objects[i] = NULL_STR(from->objects[i]);

  for (i=0; i<NUM_CLASSES; i++) {
    to->assign[i].class_num = from->assign[i].class_num;
    to->assign[i].level = from->assign[i].level;
    to->assign[i].num_prac = NULL_STR(from->assign[i].num_prac);
    to->assign[i].num_mana = NULL_STR(from->assign[i].num_mana);
  }
  to->function = from->function;
}

void spedit_save_internally (struct str_spells *spell) 
{
 struct str_spells *i, *p = NULL;

 for (i = list_spells; i; p = i, i = i->next)
   if (i->vnum >= spell->vnum)
     break;

 if (i && (i->vnum == spell->vnum)) {
   if (!i->function)
     spedit_copyover_spell(spell, i);
   else
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

 spell->next     = NULL;
 spell->status   = unavailable;
 spell->type     = 'P';
 spell->name     = strdup ("Undefined");
 spell->targ_flags = 0;
 spell->mag_flags = 0;
 spell->min_pos  = 0;
 spell->max_dam  = 0;
 spell->violent  = FALSE;
 spell->effectiveness = NULL;
 spell->damages  = NULL;
 spell->delay    = NULL;
 spell->script   = NULL;

 spell->messages.wear_off = NULL;
 spell->messages.to_self = NULL;
 spell->messages.to_vict = NULL;
 spell->messages.to_room = NULL;

 for (i=0; i<MAX_SPELL_PROTECTIONS; i++) {
   spell->protfrom[i].prot_num = -1;
   spell->protfrom[i].duration = NULL;
   spell->protfrom[i].resist   = NULL;
 }

 for (i=0; i<MAX_SPELL_AFFECTS; i++) {
   spell->applies[i].appl_num  = -1;
   spell->applies[i].modifier  = NULL;
   spell->applies[i].duration  = NULL;
 }

 for (i=0; i<MAX_SPELL_OBJECTS; i++)
   spell->objects[i] = NULL;

 for (i=0; i<NUM_CLASSES; i++) {
   spell->assign[i].class_num  = -1;
   spell->assign[i].level      = 0;
   spell->assign[i].num_prac   = NULL;
   spell->assign[i].num_mana   = NULL;
 }
 spell->function               = NULL;
}

int spedit_create_spell (struct descriptor_data *d)
{
 struct str_spells *q = NULL;

 int vnum = 0;

 // if OLC_NUM(d) != 0 we search that vnum spell, 
 // otherwise we search the end of the list to create a new spell there. at last VNUM + 1.
 for (q = list_spells; q; q = q->next) {
   vnum = q->vnum;

   if (q->vnum && (q->vnum == OLC_NUM(d)))
     break;
 }

 // if OLC_NUM(d) == 0 that means we want to create a new spell,
 // if so we start from last VNUM + 1 and we search for the first
 // free VNUM that isn't OLCING by someone else.
 if (!OLC_NUM(d))
   while (IS_SPELL_OLCING(++vnum));

 if (vnum > MAX_SKILLS) {
   mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: Spedit: Skills and spells limits reached.");
   return 0;
 }

 OLC_NUM(d) = vnum;

 CREATE (OLC_SPELL(d), struct str_spells, 1);
 OLC_SPELL(d)->vnum = OLC_NUM(d);

 if (q) 
   spedit_copyover_spell(q, OLC_SPELL(d));
 else
   spedit_init_new_spell(OLC_SPELL(d));

 return 1;
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
      mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: attemp to assign value to Q == (null)");
      abort();
    }
    switch (fct) {
      case 1 : if (save == 1) 
                 spedit_save_internally (Q); 
               else  
                 save = 1;
               CREATE (Q, struct str_spells, 1);
               spedit_init_new_spell (Q);
               ret = fscanf (fp, "%c %d %d %d %d %d %d\n", &Q->type, &Q->vnum, &Q->status,
                             &Q->targ_flags, &Q->mag_flags, &Q->min_pos, &Q->max_dam);
               if (Q->vnum > MAX_SKILLS) {
                 mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: spell vnum > MAX_SKILLS");
                 abort();
               }
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
      case 5 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.wear_off = strdup (buf);
               }
               break;
      case 6 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.to_self = strdup (buf);
                 log("yes");
               }
               break;
      case 7 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.to_vict = strdup (buf);
               }
               break;
      case 8 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.to_room = strdup (buf);
               }
               break;
      case 14 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->objects[0] = strdup (buf);
                }
                break;
      case 15 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->objects[1] = strdup (buf);
                }
                break;
      case 16 : if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->objects[2] = strdup (buf);
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
 char buf[2048];  // fix me: unsafe
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
   sprintf (buf, "01 %c %d %d %d %d %d %d\n",
                  r->type, r->vnum, r->status, r->targ_flags, r->mag_flags, r->min_pos, r->max_dam);
   if (r->name)
     sprintf (buf, "%s02 %s\n", buf, r->name);

   /* code 03 is FREE */

   if (r->damages)
     sprintf (buf, "%s04 %s\n", buf, r->damages);

   /* 5 6 7 8 */
   if (r->messages.wear_off)
     sprintf (buf, "%s05 %s\n", buf, r->messages.wear_off);

   if (r->messages.to_self)
     sprintf (buf, "%s06 %s\n", buf, r->messages.to_self);

   if (r->messages.to_vict)
     sprintf (buf, "%s07 %s\n", buf, r->messages.to_vict);

   if (r->messages.to_room)
     sprintf (buf, "%s08 %s\n", buf, r->messages.to_room);

   /* 9 10 11 12 13 Are FREE */ 

   /* 14 15 16 = spell->objects[] */
   if (r->objects[0])
     sprintf (buf, "%s14 %s\n", buf, r->objects[0]);

   if (r->objects[1])
     sprintf (buf, "%s15 %s\n", buf, r->objects[1]);

   if (r->objects[2])
     sprintf (buf, "%s16 %s\n", buf, r->objects[2]);

   /* 17 18 19 20 21 22 */
   for (i=0; i<MAX_SPELL_PROTECTIONS; i++)
     if (r->protfrom[i].prot_num != -1) 
       sprintf (buf, "%s%d %d %s\n00 %s\n", buf, i + 17,
                r->protfrom[i].prot_num, r->protfrom[i].duration, r->protfrom[i].resist);
   /* 23 24 25 26 27 28 */
   for (i=0; i<MAX_SPELL_AFFECTS; i++)
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

 if (spedit_create_spell (d))
   spedit_main_menu (d);
 else {
   send_to_char (d->character, "Failed the limits of spells and skills has been reached!\r\n");
   cleanup_olc (d, CLEANUP_ALL);
   return 0;
 }

 return 1;
}

int spedit_setup (struct descriptor_data *d)
{
 char buf[2048];

 char *str;
 int vnum;
 
 if (OLC_STORAGE(d)) {
   if (is_number(OLC_STORAGE(d))) 
     vnum = find_spell_by_vnum(atoi(OLC_STORAGE(d))); 
   else
     vnum = find_spell_by_name(d, OLC_STORAGE(d));

   if (!vnum) {
     send_to_char (d->character, "that spell could not be found!\r\n");
     cleanup_olc (d, CLEANUP_ALL);
     return 0;
   } else {
       if ((str = IS_SPELL_OLCING (vnum))) {
         sprintf (buf, "This spell '%s' (vnum: %d) is already edited by %s.\r\n", get_spell_name(vnum), vnum, str);
         send_to_char (d->character, "%s", buf);  
         cleanup_olc (d, CLEANUP_ALL);
         return 0;
       }

       OLC_NUM(d) = vnum;
       sprintf (buf, "Do you want to edit '%s' (vnum: %d)? (y/n%s): ", get_spell_name(vnum), vnum, 
                     OLC_SEARCH(d) ? ", q" : "");
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
  struct str_spells *to;

  switch (OLC_MODE(d)) {
    case SPEDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
          case 'y' :
          case 'Y' : to = get_spell_by_vnum(OLC_NUM(d));

                     if (!to) {
                       spedit_save_internally(OLC_SPELL(d));
                       OLC_SPELL(d) = NULL;  // now, it's not a copy, it's saved in memory. 
                                             // the pointer must be NULLed, to avoid cleanup_olc to free the structure.
                     }
                     else 
                       spedit_copyover_spell(OLC_SPELL(d), to); 

                     sprintf (buf, "OLC: %s edits spells %d.", GET_NAME(d->character), OLC_NUM(d));
                     mudlog (CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "%s", buf);

                     send_to_char (d->character, "Spell saved to disk.\r\n");
                     spedit_save_to_disk();

                     cleanup_olc (d, CLEANUP_ALL);
                     break;
          case 'n' :
          case 'N' : cleanup_olc (d, CLEANUP_ALL); 
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
                               SAFE_FREE(OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana);
                               OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana =
                                  (arg && *arg) ? strdup (arg) : NULL;
                               spedit_assign_menu(d);
                             } else 
                                 send_to_char (d->character, "Num mana : ");
                             return;
    case SPEDIT_GET_NAME   : SAFE_FREE(OLC_SPELL(d)->name);
                             OLC_SPELL(d)->name = 
                               (arg && *arg) ? strdup(arg) : strdup("Undefined");
                             break;
    case SPEDIT_GET_PROTDUR: value = formula_interpreter (d->character, 
                                     d->character, OLC_NUM(d), FALSE,
                                     arg, &rts_code);
                             if (!rts_code) { 
                               SAFE_FREE(OLC_SPELL(d)->protfrom[OLC_VAL(d)].duration);
                               OLC_SPELL(d)->protfrom[OLC_VAL(d)].duration = 
                                 (arg && *arg) ? strdup (arg) : NULL;
                               send_to_char (d->character, "Resist %% : ");
                               OLC_MODE(d) = SPEDIT_GET_RESIST;
                             } else 
                                 send_to_char (d->character, "Duration : ");
                             return;
    case SPEDIT_GET_SPELL_NUM : if ((*arg == 'r') || (*arg == 'R')) {
                                  OLC_SPELL(d)->protfrom[OLC_VAL(d)].prot_num = -1;
                                  SAFE_FREE(OLC_SPELL(d)->protfrom[OLC_VAL(d)].duration);
                                  OLC_SPELL(d)->protfrom[OLC_VAL(d)].duration = NULL;
                                  SAFE_FREE(OLC_SPELL(d)->protfrom[OLC_VAL(d)].resist);
                                  OLC_SPELL(d)->protfrom[OLC_VAL(d)].resist = NULL;
                                  spedit_protection_menu (d);
                                  return; 
                                }
                                if (!atoi(arg)) 
                                  spedit_protection_menu (d);
                                else {
                                  int vnum = atoi(arg);
                                  if (!find_spell_by_vnum(vnum)) {
                                    send_to_char (d->character, "Invalid: spell not found!\r\n"
                                                                "\r\nSpell VNUM (0 to quit, 'r' to remove) : ");
                                  } else {
                                      OLC_SPELL(d)->protfrom[OLC_VAL(d)].prot_num = vnum;
                                      send_to_char (d->character, "Duration : ");
                                      OLC_MODE(d) = SPEDIT_GET_PROTDUR;
                                    }
                                }
                                return; 
    case SPEDIT_GET_NUMPRAC: value = formula_interpreter (d->character, 
                                     d->character, OLC_NUM(d), FALSE,
                                     arg, &rts_code);
                             if (!rts_code) {  
                                SAFE_FREE(OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac);
                                OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac = 
                                  (arg && *arg) ? strdup (arg) : NULL;
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
                               SAFE_FREE(OLC_SPELL(d)->applies[OLC_VAL(d)].modifier);
                               OLC_SPELL(d)->applies[OLC_VAL(d)].modifier = 
                                 (arg && *arg) ? strdup(arg) : NULL;
                               send_to_char (d->character, "Duration : ");
                               OLC_MODE (d) = SPEDIT_GET_APPLDUR;
                              } else 
                                  send_to_char (d->character, "Modifier : ");
                              return;
    case SPEDIT_GET_APPLDUR : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE, 
                                      arg, &rts_code);
                              if (!rts_code) {
                                SAFE_FREE(OLC_SPELL(d)->applies[OLC_VAL(d)].duration);
                                OLC_SPELL(d)->applies[OLC_VAL(d)].duration = 
                                  (arg && *arg) ? strdup (arg) : NULL;
                                spedit_apply_menu (d);
                              } else 
                                  send_to_char (d->character, "Duration : ");
                              return;
    case SPEDIT_GET_MAXDAM  : OLC_SPELL(d)->max_dam = atoi(arg);
                              break;
    case SPEDIT_GET_DAMAGES : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE,
                                      arg, &rts_code);
                              if (!rts_code) {
                                SAFE_FREE(OLC_SPELL(d)->damages);
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
                                SAFE_FREE(OLC_SPELL(d)->effectiveness);
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
                                SAFE_FREE(OLC_SPELL(d)->protfrom[OLC_VAL(d)].resist);
                                OLC_SPELL(d)->protfrom[OLC_VAL(d)].resist = 
                                  (arg && *arg) ? strdup (arg) : NULL;
                                spedit_protection_menu(d);
                              } else  
                                  send_to_char (d->character, "Resist %% : ");
                              return;
    case SPEDIT_GET_DELAY   : value = formula_interpreter (d->character,
                                      d->character, OLC_NUM(d), FALSE,
                                      arg, &rts_code);
                              if (!rts_code) {
                                SAFE_FREE(OLC_SPELL(d)->delay);
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
             if ((x > 0) && (x < MAX_SPELL_AFFECTS)) {
               OLC_VAL(d) = x - 1;  
               spedit_choose_apply (d);
             } else {
                 send_to_char (d->character, "Invalid choice!\r\n");
                 spedit_apply_menu (d);
               } 
           return; 
    case SPEDIT_SHOW_APPLY : 
           if ((*arg == 'r') || (*arg == 'R')) {
             OLC_SPELL(d)->applies[OLC_VAL(d)].appl_num = - 1;
             SAFE_FREE(OLC_SPELL(d)->applies[OLC_VAL(d)].modifier);
             OLC_SPELL(d)->applies[OLC_VAL(d)].modifier = NULL;
             SAFE_FREE(OLC_SPELL(d)->applies[OLC_VAL(d)].duration);
             OLC_SPELL(d)->applies[OLC_VAL(d)].duration = NULL;
             spedit_apply_menu (d);
             return; 
           }

           if (!(x = atoi(arg))) 
             spedit_apply_menu (d);
           else
             if ((x < 0) || (x > NUM_APPLIES + NUM_AFF_FLAGS)) {
               send_to_char (d->character, "Invalid choice!\r\n");
               spedit_choose_apply (d);
             } else {
                  if (x <= NUM_APPLIES) {
                    OLC_SPELL(d)->applies[OLC_VAL(d)].appl_num = x - 1;
                    send_to_char (d->character, "Modifier : ");
                    OLC_MODE(d) = SPEDIT_GET_MODIF;
                  } else {
                      OLC_SPELL(d)->applies[OLC_VAL(d)].appl_num = x;
                      SAFE_FREE(OLC_SPELL(d)->applies[OLC_VAL(d)].modifier);
                      OLC_SPELL(d)->applies[OLC_VAL(d)].modifier = NULL;
                      send_to_char (d->character, "Duration : ");
                      OLC_MODE (d) = SPEDIT_GET_APPLDUR;
                    }
               }
           return; 
    case SPEDIT_SHOW_ASSIGNEMENT :
           if ((*arg == 'r') || (*arg == 'R')) {
             OLC_SPELL(d)->assign[OLC_VAL(d)].class_num = -1;
             OLC_SPELL(d)->assign[OLC_VAL(d)].level = 0;
             SAFE_FREE(OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac);
             OLC_SPELL(d)->assign[OLC_VAL(d)].num_prac = NULL;
             SAFE_FREE(OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana);
             OLC_SPELL(d)->assign[OLC_VAL(d)].num_mana = NULL;
             spedit_assign_menu(d);
             return;
           }
           if (!(x = atoi (arg))) 
             spedit_assign_menu (d);
           else 
           if ((x < 1) || (x > NUM_CLASSES)) {
             send_to_char (d->character, "Invalid choice!\r\n");
             spedit_assignement_menu (d);
           } else {
               OLC_SPELL(d)->assign[OLC_VAL(d)].class_num = x - 1;
               send_to_char (d->character, "Level : ");
               OLC_MODE(d) = SPEDIT_GET_LEVEL;
             }
           return;
    case SPEDIT_GET_MSG_WEAR_OFF :
         SAFE_FREE(OLC_SPELL(d)->messages.wear_off);
         OLC_SPELL(d)->messages.wear_off = (arg && *arg) ? strdup(arg) : NULL;
         spedit_show_messages (d);
         return;
    case SPEDIT_GET_MSG_TO_SELF :
         SAFE_FREE(OLC_SPELL(d)->messages.to_self);
         OLC_SPELL(d)->messages.to_self = (arg && *arg) ? strdup(arg) : NULL;
         spedit_show_messages (d);
         return;
    case SPEDIT_GET_MSG_TO_VICT :
         SAFE_FREE(OLC_SPELL(d)->messages.to_vict);
         OLC_SPELL(d)->messages.to_vict = (arg && *arg) ? strdup(arg) : NULL;
         spedit_show_messages (d);
         return;
    case SPEDIT_GET_MSG_TO_ROOM :
         SAFE_FREE(OLC_SPELL(d)->messages.to_room);
         OLC_SPELL(d)->messages.to_room = (arg && *arg) ? strdup(arg) : NULL;
         spedit_show_messages (d);
         return;
    case SPEDIT_SHOW_MESSAGES :
         x = atoi(arg); 
         switch (x) {
           case 0 : break;
           case 1 : send_to_char(d->character, "Wear off : ");
                    OLC_MODE(d) = SPEDIT_GET_MSG_WEAR_OFF; return;
           case 2 : send_to_char(d->character, "To self: ");
                    OLC_MODE(d) = SPEDIT_GET_MSG_TO_SELF; return;
           case 3 : send_to_char(d->character, "To victim: ");  
                    OLC_MODE(d) = SPEDIT_GET_MSG_TO_VICT; return;
           case 4 : send_to_char(d->character, "To room: "); 
                    OLC_MODE(d) = SPEDIT_GET_MSG_TO_ROOM; return;
           default : send_to_char (d->character, "Invalid choice!\r\n");
                     spedit_show_messages (d);
                     return;
           }
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
    case SPEDIT_SHOW_TARG_FLAGS : 
         if (!(x = atoi (arg))) break;
         else 
            if ( (x < 0) || (x > NUM_SPELL_FLAGS) ) 
              send_to_char (d->character, "Invalid choice!\r\n");
            else
              OLC_SPELL(d)->targ_flags ^= (1 << (x - 1));
         spedit_targ_flags_menu (d);
         return;
    case SPEDIT_SHOW_MAG_FLAGS:
         if (!(x = atoi (arg))) break;
         else 
            if ( (x < 0) || (x > NUM_MAG) ) 
              send_to_char (d->character, "Invalid choice!\r\n");
            else
              OLC_SPELL(d)->mag_flags ^= (1 << (x - 1));
         spedit_mag_flags_menu (d);
         return;
    case SPEDIT_PROTECTION_MENU :
         if (!(x = atoi (arg))) break;
         else 
           if ( (x < 0) || (x > MAX_SPELL_PROTECTIONS) ) {
             send_to_char (d->character, "Invalid choice!\r\n");
             spedit_protection_menu (d);
             return;
           }
         OLC_VAL(d) = x - 1;
         send_to_char (d->character, "Spell VNUM (0 to quit, 'r' to remove) : ");
         OLC_MODE(d) = SPEDIT_GET_SPELL_NUM;
         return;   
    case SPEDIT_GET_OBJECT : if (!*arg) {
                               SAFE_FREE(OLC_SPELL(d)->objects[OLC_VAL(d)]); 
                               OLC_SPELL(d)->objects[OLC_VAL(d)] = NULL;
                               spedit_show_objects(d);
                               return;
                             }
                             value = formula_interpreter (d->character,
                                        d->character, OLC_NUM(d), FALSE,
                                        arg, &rts_code);
                             if (!rts_code) {
                               SAFE_FREE(OLC_SPELL(d)->objects[OLC_VAL(d)]); 
                               OLC_SPELL(d)->objects[OLC_VAL(d)] = strdup(arg);
                             }
                             spedit_show_objects(d);
                             return;
    case SPEDIT_SHOW_OBJECTS :
         if (!(x = atoi(arg))) break;
         if (x > 3) {
           send_to_char (d->character, "Invalid choice!\r\n");
           spedit_show_objects(d);
           return;
         }
         send_to_char(d->character, "Object #%d : ", x);
         OLC_VAL(d) = x - 1;
         OLC_MODE(d) = SPEDIT_GET_OBJECT; 
         return;
    case SPEDIT_CONFIRM_EDIT : 
         if ((*arg == 'y') || (*arg == 'Y')) { 
           send_to_char (d->character, "\r\n");
           spedit_setup2 (d);
         } else
           if (OLC_SEARCH(d) && (*arg != 'q') && (*arg != 'Q')) {
             spedit_setup (d);
           } else
               cleanup_olc (d, CLEANUP_ALL); 
         return; 
    case SPEDIT_MAIN_MENU :
        if (OLC_SPELL(d)->function && *arg != 'q' && *arg != 'Q' && *arg != '1') {
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
          case '1' : if (GET_LEVEL(d->character) < LVL_IMPL) { 
                       send_to_char (d->character, "Only the implentors can set that!\r\n");
                       break;
                     } 
                     else {
                       send_to_char (d->character, "0-Unavailable, 1-Available\r\n\r\n"
                                                   "Enter choice : ");
                       OLC_MODE(d) = SPEDIT_GET_STATUS;
                       return; 
                     }
          case '2' : send_to_char (d->character, "Spell name : ");
                     OLC_MODE(d) = SPEDIT_GET_NAME;
                     return;
          case '3' : spedit_minpos_menu (d);
                     return;
          case '4' : spedit_targ_flags_menu (d);
                     return;
          case '5' : spedit_mag_flags_menu (d);
                     return;
          case '6' : send_to_char (d->character, "Damages : ");
                     OLC_MODE(d) = SPEDIT_GET_DAMAGES;
                     return; 
          case '7' : send_to_char (d->character, "Passes (10 passes = 1 sec) : ");
                     OLC_MODE(d) = SPEDIT_GET_DELAY;
                     return;
          case '8' : send_to_char (d->character, "%% of effectiveness : ");
                     OLC_MODE(d) = SPEDIT_GET_EFFECTIVENESS;
                     return;
          case 'p' :
          case 'P' : spedit_protection_menu (d);
                     return; 
          case 'a' :
          case 'A' : spedit_apply_menu (d);
                     return;
          case 'o' :
          case 'O' : spedit_show_objects (d);
                     return;
          case 's' :
          case 'S' : page_string (d, OLC_SPELL(d)->script, 1);
                     d->backstr  = NULL_STR(OLC_SPELL(d)->script);
                     d->str      = &OLC_SPELL(d)->script;
                     d->max_str  = MAX_STRING_LENGTH;
                     d->mail_to  = 0;
                     OLC_VAL(d)  = 1;
                     return;
          case 'c' :
          case 'C' : spedit_assign_menu (d);
                     return;
          case 'm' :
          case 'M' : spedit_show_messages (d);
                     return;
          case 't' : 
          case 'T' : send_to_char (d->character, "\r\n0-Spell     1-Skill\r\n");
                     send_to_char (d->character, "Enter choice : ");
                     OLC_MODE(d) = SPEDIT_GET_TYPE;
                     return;
          case 'w' :
          case 'W' : spedit_show_warnings (d);
                     break;
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

  if (FIGHTING(ch)) {
    send_to_char(ch, "You should focus on your fight!\r\n");
    return;
  }

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
    OLC_STORAGE(d) = NULL;  // remove? olc_cleanup is supposed to remove and NULL the pointer
 
  STATE(d) = CON_SPEDIT;

  spedit_setup(ch->desc);
}

ACMD(do_splist) {
{
 char buf[MAX_STRING_LENGTH];
 int cpt = 0;
 int search_by_class = CLASS_UNDEFINED;
 int search_by_part_name = 0;
 int search_disabled = 0;
 size_t len = 0, tmp_len = 0;

 struct str_spells *ptr;
 
 skip_spaces(&argument);

 if (!*argument) {
   send_to_char(ch, "splist all - list all spells\r\n"
                    "splist mu  - list all spells in class magical user\r\n" 
                    "splist cl  - list all spells in class cleric\r\n" 
                    "splist th  - list all spells in class thief\r\n" 
                    "splist wa  - list all spells in class warrior\r\n" 
                    "splist off - list all spells disabled\r\n"
                    "splist <word> - list all spells containing the <word>\r\n");
   return;
 }
 
 if(!strcmp(argument, "mu")) 
   search_by_class = CLASS_MAGIC_USER;
 else
 if(!strcmp(argument, "cl"))
   search_by_class = CLASS_CLERIC;
 else
 if(!strcmp(argument, "th"))
   search_by_class = CLASS_THIEF;
 else
 if(!strcmp(argument, "wa"))
   search_by_class = CLASS_WARRIOR;
 else
 if(!strcmp(argument, "off")) 
   search_disabled = 1;
 else
 if(strcmp(argument, "all"))
   search_by_part_name = 1;


 len = snprintf(buf, sizeof(buf), 
 "Index VNum    Name                 Type     Spec_Prog    Available   Classe(s)\r\n"
 "----- ------- ----                 ----     ---------    ---------   -----------\r\n");
 
 for (ptr = list_spells; ptr; ptr = ptr->next) {
   char classes[80] = "";
   int mu, cl, th, wa;

   if ((mu = IS_SPELL_CLASS(ptr, CLASS_MAGIC_USER))) strcat(classes, "Mu ");
   if ((search_by_class == CLASS_MAGIC_USER) && !mu) continue;

   if ((cl = IS_SPELL_CLASS(ptr, CLASS_CLERIC))) strcat(classes, "Cl ");
   if ((search_by_class == CLASS_CLERIC) && !cl) continue;

   if ((th = IS_SPELL_CLASS(ptr, CLASS_THIEF))) strcat(classes, "Th ");
   if ((search_by_class == CLASS_THIEF) && !th) continue;

   if ((wa = IS_SPELL_CLASS(ptr, CLASS_WARRIOR))) strcat(classes, "Wa");
   if ((search_by_class == CLASS_WARRIOR) && !wa) continue;

   if (search_disabled && (ptr->status == available)) continue;

   if (search_by_part_name && !strstr(ptr->name, argument)) continue;

   tmp_len = snprintf(buf+len, sizeof(buf)-len, "%s%4d%s) [%s%5d%s]%s %-20s%s %-5s    %s%-3s          %s%-3s         %s%s%s\r\n",
             QGRN, ++cpt, QNRM, QGRN, ptr->vnum, QNRM, 
             QCYN, ptr->name, QYEL, ptr->type == SPELL ? "SPELL" : "SKILL", 
             QNRM, ptr->function ? "Yes" : "No", 
             ptr->status == available ? QGRN : QRED, ptr->status == available ? "Yes" : "No", 
             QCYN, classes, QNRM); 
   len += tmp_len;
   if (len > sizeof(buf))
     break;
  }

  page_string(ch->desc, buf, TRUE);
 }
}


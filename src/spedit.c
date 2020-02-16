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
#include "constants.h"
#include "class.h"

#define EMPTY_STR(str) ((str) ? (str) : "<empty>")
#define NULL_STR(str) ((str) ? (str) : "")
#define STRDUP(str)  ((str) ? (strdup(str)) : NULL)
#define SAFE_FREE(str) if(str) free(str)
#define BUFSIZE 2048

// global variable
struct str_spells *list_spells = NULL;
int last_spell_vnum = 0;
char *UNDEF_SPELL = "Undefined";

// return a pointer on the player's name that is editing the spell vnum, or NULL if not edited.
// The name will be used to says: Spell is edited by: <name>.
// but, it can also be used to check if a SPELL VNUM is edited.
char *SPELL_OLCING_BY (int vnum) 
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

 for (i=0; i<MAX_SPELL_PROTECTIONS; i++) 
   if (spell->protfrom[i].prot_num != -1)
     return 1;
 return 0;
}

int is_apply_set(struct str_spells *spell)
{
 int i;

 for (i=0; i<MAX_SPELL_AFFECTS; i++) 
   if (spell->applies[i].appl_num != -1)
     return 1;
 return 0;
}

int is_summon_set(struct str_spells *spell)
{
 return spell->summon_mob ? 1 : 0;
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
  int i;

  for (i=0; i<MAX_SPELL_DISPEL; i++)
    if (spell->dispel[i])
      return 1;
  return 0;
}

int is_points_set(struct str_spells *spell)
{
  if (spell->points.hp) return 1;
  if (spell->points.mana) return 1;
  if (spell->points.move) return 1;
  if (spell->points.gold) return 1;

  return 0;
}

int is_messages_set(struct str_spells *spell)
{
  if (spell->messages.wear_off) return 1;
  if (spell->messages.to_self) return 1;
  if (spell->messages.to_vict) return 1;
  if (spell->messages.to_room) return 1;

  return 0;
}

int find_spell_assign (struct char_data *ch, struct str_spells *ptr)
{
 int i;
 
 for (i=0; i<NUM_CLASSES; i++)
   if (ptr->assign[i].class_num == GET_CLASS(ch))
     return i;
 return -1;
}

int get_spell_class(struct str_spells *spell, int class) {
  int i;

  for (i=0; i<NUM_CLASSES; i++)
    if (spell->assign[i].class_num == class)
      return i;
  return -1;
}

int get_spell_mag_flags(int vnum)
{
 struct str_spells *Q;

 for (Q = list_spells; Q; Q=Q->next)
   if (Q->vnum == vnum)
     return (Q->mag_flags);

 log("SYSERR: Spell vnum %d not found at get_spell_mag_flags.", vnum);
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

int find_skill_num (char *name) {
  struct str_spells *ptr;
  int first = -1; 
  int cpt = 0;

  for (ptr = list_spells; ptr; ptr = ptr->next) {
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

// check all the assigned classes.
int get_spell_level(int vnum, int class)
{
 int i;
 struct str_spells *spell = get_spell_by_vnum(vnum);

 if (spell) {
   for (i=0; i<NUM_CLASSES; i++)
     if (spell->assign[i].class_num == class)
       return spell->assign[i].level;
 }
 return -1;
}

struct str_spells *get_spell_by_name(char *name, char type)
{
  struct str_spells *ptr;

  for (ptr = list_spells; ptr; ptr = ptr->next)
    if (is_abbrev(name, ptr->name) && (ptr->type == type)) 
      return ptr;
  return NULL;
}

int olc_spell_by_name (struct descriptor_data *d, char *name) 
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
  const int bSIZE = 2048;
  char buf[bSIZE];
  int len = 0;

  buf[0] = '\0';
  if (OLC_SPELL(d)->status != available)
    len += snprintf(buf, bSIZE, "Spell status is unavailable.\r\n");

  if ((len < bSIZE) && !is_assign_set(OLC_SPELL(d)))
    len += snprintf(buf + len, bSIZE - len, "Spell is not assigned to any classes.\r\n");

  if ((len < bSIZE) && !OLC_SPELL(d)->effectiveness)
    len += snprintf(buf + len, bSIZE - len, "Spell effectiveness is not set.\r\n");

  if ((len < bSIZE) && OLC_SPELL(d)->damages && !(OLC_SPELL(d)->mag_flags & MAG_DAMAGE))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: MAG_DAMAGE is required. (Damages is set).\r\n");

  if ((len < bSIZE) && OLC_SPELL(d)->damages && !(OLC_SPELL(d)->mag_flags & MAG_VIOLENT))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: MAG_VIOLENT is required. (Damages is set).\r\n");

  if ((len < bSIZE) && is_apply_set(OLC_SPELL(d)) && !(OLC_SPELL(d)->mag_flags & MAG_AFFECTS))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: MAG_AFFECTS is required. (Affects and applies are set).\r\n");

  if ((len < bSIZE) && (OLC_SPELL(d)->mag_flags & MAG_AFFECTS) && (OLC_SPELL(d)->mag_flags & MAG_UNAFFECTS))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: AFFECTS and UNAFFECTS are both set.\r\n");

  if ((len < bSIZE) && is_objects_set(OLC_SPELL(d)) && !(OLC_SPELL(d)->mag_flags & MAG_CREATIONS))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: MAG_CREATIONS is required. (Create objects is set).\r\n");

  if ((len < bSIZE) && is_dispel_set(OLC_SPELL(d)) && !(OLC_SPELL(d)->mag_flags & MAG_UNAFFECTS))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: MAG_UNAFFECTS is required. (Dispell is set).\r\n");
  
  if ((len < bSIZE) && is_points_set(OLC_SPELL(d)) && !(OLC_SPELL(d)->mag_flags & MAG_POINTS))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: MAG_POINTS is required. (Points is set).\r\n");

  if ((len < bSIZE) && is_summon_set(OLC_SPELL(d)) && !(OLC_SPELL(d)->mag_flags & MAG_SUMMONS))
    len += snprintf(buf + len, bSIZE - len, "Magic flags: MAG_SUMMON is required. (Summon mobile is set).\r\n");

  if ((len < bSIZE) && OLC_SPELL(d)->damages && !OLC_SPELL(d)->max_dam)
    len += snprintf(buf + len, bSIZE - len, "Damages is set, but max damages is set to 0.\r\n");

  if ((len < bSIZE) && !OLC_SPELL(d)->targ_flags)
    len += snprintf(buf + len, bSIZE - len, "No target flags set.\r\n");

  if (*buf)
    send_to_char (d->character, "\r\n%s", buf);

  if (len >= bSIZE) {
    send_to_char (d->character, " *** OVERFLOW ***\r\n");
    log("SYSERR: buffer overflow from spedit_show_warnings.");
  }
}

void spedit_show_messages(struct descriptor_data *d) {
  char buf[2048];

  snprintf (buf, sizeof(buf), "\r\n%s1%s) Wear off  : %s%s\r\n"
                              "%s2%s) To self   : %s%s\r\n"
                              "%s3%s) To victim : %s%s\r\n"
                              "%s4%s) To room   : %s%s\r\n" 
                              "\r\n%sEnter choice (0 to quit) : ",
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->messages.wear_off),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->messages.to_self),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->messages.to_vict),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->messages.to_room),
                              nrm);

  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_MESSAGES;
}

void spedit_show_objects(struct descriptor_data *d) {
  char buf[BUFSIZE];

  snprintf (buf, BUFSIZE, "\r\n%s1%s) Object : %s%s\r\n"
                              "%s2%s) Object : %s%s\r\n"
                              "%s3%s) Object : %s%s\r\n"
                              "\r\n%sEnter choice (0 to quit) : ",
                              grn, nrm, cyn,EMPTY_STR(OLC_SPELL(d)->objects[0]),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->objects[1]),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->objects[2]),
                              nrm);

  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_OBJECTS;
}

void spedit_show_dispel(struct descriptor_data *d) {
  char buf[BUFSIZE];

  snprintf (buf, BUFSIZE, "\r\n%s1%s) Spell : %s%s\r\n"
                              "%s2%s) Spell : %s%s\r\n"
                              "%s3%s) Spell : %s%s\r\n"
                              "\r\n%sEnter choice (0 to quit) : ",
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->dispel[0]),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->dispel[1]),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->dispel[2]),
                              nrm);

  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_DISPEL;
}

void spedit_show_points(struct descriptor_data *d) {
  char buf[BUFSIZE];
  
  snprintf (buf, BUFSIZE, "\r\n%s1%s) Hit points  : %s%s\r\n"
                              "%s2%s) Mana points : %s%s\r\n"
                              "%s3%s) Move points : %s%s\r\n"
                              "%s4%s) Gold        : %s%s\r\n"
                              "\r\n%sEnter choice (0 to quit) : ",
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->points.hp),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->points.mana),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->points.move),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->points.gold),
                              nrm);

  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_POINTS;
}

void spedit_show_mobile(struct descriptor_data *d) {
  char buf[BUFSIZE];

  snprintf (buf, BUFSIZE, "\r\n%s1%s) Mobile        : %s%s\r\n"
                              "%s2%s) Required item : %s%s\r\n"
                              "\r\n%sEnter choice (0 to quit) : ",
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->summon_mob),
                              grn, nrm, cyn, EMPTY_STR(OLC_SPELL(d)->summon_req),
                              nrm);

  send_to_char (d->character, "%s", buf);
  OLC_MODE(d) = SPEDIT_SHOW_MOBILE;
}

void spedit_main_menu (struct descriptor_data *d) {
  char buf[BUFSIZE];

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

  snprintf (buf, BUFSIZE, "%s-- %s Number      : [%s%5d%s] %s%s%s\r\n"
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
                "%sX%s) %sMenu -> Summon mobile\r\n"
                "%sS%s) %sMenu -> Script\r\n" 
                "%sC%s) %sMenu -> Classes\r\n"
                "%sM%s) %sMenu -> Messages\r\n"
                "%sW%s) Warnings\r\n"
                "%sQ%s) Quit\r\n\r\n"
                "%sEnter choice : ",
                 nrm, (Q->type == SPELL) ? "Spell" : "Skill", cyn, OLC_NUM(d), nrm, yel, Q->function ? "Special" : "", nrm,
                 prog ? red : grn, nrm, cyn, (Q->type == SPELL) ? "SPELL" : "SKILL", nrm,
                 grn, nrm, (Q->status == available) ? nrm : YEL, yel, (Q->status == available) ? "Available" : "Unavailable",
                 prog ? red : grn, nrm, yel, Q->name ? Q->name : UNDEF_SPELL, 
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

  SAFE_FREE(spell->points.hp);
  SAFE_FREE(spell->points.mana);
  SAFE_FREE(spell->points.move);
  SAFE_FREE(spell->points.gold);

  SAFE_FREE(spell->summon_mob);
  SAFE_FREE(spell->summon_req);

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

  for (i=0; i<MAX_SPELL_DISPEL; i++)
    SAFE_FREE(spell->dispel[i]);

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
  to->name = STRDUP(from->name);
  to->targ_flags = from->targ_flags;
  to->mag_flags = from->mag_flags;
  to->min_pos = from->min_pos;
  to->max_dam = from->max_dam;
  to->effectiveness = STRDUP(from->effectiveness);
  to->damages = STRDUP(from->damages);
  to->delay = STRDUP(from->delay);
  to->script = STRDUP(from->script);
  to->summon_mob = STRDUP(from->summon_mob);
  to->summon_req = STRDUP(from->summon_req);

  to->messages.wear_off = STRDUP(from->messages.wear_off);
  to->messages.to_self = STRDUP(from->messages.to_self);
  to->messages.to_vict = STRDUP(from->messages.to_vict);
  to->messages.to_room = STRDUP(from->messages.to_room);

  to->points.hp = STRDUP(from->points.hp);
  to->points.mana = STRDUP(from->points.mana);
  to->points.move = STRDUP(from->points.move);
  to->points.gold = STRDUP(from->points.gold);

  for (i=0; i<MAX_SPELL_PROTECTIONS; i++) {
    to->protfrom[i].prot_num = from->protfrom[i].prot_num;
    to->protfrom[i].duration = STRDUP(from->protfrom[i].duration);
    to->protfrom[i].resist = STRDUP(from->protfrom[i].resist);
  }

  for (i=0; i<MAX_SPELL_AFFECTS; i++) {
    to->applies[i].appl_num = from->applies[i].appl_num;
    to->applies[i].modifier = STRDUP(from->applies[i].modifier);
    to->applies[i].duration = STRDUP(from->applies[i].duration);
  }
 
  for (i=0; i<MAX_SPELL_OBJECTS; i++)
    to->objects[i] = STRDUP(from->objects[i]);

  for (i=0; i<MAX_SPELL_DISPEL; i++)
    to->dispel[i] = STRDUP(from->dispel[i]);

  for (i=0; i<NUM_CLASSES; i++) {
    to->assign[i].class_num = from->assign[i].class_num;
    to->assign[i].level = from->assign[i].level;
    to->assign[i].num_prac = STRDUP(from->assign[i].num_prac);
    to->assign[i].num_mana = STRDUP(from->assign[i].num_mana);
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

 if (spell->vnum > last_spell_vnum)
   last_spell_vnum = spell->vnum;

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
 spell->name     = NULL;
 spell->targ_flags = 0;
 spell->mag_flags = 0;
 spell->min_pos  = 0;
 spell->max_dam  = 0;
 spell->violent  = FALSE;
 spell->effectiveness = NULL;
 spell->damages  = NULL;
 spell->delay    = NULL;
 spell->script   = NULL;
 spell->summon_mob = NULL;
 spell->summon_req = NULL;

 spell->messages.wear_off = NULL;
 spell->messages.to_self = NULL;
 spell->messages.to_vict = NULL;
 spell->messages.to_room = NULL;

 spell->points.hp = NULL;
 spell->points.mana = NULL;
 spell->points.move = NULL;
 spell->points.gold = NULL;

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

 for (i=0; i<MAX_SPELL_DISPEL; i++)
   spell->dispel[i] = NULL;

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
 if (!OLC_NUM(d)) {
   while (SPELL_OLCING_BY(++vnum));

   if (vnum > MAX_SKILLS) {
     mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: Spedit: Skills and spells limits reached.");
     return 0;
   }

   OLC_NUM(d) = vnum;
 }

 CREATE (OLC_SPELL(d), struct str_spells, 1);
 OLC_SPELL(d)->vnum = OLC_NUM(d);

 if (q) 
   spedit_copyover_spell(q, OLC_SPELL(d));
 else {
   spedit_init_new_spell(OLC_SPELL(d));
   OLC_SPELL(d)->name = strdup(UNDEF_SPELL);
 }

 return 1;
}

void boot_spells (void)
{
 char buf[MAX_STRING_LENGTH + 1] = "";
 char buf1[MAX_STRING_LENGTH + 1] = "";

 FILE *fp;
 int  x, ret, fct, d1, err = 0, save = 0;
 struct str_spells *Q = NULL;

 if ((fp=fopen(SPELL_FILE, "r")) == NULL) {
    mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT: Can't boot spells.");
    return;
 }

 while (!feof(fp)) {
    ret = fscanf (fp, "%d ", &fct);
    if (!save && (fct != DB_CODE_INIT_VARS)) {
      mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: attemp to assign value to Q == (null)");
      abort();
    }
    switch (fct) {
      case DB_CODE_INIT_VARS : 
               if (save == 1) 
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
      case DB_CODE_NAME : 
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0'; 
                 Q->name = strdup (buf);
               }
               break;
      case DB_CODE_DAMAGES : 
              if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0'; 
                 Q->damages = strdup (buf);
               }
               break;
      case DB_CODE_MSG_WEAR_OFF : 
              if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.wear_off = strdup (buf);
               }
               break;
      case DB_CODE_MSG_TO_SELF : 
              if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.to_self = strdup (buf);
               }
               break;
      case DB_CODE_MSG_TO_VICT : 
              if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.to_vict = strdup (buf);
               }
               break;
      case DB_CODE_MSG_TO_ROOM : 
              if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->messages.to_room = strdup (buf);
              }
              break;
      case DB_CODE_SUMMON_MOB:
              if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->summon_mob = strdup (buf);
              }
              break;
      case DB_CODE_SUMMON_REQ:
              if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->summon_req = strdup (buf);
              }
              break;
      case DB_CODE_DISPEL_1 : 
      case DB_CODE_DISPEL_2 : 
      case DB_CODE_DISPEL_3 : 
               x = fct - DB_CODE_DISPEL_1;
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 buf[strlen(buf)-1] = '\0';
                 Q->dispel[x] = strdup(buf);
               }
               break;
      case DB_CODE_OBJECTS_1 : 
      case DB_CODE_OBJECTS_2 : 
      case DB_CODE_OBJECTS_3 : 
               x = fct - DB_CODE_OBJECTS_1;
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->objects[x] = strdup (buf);
                }
                break;
      case DB_CODE_PTS_HP :
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->points.hp = strdup (buf);
                }
                break;
      case DB_CODE_PTS_MANA :
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->points.mana = strdup (buf);
                }
                break;
      case DB_CODE_PTS_MOVE :
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->points.move = strdup (buf);
                }
                break;
      case DB_CODE_PTS_GOLD :
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0';
                  Q->points.gold = strdup (buf);
                }
                break;
      case DB_CODE_PROT_1 :
      case DB_CODE_PROT_2 :
      case DB_CODE_PROT_3 :
      case DB_CODE_PROT_4 :
      case DB_CODE_PROT_5 :
      case DB_CODE_PROT_6 :
               x = fct - DB_CODE_PROT_1;
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  ret = sscanf (buf, "%d %[^\n]", &Q->protfrom [x].prot_num, buf1);
                  if (ret == 2) Q->protfrom [x].duration = strdup(buf1);
               }
               if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                 ret = sscanf (buf, "%d %[^\n]", &d1, buf1);
                 if (d1 != DB_CODE_MARKER)
                   log("SYSERR: boot spells: Invalid marker in DB_CODE_PROT_%d", x + 1); 
                 if (ret == 2) Q->protfrom [x].resist = strdup(buf1); 
               } 
               break;
      case DB_CODE_AFFECTS_1 :
      case DB_CODE_AFFECTS_2 :
      case DB_CODE_AFFECTS_3 :
      case DB_CODE_AFFECTS_4 :
      case DB_CODE_AFFECTS_5 :
      case DB_CODE_AFFECTS_6 :
                x = fct - DB_CODE_AFFECTS_1;
                if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  ret = sscanf (buf, "%d %[^\n]", &Q->applies[x].appl_num, buf1);
                  if (ret == 2) Q->applies[x].modifier = strdup(buf1);
                }
                if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  ret = sscanf (buf, "%d %[^\n]", &d1, buf1);
                  if (d1 != DB_CODE_MARKER)
                    log("SYSERR: boot spells: Invalid marker in DB_CODE_AFFECTS_%d", x + 1); 
                  if (ret == 2) Q->applies[x].duration = strdup (buf1);
                } 
                break;
      case DB_CODE_CLASS_MU :
      case DB_CODE_CLASS_CL :
      case DB_CODE_CLASS_TH :
      case DB_CODE_CLASS_WA :
                x = fct - DB_CODE_CLASS_MU;
                if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  ret = sscanf (buf, "%d %d %[^\n]", &Q->assign [x].class_num, &Q->assign [x].level, buf1);
                  if (ret == 3) Q->assign [x].num_prac = strdup (buf1);
                  if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                    ret = sscanf (buf, "%d %[^\n]", &d1, buf1);
                    if (d1 != DB_CODE_MARKER)
                      log("SYSERR: boot spells: Invalid marker in DB_CODE_CLASS: %d", x + 1); 
                    if (ret == 2) Q->assign[x].num_mana = strdup (buf1);
                  }
                }
                break;
      case DB_CODE_EFFECTIVENESS : 
                if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0'; 
                  Q->effectiveness = strdup (buf);
                }
                break; 
      case DB_CODE_DELAY : 
                if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  buf[strlen(buf)-1] = '\0'; 
                  Q->delay = strdup (buf);
                }
                break;
      case DB_CODE_SCRIPT : 
                if (fgets (buf, MAX_STRING_LENGTH, fp)) {
                  if (!Q->script)
                    Q->script = strdup (buf); 
                  else {
                    snprintf (buf1, BUFSIZE, "%s%s", Q->script, buf); 
                    if (strlen(buf1) == BUFSIZE) 
                      log("SYSERR: boot spells: spell script buffer overflow"); 
                    free (Q->script);
                    Q->script = strdup (buf1);
                  } 
                }   
                break;
      case DB_CODE_END : break;    
      default : if (err++ > 9) {
                  log ("SYSERR: BOOT SPELLS: program abort too much errors.");
                  abort();
                } else
                    log ("SYSERR: BOOT SPELLS: invalide code in database.");
    } 
 }

 if (save == 1)
   spedit_save_internally (Q); 
 else
   mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: BOOT SPELLS: No spells available!");
}

void spedit_save_to_disk (void)
{
 char buf[BUFSIZE] = "";  
 char buf1[BUFSIZE] = "";
 char outbuf[MAX_STRING_LENGTH] = "";

 FILE *fp;
 struct str_spells *r;
 char *p;

 sprintf (buf, "cp %s %s.bak", SPELL_FILE, SPELL_FILE);
 if (system (buf) == -1) {
   mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: SPEDIT: Failed to create spell file backup.");
 }

 if ((fp=fopen(SPELL_FILE, "w")) == NULL) {
   mudlog (BRF, LVL_BUILDER, TRUE, "SYSERR: SPEDIT: Can't save spells to the database.");
   return;
 }

 setbuf(fp, outbuf);

 for (r = list_spells; r; r = r->next) {
   snprintf (buf, BUFSIZE, "%2d %c %d %d %d %d %d %d\n", DB_CODE_INIT_VARS,
                            r->type, r->vnum, r->status, r->targ_flags, r->mag_flags, r->min_pos, r->max_dam);
   fprintf(fp, "%s", buf);
 
   if (r->name) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_NAME, r->name);
     fprintf(fp, "%s", buf);
   }

   if (r->damages) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_DAMAGES, r->damages);
     fprintf(fp, "%s", buf);
   }

   if (r->messages.wear_off) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_MSG_WEAR_OFF, r->messages.wear_off);
     fprintf(fp, "%s", buf);
   }

   if (r->messages.to_self) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_MSG_TO_SELF, r->messages.to_self);
     fprintf(fp, "%s", buf);
   }

   if (r->messages.to_vict) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_MSG_TO_VICT, r->messages.to_vict);
     fprintf(fp, "%s", buf);
   }

   if (r->messages.to_room) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_MSG_TO_ROOM, r->messages.to_room);
     fprintf(fp, "%s", buf);
   }

   if (r->summon_mob) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_SUMMON_MOB, r->summon_mob);
     fprintf(fp, "%s", buf);
   }

   if (r->summon_req) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_SUMMON_REQ, r->summon_req);
     fprintf(fp, "%s", buf);
   }

   if (r->dispel[0]) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_DISPEL_1, r->dispel[0]);
     fprintf(fp, "%s", buf);
   }

   if (r->dispel[1]) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_DISPEL_2, r->dispel[1]);
     fprintf(fp, "%s", buf);
   }

   if (r->dispel[2]) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_DISPEL_3, r->dispel[2]);
     fprintf(fp, "%s", buf);
   }

   if (r->objects[0]) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_OBJECTS_1, r->objects[0]);
     fprintf(fp, "%s", buf);
   }

   if (r->objects[1]) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_OBJECTS_2, r->objects[1]);
     fprintf(fp, "%s", buf);
   }

   if (r->objects[2]) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_OBJECTS_3, r->objects[2]);
     fprintf(fp, "%s", buf);
   }

   if (r->points.hp) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_PTS_HP, r->points.hp);
     fprintf(fp, "%s", buf);
   }

   if (r->points.mana) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_PTS_MANA, r->points.mana);
     fprintf(fp, "%s", buf);
   }

   if (r->points.move) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_PTS_MOVE, r->points.move);
     fprintf(fp, "%s", buf);
   }
   if (r->points.gold) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_PTS_GOLD, r->points.gold);
     fprintf(fp, "%s", buf);
   }

   if (r->protfrom[0].prot_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_PROT_1, 
                              r->protfrom[0].prot_num, r->protfrom[0].duration, DB_CODE_MARKER, r->protfrom[0].resist);
     fprintf(fp, "%s", buf);
   }

   if (r->protfrom[1].prot_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_PROT_2, 
                              r->protfrom[1].prot_num, r->protfrom[1].duration, DB_CODE_MARKER, r->protfrom[1].resist);
     fprintf(fp, "%s", buf);
   }

   if (r->protfrom[2].prot_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_PROT_3,
                              r->protfrom[2].prot_num, r->protfrom[2].duration, DB_CODE_MARKER, r->protfrom[2].resist);
     fprintf(fp, "%s", buf);
   }

   if (r->protfrom[3].prot_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_PROT_4, 
                              r->protfrom[3].prot_num, r->protfrom[3].duration, DB_CODE_MARKER, r->protfrom[3].resist);
     fprintf(fp, "%s", buf);
   }

   if (r->protfrom[4].prot_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_PROT_5, 
                              r->protfrom[4].prot_num, r->protfrom[4].duration, DB_CODE_MARKER, r->protfrom[4].resist);
     fprintf(fp, "%s", buf);
   }

   if (r->protfrom[5].prot_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_PROT_6, 
                              r->protfrom[5].prot_num, r->protfrom[5].duration, DB_CODE_MARKER, r->protfrom[5].resist);
     fprintf(fp, "%s", buf);
   }

   if (r->applies[0].appl_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_AFFECTS_1, 
                              r->applies[0].appl_num, NULL_STR(r->applies[0].modifier), DB_CODE_MARKER, r->applies[0].duration);
     fprintf(fp, "%s", buf);
   }

   if (r->applies[1].appl_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_AFFECTS_2, 
                              r->applies[1].appl_num, NULL_STR(r->applies[1].modifier), DB_CODE_MARKER, r->applies[1].duration);
     fprintf(fp, "%s", buf);
   }

   if (r->applies[2].appl_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_AFFECTS_3, 
                              r->applies[2].appl_num, NULL_STR(r->applies[2].modifier), DB_CODE_MARKER, r->applies[2].duration);
     fprintf(fp, "%s", buf);
   }

   if (r->applies[3].appl_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_AFFECTS_4, 
                              r->applies[3].appl_num, NULL_STR(r->applies[3].modifier), DB_CODE_MARKER, r->applies[3].duration);
     fprintf(fp, "%s", buf);
   }

   if (r->applies[4].appl_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_AFFECTS_5, 
                              r->applies[4].appl_num, NULL_STR(r->applies[4].modifier), DB_CODE_MARKER, r->applies[4].duration);
     fprintf(fp, "%s", buf);
   }

   if (r->applies[5].appl_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %s\n%2d %s\n", DB_CODE_AFFECTS_6, 
                              r->applies[5].appl_num, NULL_STR(r->applies[5].modifier), DB_CODE_MARKER, r->applies[5].duration);
     fprintf(fp, "%s", buf);
   }

   if (r->assign[0].class_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %d %s\n%2d %s\n", DB_CODE_CLASS_MU, 
                              r->assign[0].class_num, r->assign[0].level, NULL_STR(r->assign[0].num_prac), DB_CODE_MARKER, NULL_STR(r->assign[0].num_mana));
     fprintf(fp, "%s", buf);
   }

   if (r->assign[1].class_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %d %s\n%2d %s\n", DB_CODE_CLASS_CL, 
                              r->assign[1].class_num, r->assign[1].level, NULL_STR(r->assign[1].num_prac), DB_CODE_MARKER, NULL_STR(r->assign[1].num_mana));
     fprintf(fp, "%s", buf);
   }

   if (r->assign[2].class_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %d %s\n%2d %s\n", DB_CODE_CLASS_TH, 
                              r->assign[2].class_num, r->assign[2].level, NULL_STR(r->assign[2].num_prac), DB_CODE_MARKER, NULL_STR(r->assign[2].num_mana));
     fprintf(fp, "%s", buf);
   }

   if (r->assign[3].class_num != -1) {
     snprintf (buf, BUFSIZE, "%2d %d %d %s\n%2d %s\n", DB_CODE_CLASS_WA, 
                              r->assign[3].class_num, r->assign[3].level, NULL_STR(r->assign[3].num_prac), DB_CODE_MARKER, NULL_STR(r->assign[3].num_mana));
     fprintf(fp, "%s", buf);
   }

   if (r->effectiveness) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_EFFECTIVENESS, r->effectiveness);
     fprintf(fp, "%s", buf);
   }

   if (r->delay) {
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_DELAY, r->delay);
     fprintf(fp, "%s", buf);
   }

   if (r->script) {
     strncpy (buf1, r->script, BUFSIZE); 
     p = strtok (buf1, "\n");
     snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_SCRIPT, p);
     fprintf(fp, "%s", buf);
     while ((p=strtok('\0', "\n"))) {
       snprintf (buf, BUFSIZE, "%2d %s\n", DB_CODE_SCRIPT, p);
       fprintf(fp, "%s", buf);
     }
   }
 }
 fprintf (fp, "%2d\n", DB_CODE_END);
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
 char buf[BUFSIZE];
 char *str;
 const char *name;
 int vnum;
 
 OLC_NUM(d) = 0;

 if (OLC_STORAGE(d)) {
   if (is_number(OLC_STORAGE(d)))
     vnum = atoi(OLC_STORAGE(d));
   else
     vnum = olc_spell_by_name(d, OLC_STORAGE(d));

   // we don't allow to edit spell vnum 0 anyway.
   if (!vnum) {
     send_to_char (d->character, "that spell could not be found!\r\n");
     cleanup_olc (d, CLEANUP_ALL);
     return 0;
   } else {
       if (vnum < 0) {
         send_to_char (d->character, "The spell (vnum: %d) must be above 0.\r\n", vnum);
         cleanup_olc (d, CLEANUP_ALL);
         return 0;
       }

       if (vnum > last_spell_vnum) {
         send_to_char (d->character, "This spell (vnum: %d) is out of bound. Try 'spedit' to create a new spell.\r\n", vnum);
         cleanup_olc (d, CLEANUP_ALL);
         return 0;
       } 

       name = get_spell_name(vnum);

       if ((str = SPELL_OLCING_BY (vnum))) {
         sprintf (buf, "This spell '%s' (vnum: %d) is already edited by %s.\r\n", name, vnum, str);
         send_to_char (d->character, "%s", buf);  
         cleanup_olc (d, CLEANUP_ALL);
         return 0;
       }

       OLC_NUM(d) = vnum;

       if (name != UNDEF_SPELL) {
         sprintf (buf, "Do you want to edit '%s' (vnum: %d)? (y/n%s): ", name, vnum, 
                       OLC_SEARCH(d) ? ", q" : "");
         send_to_char (d->character, "%s", buf);  
         OLC_MODE(d) = SPEDIT_CONFIRM_EDIT;
         return 1;
       }
     }
 }

 spedit_setup2(d);
 return 1;
}

void spedit_parse (struct descriptor_data *d, char *arg) {
  const char *points[] = { "Hit points += ",
                           "Mana points += ",
                           "Move points += ",
                           "Gold += " }; 
  char buf[BUFSIZE];

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
                               (arg && *arg) ? strdup(arg) : strdup(UNDEF_SPELL);
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
                                  if (!get_spell_by_vnum(vnum)) {
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
         delete_doubledollar(arg);
         SAFE_FREE(OLC_SPELL(d)->messages.wear_off);
         OLC_SPELL(d)->messages.wear_off = (arg && *arg) ? strdup(arg) : NULL;
         spedit_show_messages (d);
         return;
    case SPEDIT_GET_MSG_TO_SELF :
         delete_doubledollar(arg);
         SAFE_FREE(OLC_SPELL(d)->messages.to_self);
         OLC_SPELL(d)->messages.to_self = (arg && *arg) ? strdup(arg) : NULL;
         spedit_show_messages (d);
         return;
    case SPEDIT_GET_MSG_TO_VICT :
         delete_doubledollar(arg);
         SAFE_FREE(OLC_SPELL(d)->messages.to_vict);
         OLC_SPELL(d)->messages.to_vict = (arg && *arg) ? strdup(arg) : NULL;
         spedit_show_messages (d);
         return;
    case SPEDIT_GET_MSG_TO_ROOM :
         delete_doubledollar(arg);
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
         if ((x < 0) || (x > NUM_CHAR_POSITION)) {
           send_to_char (d->character, "Invalid choice!\r\n");
           spedit_minpos_menu (d);
           return;
         }   
         else
           OLC_SPELL(d)->min_pos = x - 1;
         break;   
    case SPEDIT_SHOW_TARG_FLAGS : 
         if (!(x = atoi (arg))) break;
         if ((x < 0) || (x > NUM_SPELL_FLAGS)) 
           send_to_char (d->character, "Invalid choice!\r\n");
         else
           OLC_SPELL(d)->targ_flags ^= (1 << (x - 1));
         spedit_targ_flags_menu (d);
         return;
    case SPEDIT_SHOW_MAG_FLAGS:
         if (!(x = atoi (arg))) break;
         if ((x < 0) || (x > NUM_MAG)) 
           send_to_char (d->character, "Invalid choice!\r\n");
         else
           OLC_SPELL(d)->mag_flags ^= (1 << (x - 1));
         spedit_mag_flags_menu (d);
         return;
    case SPEDIT_PROTECTION_MENU :
         if (!(x = atoi (arg))) break;
         if ((x < 0) || (x > MAX_SPELL_PROTECTIONS)) {
           send_to_char (d->character, "Invalid choice!\r\n");
           spedit_protection_menu (d);
           return;
         }
         OLC_VAL(d) = x - 1;
         send_to_char (d->character, "Spell VNUM (0 to quit, 'r' to remove) : ");
         OLC_MODE(d) = SPEDIT_GET_SPELL_NUM;
         return;   
    case SPEDIT_GET_OBJECT : if (*arg) 
                               value = formula_interpreter (d->character,
                                          d->character, OLC_NUM(d), FALSE,
                                          arg, &rts_code);
                             if (!*arg || !rts_code) {
                               SAFE_FREE(OLC_SPELL(d)->objects[OLC_VAL(d)]); 
                               OLC_SPELL(d)->objects[OLC_VAL(d)] = NULL;

                               if (*arg) 
                                 OLC_SPELL(d)->objects[OLC_VAL(d)] = strdup(arg);
                             }
                             spedit_show_objects(d);
                             return;
    case SPEDIT_GET_DISPEL : if (*arg) 
                               value = formula_interpreter (d->character,
                                          d->character, OLC_NUM(d), FALSE,
                                          arg, &rts_code);
                             if (!*arg || !rts_code) {
                               SAFE_FREE(OLC_SPELL(d)->dispel[OLC_VAL(d)]); 
                               OLC_SPELL(d)->dispel[OLC_VAL(d)] = NULL;

                               if (*arg)
                                 OLC_SPELL(d)->dispel[OLC_VAL(d)] = strdup(arg);
                             }
                             spedit_show_dispel(d);
                             return;
    case SPEDIT_GET_POINTS : if (*arg) 
                                value = formula_interpreter (d->character,
                                          d->character, OLC_NUM(d), FALSE,
                                          arg, &rts_code);
                             if (!arg || !rts_code) {
		               switch (OLC_VAL(d)) {
                                  case 0 : SAFE_FREE(OLC_SPELL(d)->points.hp);
                                           OLC_SPELL(d)->points.hp = NULL;
                                           if (*arg) OLC_SPELL(d)->points.hp = strdup(arg);
                                           break;
                                  case 1 : SAFE_FREE(OLC_SPELL(d)->points.mana);
                                           OLC_SPELL(d)->points.mana = NULL;
                                           if (*arg) OLC_SPELL(d)->points.mana = strdup(arg);
                                           break;
                                  case 2 : SAFE_FREE(OLC_SPELL(d)->points.move);
                                           OLC_SPELL(d)->points.move = NULL;
                                           if (*arg) OLC_SPELL(d)->points.move = strdup(arg);
                                           break;
                                  case 3 : SAFE_FREE(OLC_SPELL(d)->points.gold);
                                           OLC_SPELL(d)->points.gold = NULL;
                                           if (*arg) OLC_SPELL(d)->points.gold = strdup(arg);
                               } 
                             } 
                             spedit_show_points (d);
                             return;
    case SPEDIT_GET_MOBILE : if (*arg)
                               value = formula_interpreter (d->character,
                                          d->character, OLC_NUM(d), FALSE,
                                          arg, &rts_code);
                             if (!arg || !rts_code) {
                               switch (OLC_VAL(d)) {
                                 case 0 : SAFE_FREE(OLC_SPELL(d)->summon_mob);
                                          OLC_SPELL(d)->summon_mob = NULL;
                                          if (*arg) OLC_SPELL(d)->summon_mob = strdup(arg);
                                          break;
                                 case 1 : SAFE_FREE(OLC_SPELL(d)->summon_req);
                                          OLC_SPELL(d)->summon_req = NULL;
                                          if (*arg) OLC_SPELL(d)->summon_req = strdup(arg);
                               }
                             }
                             spedit_show_mobile (d);
                             return;
    case SPEDIT_SHOW_MOBILE :
         if (!(x = atoi(arg))) break;
         if ((x < 0) || (x > 2)) {
           send_to_char (d->character, "Invalid choice!\r\n");
           spedit_show_mobile(d);
           return;
         }
         if (x == 1) send_to_char(d->character, "Mobile : ");
         else        send_to_char(d->character, "Item   : ");
         OLC_VAL(d) = x - 1;
         OLC_MODE(d) = SPEDIT_GET_MOBILE;
         return;
    case SPEDIT_SHOW_OBJECTS :
         if (!(x = atoi(arg))) break;
         if ((x < 0) || (x > MAX_SPELL_OBJECTS)) {
           send_to_char (d->character, "Invalid choice!\r\n");
           spedit_show_objects(d);
           return;
         }
         send_to_char(d->character, "Object #%d : ", x);
         OLC_VAL(d) = x - 1;
         OLC_MODE(d) = SPEDIT_GET_OBJECT; 
         return;
    case SPEDIT_SHOW_DISPEL :
         if (!(x = atoi(arg))) break;
         if ((x < 0) || (x > MAX_SPELL_DISPEL)) {
           send_to_char (d->character, "Invalid choice!\r\n");
           spedit_show_dispel(d);
           return;
         }
         send_to_char(d->character, "Dispel #%d : ", x);
         OLC_VAL(d) = x - 1;
         OLC_MODE(d) = SPEDIT_GET_DISPEL; 
         return;
    case SPEDIT_SHOW_POINTS :
         if (!(x = atoi(arg))) break;
         if ((x < 0) || (x > 4)) {
           send_to_char (d->character, "Invalid choice!\r\n");
           spedit_show_points (d);
           return;
         }
         send_to_char(d->character, "%s", points[x-1]);
         OLC_VAL(d) = x - 1;
         OLC_MODE(d) = SPEDIT_GET_POINTS;
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
          case '9' : spedit_show_points (d);
                     return;
          case 'p' :
          case 'P' : spedit_protection_menu (d);
                     return; 
          case 'a' :
          case 'A' : spedit_apply_menu (d);
                     return;
          case 'd' :
          case 'D' : spedit_show_dispel (d);
                     return;
          case 'o' :
          case 'O' : spedit_show_objects (d);
                     return;
          case 'x' :
          case 'X' : spedit_show_mobile (d);
                     return;
          case 's' :
          case 'S' : page_string (d, OLC_SPELL(d)->script, 1);
                     d->backstr  = STRDUP(OLC_SPELL(d)->script);
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
    OLC_STORAGE(d) = NULL;
 
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
 int search_not_assigned = 0;
 int search_by_skill = 0;
 size_t len = 0, tmp_len = 0;

 struct str_spells *ptr;
 
 skip_spaces(&argument);

 if (!*argument) {
   send_to_char(ch, "splist all - list all spells\r\n"
                    "splist sk  - list all skills\r\n"
                    "splist mu  - list all spells in class magical user\r\n" 
                    "splist cl  - list all spells in class cleric\r\n" 
                    "splist th  - list all spells in class thief\r\n" 
                    "splist wa  - list all spells in class warrior\r\n" 
                    "splist not - list all spells not assigned\r\n"
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
 if(!strcmp(argument, "sk"))
   search_by_skill = 1;
 else
 if(!strcmp(argument, "off")) 
   search_disabled = 1;
 else
 if(!strcmp(argument, "not"))
   search_not_assigned = 1;
 else
 if(strcmp(argument, "all"))
   search_by_part_name = 1;


 len = snprintf(buf, MAX_STRING_LENGTH, 
 "Index VNum    Name                 Type     Spec_Prog    Available   Classe(s)\r\n"
 "----- ------- ----                 ----     ---------    ---------   -----------\r\n");
 
 for (ptr = list_spells; ptr; ptr = ptr->next) {
   char classes[80] = "";
   int mu, cl, th, wa;

   if ((mu = (get_spell_class(ptr, CLASS_MAGIC_USER) != -1))) strcat(classes, "Mu ");
   if ((search_by_class == CLASS_MAGIC_USER) && !mu) continue;

   if ((cl = (get_spell_class(ptr, CLASS_CLERIC) != -1))) strcat(classes, "Cl ");
   if ((search_by_class == CLASS_CLERIC) && !cl) continue;

   if ((th = (get_spell_class(ptr, CLASS_THIEF) != -1))) strcat(classes, "Th ");
   if ((search_by_class == CLASS_THIEF) && !th) continue;

   if ((wa = (get_spell_class(ptr, CLASS_WARRIOR) != -1))) strcat(classes, "Wa");
   if ((search_by_class == CLASS_WARRIOR) && !wa) continue;

   if (search_disabled && (ptr->status == available)) continue;

   if ((mu || cl || th || wa) && search_not_assigned) continue;

   if (search_by_skill && (ptr->type == SPELL)) continue;

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


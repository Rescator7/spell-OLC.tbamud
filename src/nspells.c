#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "utils.h"
#include "interpreter.h"
#include "oasis.h"
#include "spedit.h"
#include "spells.h"
#include "nspells.h" 
#include "code.h"
#include "modify.h"
#include "db.h"

extern char* NOEFFECT;

void save_plrspells (struct char_data *ch)
{
 char buf[2048];

 FILE *fp;
 struct str_plrspells *Q;
 extern struct str_spells *list_spells;
 struct str_spells *ptr;

 if (!get_filename (buf, sizeof(buf), PLR_SPELLS_FILE, GET_NAME(ch))) { 
   log ("SYSERR: can't get a filename for a spell_file");
   return;
 }
 if (!(fp = fopen(buf, "w"))) {
   log ("SYSERR: can't save a spell_file");
   return;
 }
 for (ptr = list_spells; ptr; ptr = ptr->next) {
   for (Q = ch->plrspells; Q; Q = Q->next) 
     if (Q->serial == ptr->serial) {
       fprintf (fp, "%d %d\n", Q->serial, Q->num_prac);
       break;
     }
   if (!Q)  
     fprintf (fp, "%d 0\n", ptr->serial); 
 } 
 fflush (fp);
 fclose (fp);
}

void load_plrspells (struct char_data *ch) 
{
 char buf[2048];
 int ret;

 FILE *fp;
 struct str_plrspells *plrspells = NULL, *prev;

 if (!get_filename (buf, sizeof(buf), PLR_SPELLS_FILE, GET_NAME(ch))) { 
   log ("SYSERR: can't get a filename for a spell_file");
   return;
 }
 if (!(fp = fopen(buf, "r"))) 
   return;
 while (!feof(fp)) {
   prev = plrspells;
   CREATE (plrspells, struct str_plrspells, 1);
   ret = fscanf (fp, "%d %d\n", &plrspells->serial, &plrspells->num_prac); 
   if (prev)
     prev->next = plrspells;
   else
     ch->plrspells = plrspells;
   plrspells->next = NULL;  
 }
 fclose (fp);
}

int find_spell_assign (struct char_data *ch, struct str_spells *ptr)
{
 int i;
 
 for (i=0; i<6; i++)
   if ( ((ptr->assign[i].class_num  < NUM_CLASSES)    && 
         (ptr->assign[i].class_num == GET_CLASS(ch))) ) // ||
//        ((ptr->assign[i].class_num >= NUM_CLASSES)    &&
//         ((ptr->assign[i].class_num - NUM_CLASSES)    == GET_ALIGN_TYPE(ch))) ) 
    return i;
 return -1;
}

int IS_SPELL_LEARNED (struct char_data *ch, int serial) {
  struct str_plrspells *Q;
  struct str_spells *ptr;
  int x, rts_code;

  for (ptr = list_spells; ptr && (ptr->serial != serial); ptr = ptr->next);
  if (!ptr) {
    log("SYSERR: (IS_SPELL_LEARNED): spells %d doesn't exist!", serial);
    return FALSE;
  }
  for (Q = ch->plrspells; Q; Q = Q->next)
    if (Q->serial == serial) {
      if ((x = find_spell_assign (ch, ptr)) == -1)  
        return FALSE; 
      if (Q->num_prac >= formula_interpreter (ch, ch, serial, TRUE, ptr->assign[x].num_prac, 
                                              &rts_code))
        return TRUE;
    }
  return FALSE;   
}

ACMD(do_listspells)
{
 char buf[2048];
 char buf1[2048];

 int cpt = 0, x;
 struct str_spells *i;

 if (cmd == 1) {
   if (!GET_PRACTICES(ch))
     strcpy(buf, "You have no practice sessions remaining.\r\n");
   else
     sprintf(buf, "You have %d practice session%s remaining.\r\n", GET_PRACTICES(ch),
    (GET_PRACTICES(ch) == 1 ? "" : "s"));  
   sprintf(buf, "%sYou know of the following spells and skills:\r\n", buf);
   send_to_char (ch, "%s", buf);
 }

 buf[0] = '\0';
 for (i=list_spells; i; i = i->next)
   if (i && i->name) {
     if (GET_LEVEL(ch) >= LVL_IMMORT) {
       cpt++;
       sprintf (buf1, "<%s:%5d> [%c%c%c%c] ", 
                      (i->type == SKILL) ? "SKL" : "SPL",
                      i->serial,
                      (i->status == available) ? 'A' : 'a',
                      i->function              ? 'F' : 'f', 
                      i->script                ? 'S' : 's',
                      i->damages               ? 'D' : 'd');
       sprintf (buf, "%s%s%s\r\n", buf, buf1, i->name);
     } else 
        if ((i->status == available) && ((x = find_spell_assign (ch, i)) != -1))
          sprintf (buf, "%s%-22s (%s)\r\n", buf, i->name, IS_SPELL_LEARNED (ch, i->serial) ?
                        "superb" : (i->assign[x].level > GET_LEVEL(ch)) ? "advance first" : 
                        "not learned");
   }
 if (GET_LEVEL(ch) >= LVL_IMMORT)
   sprintf (buf, "%s &g%d &nspell%s found!\r\n", buf, cpt, cpt > 1 ? "s" : "");
 page_string (ch->desc, buf, 1);
}

char *get_spell_wear_off (int serial) 
{
 struct str_spells *Q;

 for (Q = list_spells; Q; Q=Q->next)
   if (Q->serial == serial)
     return (Q->wear_off);
 return NULL;
}

void free_player_spells (struct char_data *ch)
{
  struct str_plrspells *Q = NULL, *next;
  
  for (Q = ch->plrspells; Q; Q = next) {
    next = Q->next;
    free (Q);
  }
  ch->plrspells = NULL;
}

void assign_spells (void) 
{
 void spedit_init_new_spell  (struct str_spells *spell);
 void spedit_save_internally (struct str_spells *spell);
 // ASPELL(spell_gate);
 ASPELL(spell_create_water);
 ASPELL(spell_recall);
 ASPELL(spell_teleport);
 ASPELL(spell_summon);
 ASPELL(spell_locate_object);
 ASPELL(spell_charm);
 ASPELL(spell_identify);
 ASPELL(spell_enchant_weapon);
 ASPELL(spell_detect_poison);
 ACMD(do_backstab);
 ACMD(do_bash);
 ACMD(do_rescue);
 ACMD(do_kick);
 ACMD(do_sneak);
 ACMD(do_track);
 ACMD(do_steal);
 ACMD(do_hide);

 int i = 0;
 struct str_spells *Q;
 struct str_spell_assign {
    char *name;
    void *function;
    char type;
    int  cmd, subcmd;
 } spell_assign[] = {
{"charm",          spell_charm         , SPELL, 0, 0},
{"create water",   spell_create_water  , SPELL, 0, 0},
{"detect poison",  spell_detect_poison , SPELL, 0, 0},
{"enchant weapon", spell_enchant_weapon, SPELL, 0, 0},
// {"gate",           spell_gate          , SPELL, 0, 0},
{"identify",       spell_identify      , SPELL, 0, 0},
{"locate object",  spell_locate_object , SPELL, 0, 0},
{"recall",         spell_recall        , SPELL, 0, 0},
{"summon",         spell_summon        , SPELL, 0, 0},
{"teleport",       spell_teleport      , SPELL, 0, 0},
{"backstab",       do_backstab         , SKILL, 1, 0},
{"bash",           do_bash             , SKILL, 1, 0},
{"hide",           do_hide             , SKILL, 1, 0},
{"kick",           do_kick             , SKILL, 1, 0},
{"rescue",         do_rescue           , SKILL, 1, 0},
{"sneak",          do_sneak            , SKILL, 1, 0},
{"steal",          do_steal            , SKILL, 1, 0},
{"track",          do_track            , SKILL, 0, 0},
{"",               NULL}
   };
 
 while (spell_assign[i].function) {
   CREATE(Q, struct str_spells, 1);
   spedit_init_new_spell (Q);
   free (Q->name);    /* spedit_init_new_spell will strdup("undefined"); */
   Q->name     = strdup (spell_assign[i].name);
   Q->type     = spell_assign[i].type;
   Q->function = spell_assign[i].function; 
   Q->serial   = ++i;
   Q->status   = available;
   spedit_save_internally (Q);
 }
}

void call_ASPELL (void (*function) (), int level, struct char_data *ch,
                  struct char_data *vict, struct obj_data *obj)
{
 (*function)(level, ch, vict, obj);
}

void call_ACMD (void (*function) (), struct char_data *ch, char *argument, int cmd, int subcmd)
{
 (*function)(ch, argument, cmd, subcmd);
}

ACMD(do_cast)
{
 char *s, *targ;
 static const char *wrong_pos[] = 
{ "You can't do much of anything like this!\r\n",
  "You can't do much of anything like this!\r\n",
  "You can't do much of anything like this!\r\n",
  "You can't do much of anything like this!\r\n",
  "You dream about great magical powers.\r\n",
  "You cannot concentrate while resting.\r\n",
  "You can't do this sitting!\r\n",
  "Impossible! You can't concentrate enough!\r\n",
  "Sorry, this spell isn't available!\r\n" };

 struct char_data *vict;
 struct obj_data *ovict = NULL;
 struct str_spells *spell;
 struct affected_type af;
 int i, delay, rts_code = TRUE;

 if (IS_NPC(ch))
   return;

 if (subcmd == SCMD_SPELL) {
   s = strtok(argument, "'");
   if (s == NULL) {
     send_to_char (ch, "Cast what where?\r\n");
     return;
   }
   if (!(s = strtok(NULL, "'"))) {
     send_to_char (ch, "Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
     return;
   }
   targ = strtok (NULL, "\0");
 }
 else // skill
   s = argument;

 for (spell = list_spells; spell; spell=spell->next) {
   if (isname(s, spell->name) && ((spell->status == available) || 
                                  (GET_LEVEL(ch) == LVL_IMPL)) )
     break;
 }
 if (!spell) {
   send_to_char (ch, "There's no such %s available to you.\r\n", (subcmd == SCMD_SKILL) ? "skill" : "spell");
   return;
 }
 if ((spell->status != available) ||
     ((spell->type == SKILL) && (subcmd != SCMD_SKILL))) {
   send_to_char (ch, "There's no such spell available to you.\r\n");
   return;
 }
 if ((spell->type == SPELL) && (subcmd != SCMD_SPELL)) {
   send_to_char (ch, "Huh?!?\r\n");
   return;
 }
 if ((GET_LEVEL(ch) < LVL_IMMORT) && !IS_SPELL_LEARNED (ch, spell->serial)) {
   send_to_char (ch, "You are unfamilliar with that %s.\r\n", (subcmd == SCMD_SKILL) ? "skill" : "spell");
   return;
 }
 if (GET_POS(ch) < spell->min_pos) {
   send_to_char (ch, "%s", wrong_pos[(int) GET_POS(ch)]);
   return;
 }
 if (!targ)
   vict = ch;
 else {
    skip_spaces (&targ);
    if (!(vict = get_char_room_vis (ch, targ, NULL))) {
      send_to_char (ch, "%s", NOPERSON);
      return;
    }
 }
 if ((vict == ch) && (!IS_SPELL_SELF(spell->flags)) && (!IS_SPELL_GROUP(spell->flags)) ) {
   send_to_char (ch, "You can't cast this spell upon yourself!\r\n");
   return;
 }
 if ((vict != ch) && (!IS_SPELL_VICT(spell->flags)) && (!IS_SPELL_VICTGRP(spell->flags)) 
                  && (!IS_SPELL_ROOM(spell->flags))) {
   send_to_char (ch, "You can't cast this spell on someone else!\r\n");
   return;
 }

 if (!spell->effectiveness || (rand_number(1, 100) > 
      formula_interpreter (ch, vict, spell->serial, TRUE, spell->effectiveness, &rts_code))){
   send_to_char (ch, "You lost your concentration!\r\n");
   return;
 }

 if (spell->delay) {
   delay = formula_interpreter (ch, vict, spell->serial, TRUE, spell->delay, 
                                &rts_code);
   WAIT_STATE (ch, (delay > MAX_SPELL_DELAY) ? MAX_SPELL_DELAY : delay);
 }

 if (affected_by_spell (vict, spell->serial) && (!IS_SPELL_ACCDUR(spell->flags)) &&
                                                (!IS_SPELL_ACCAFF(spell->flags))) {
   send_to_char (ch, "%s", NOEFFECT);
   rts_code = TRUE;
 }
 else
   for (i=0; i<6; i++) 
     if (spell->applies[i].appl_num != -1) {
       af.spell = spell->serial;
       af.duration = spell->applies[i].duration ? 
                     formula_interpreter (ch, vict, spell->serial, TRUE, spell->applies[i].duration, &rts_code) : 0;
       af.modifier = spell->applies[i].modifier ?  formula_interpreter (ch, vict, spell->serial, TRUE,
                                                   spell->applies[i].modifier, &rts_code) : 0; 
       af.bitvector[0] = spell->applies[i].appl_num < NUM_APPLIES ? 0 : 
                      (1 << (spell->applies[i].appl_num - NUM_APPLIES));
       af.location = spell->applies[i].appl_num < NUM_APPLIES ?  spell->applies[i].appl_num : APPLY_NONE;
       affect_join (vict, &af, IS_SPELL_ACCDUR(spell->flags), FALSE, IS_SPELL_ACCAFF(spell->flags), FALSE); 
       rts_code = TRUE; 
     }

 if (spell->function) 
   if ((spell->type == SPELL) && (subcmd == SCMD_SPELL)) {
     call_ASPELL (spell->function, GET_LEVEL(ch), ch, vict, ovict);
     rts_code = TRUE; 
   }
   else if ((spell->type == SKILL) && (subcmd == SCMD_SKILL)) {
          call_ACMD (spell->function, ch, argument, 0, 0);
          rts_code = TRUE;
        }
        else;
 else;

 if (spell->script)
   if (!perform_script (spell->script, ch, vict, ovict, spell->serial, 0) && !rts_code)
     send_to_char (ch, "%s", NOEFFECT);   
}


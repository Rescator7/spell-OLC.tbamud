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
#include "spells.h"

extern char* NOEFFECT;
ACMD(do_whirlwind);

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
 char buf[2048];   // TODO: fix me. crash buffer overflow
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
 ACMD(do_pick_lock);
 ACMD(do_whirwind);
 ACMD(do_bandage);

 int i = 0;
 struct str_spells *Q;
 struct str_spell_assign {
    char *name;
    void *function;
    char type;
    int  maxmana,
         minmana,
         manachng,
         min_pos,
         targ_flags,
         violent_flags,
         mag_flags;
    char *mesg_to_self;
 } spell_assign[] = {
{"backstab",       do_backstab         , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"bandage",        do_bandage          , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"bash",           do_bash             , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"hide",           do_hide             , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"kick",           do_kick             , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"pick lock",      NULL                , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"rescue",         do_rescue           , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"sneak",          do_sneak            , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"steal",          do_steal            , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"track",          do_track            , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"whirlwind",      do_whirlwind        , SKILL, 0, 0, 0, 0, 0, 0, 0, NULL},

{"charm",          spell_charm         , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"create water",   spell_create_water  , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"detect poison",  spell_detect_poison , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"enchant weapon", spell_enchant_weapon, SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"identify",       spell_identify      , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"locate object",  spell_locate_object , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"recall",         spell_recall        , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"summon",         spell_summon        , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},
{"teleport",       spell_teleport      , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL},

{"animate dead",   NULL                , SPELL, 35, 10, 3, POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, NULL},
{"armor",          NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS, 
                                         "You feel less protected."},
{"bless",          NULL                , SPELL, 35, 5, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, 
                                                                        MAG_AFFECTS | MAG_ALTER_OBJS,
	                                 "You feel less righteous."},
{"blindness",      NULL                , SPELL, 35, 25, 1, POS_STANDING,  TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS,
	                                 "You feel a cloak of blindness dissolve."},
{"burning hands",  NULL                , SPELL, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"call lightning", NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         NULL},
{"charm person",   NULL                , SPELL, 75, 50, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
	                                 "You feel more self-confident."},
{"chill touch",    NULL                , SPELL, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 
                                                                         TRUE, MAG_DAMAGE | MAG_AFFECTS,
	                                 "You feel your strength return."},
{"clone",          NULL                , SPELL, 80, 65, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS,
	                                 NULL},
{"color spray",    NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"control weather",NULL                , SPELL, 75, 25, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,
	                                 NULL}, // VERIFY MAG_MANUAL
{"create food",    NULL                , SPELL, 30, 5, 4, POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS,
	                                 NULL},
{"create water",   NULL                , SPELL,  30, 5, 4, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL,
	                                 NULL}, // doublon
{"cure blind",     NULL                , SPELL, 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
	                                 NULL},
{"cure critic",    NULL                , SPELL, 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	                                 NULL},
{"cure light",     NULL                , SPELL, 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	                                 NULL},
{"curse",          NULL                , SPELL, 80, 50, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, TRUE, 
                                                           MAG_AFFECTS | MAG_ALTER_OBJS,
	                                 "You feel more optimistic."},
{"darkness",       NULL                , SPELL, 30, 5, 4, POS_STANDING, TAR_IGNORE, FALSE, MAG_ROOMS,
	                                 NULL},
{"detect alignment",NULL               , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	                                 "You feel less aware."},
{"detect invisibility", NULL           , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	                                 "Your eyes stop tingling."},
{"detect magic",   NULL                , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	                                 "The detect magic wears off."},
{"detect poison",  NULL                , SPELL, 15, 5, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                          TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
	                                 "The detect poison wears off."},
{"dispel evil",    NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"dispel good",    NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         NULL},
{"earthquake",     NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS,
	                                 NULL},
{"enchant weapon", NULL                , SPELL, 150, 100, 10, POS_STANDING, TAR_OBJ_INV, FALSE, MAG_MANUAL,
	                                 NULL},
{"energy drain",   NULL                , SPELL, 40, 25, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, 
                                                           MAG_DAMAGE | MAG_MANUAL,
	                                 NULL},
{"group armor",    NULL                , SPELL, 50, 30, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS,
	                                 NULL},
{"fireball",       NULL                , SPELL, 40, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"fly",            NULL                , SPELL, 40, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	                                 "You drift slowly to the ground."},
{"group heal",     NULL                , SPELL, 80, 60, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS,
	                                 NULL},
{"harm",           NULL                , SPELL, 75, 45, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"heal",           NULL                , SPELL, 60, 40, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_UNAFFECTS,
	                                 NULL},
{"infravision",    NULL                , SPELL, 25, 10, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	                                 "Your night vision seems to fade."},
{"invisibility",   NULL                , SPELL, 35, 25, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                           TAR_OBJ_ROOM, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
	                                 "You feel yourself exposed."},
{"lightning bolt", NULL                , SPELL, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"locate object",  NULL                , SPELL, 25, 20, 1, POS_STANDING, TAR_OBJ_WORLD, FALSE, MAG_MANUAL,
	                                 NULL},
{"magic missile",  NULL                , SPELL, 25, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"poison",         NULL                , SPELL, 50, 20, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
	                                                   MAG_AFFECTS | MAG_ALTER_OBJS,
	                                 "You feel less sick."},
{"protection from evil", NULL          , SPELL, 40, 10, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	                                 "You feel less protected."},
{"remove curse",   NULL                , SPELL, 45, 25, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
	                                                   MAG_UNAFFECTS | MAG_ALTER_OBJS,
	                                 NULL},
{"remove poison",  NULL                , SPELL, 40, 8, 4, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                          TAR_OBJ_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS,
	                                 NULL},
{"sanctuary",      NULL                , SPELL, 110, 85, 5, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	                                 "The white aura around your body fades."},
{"sense life",     NULL                , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	                                 "You feel less aware of your surroundings."},
{"shocking grasp", NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	                                 NULL},
{"sleep",          NULL                , SPELL, 40, 25, 5, POS_STANDING, TAR_CHAR_ROOM, TRUE, MAG_AFFECTS,
	                                 "You feel less tired."},
{"strength",       NULL                , SPELL, 35, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	                                 "You feel weaker."},
{"summon",         NULL                , SPELL, 75, 50, 3, POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
	                                 NULL},
{"teleport",       NULL                , SPELL, 75, 50, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	                                 NULL},
{"waterwalk",      NULL                , SPELL, 40, 20, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	                                 "Your feet seem less buoyant."},
{"word of recall", NULL                , SPELL, 20, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	                                 NULL},
{"identify",       NULL                , SPELL, 50, 25, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                         TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
                                         NULL},
{ NULL,            NULL                , SPELL, 0, 0, 0, 0, 0, 0, 0, NULL}
};

/*
  // NON-castable spells should appear below here. 
  spello(SPELL_IDENTIFY, "identify", 0, 0, 0, 0,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
	NULL);

  // you might want to name this one something more fitting to your theme -Welcor
  spello(SPELL_DG_AFFECT, "Script-inflicted", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0,
	NULL);
*/

 while (spell_assign[i].name) {
   CREATE(Q, struct str_spells, 1);
   spedit_init_new_spell (Q);
   free (Q->name);    /* spedit_init_new_spell will strdup("undefined"); */

   Q->name     = strdup (spell_assign[i].name);
   Q->type     = spell_assign[i].type;
   Q->function = spell_assign[i].function; 
   Q->min_pos  = spell_assign[i].min_pos;
   Q->targ_flags = spell_assign[i].targ_flags;
   Q->mag_flags = spell_assign[i].mag_flags;
   Q->status   = available;

   Q->serial   = ++i;   
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
 if ((vict == ch) && !IS_SPELL_SELF(spell) && (!IS_SPELL_GROUP(spell)) ) {
   send_to_char (ch, "You can't cast this spell upon yourself!\r\n");
   return;
 }
 if ((vict != ch) && !IS_SPELL_VICT(spell) && !IS_SPELL_GRPVICT(spell) && !IS_SPELL_ROOM(spell)) {
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

/*
 if (affected_by_spell (vict, spell->serial) && (!IS_SPELL_ACCDUR(spell->flags)) &&
                                                (!IS_SPELL_ACCAFF(spell->flags))) {
   send_to_char (ch, "%s", NOEFFECT);
   rts_code = TRUE;
 }
 else
*/ // TODO: add a different flags to support ACCAFF, ACCDUR
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
//       affect_join (vict, &af, IS_SPELL_ACCDUR(spell->flags), FALSE, IS_SPELL_ACCAFF(spell->flags), FALSE); 
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


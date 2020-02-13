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
#include "config.h"
#include "fight.h"
#include "spedit.h"

extern int mag_manacost(struct char_data *ch, int spellnum);

/*
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
     if (Q->vnum == ptr->vnum) {
       fprintf (fp, "%d %d\n", Q->vnum, Q->num_prac);
       break;
     }
   if (!Q)  
     fprintf (fp, "%d 0\n", ptr->vnum); 
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
   ret = fscanf (fp, "%d %d\n", &plrspells->vnum, &plrspells->num_prac); 
   if (prev)
     prev->next = plrspells;
   else
     ch->plrspells = plrspells;
   plrspells->next = NULL;  
 }
 fclose (fp);
}
*/

/*
int find_spell_assign (struct char_data *ch, struct str_spells *ptr)
{
 int i;
 
 for (i=0; i<NUM_CLASSES; i++)
   if (ptr->assign[i].class_num == GET_CLASS(ch))
     return i;
 return -1;
}
*/

/*
char *get_spell_wear_off (int vnum) 
{
 struct str_spells *Q;

 for (Q = list_spells; Q; Q=Q->next)
   if (Q->vnum == vnum)
     return (Q->messages.wear_off);
 return NULL;
}
*/
/*
void free_player_spells (struct char_data *ch)
{
  struct str_plrspells *Q = NULL, *next;
  
  for (Q = ch->plrspells; Q; Q = next) {
    next = Q->next;
    free (Q);
  }
  ch->plrspells = NULL;
}
*/

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
 char *s, *targ = NULL;
 static const char *wrong_pos[] = {
"Lie still; you are DEAD!!! :-(\r\n",
"You are in a pretty bad shape, unable to do anything!\r\n",
"You are in a pretty bad shape, unable to do anything!\r\n",
"All you can do right now is think about the stars!\r\n",
"In your dreams, or what?\r\n",
"Nah... You feel too relaxed to do that..\r\n",
"Maybe you should get on your feet first?\r\n",
"No way!  You're fighting for your life!\r\n"};

 struct char_data *tch = NULL;
 struct obj_data *tobj = NULL;
 struct char_data *vict = NULL;
 struct obj_data *ovict = NULL;
 struct str_spells *spell = NULL;
 int i, delay, rts_code = TRUE;
 int effectiveness = 0;
 int number;
 int level, assign, target = 0;
 int mana = 0;

 if (IS_NPC(ch))
   return;

 // for spell check that is is enclosed in ''
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

 if (subcmd == SCMD_SPELL)
   spell = get_spell_by_name(s, SPELL); 
 else
   spell = get_spell_by_vnum(subcmd);

 if (!spell || (spell->status != available)) {
   if (subcmd == SCMD_SPELL)
     send_to_char(ch, "Cast what?!?\r\n");
   else
     send_to_char(ch, "%s", HUH);  
   return;
 }

 // for spells and skills with a function, we only check if status == available, and
 // we return the control to it function.
 if (spell->function) {
   if (spell->type == SPELL) {
//     call_ASPELL (spell->function, GET_LEVEL(ch), ch, vict, ovict);
   }
   else {
     call_ACMD (spell->function, ch, argument, 0, 0);
     return;
   }
 }
 if (((spell->type == SPELL) && (subcmd != SCMD_SPELL)) ||
     ((spell->type == SKILL) && (subcmd == SCMD_SPELL))) {
   send_to_char (ch, "%s", HUH);
   return;
 }
 level = get_spell_level_by_vnum(spell->vnum, GET_CLASS(ch));
 if (GET_LEVEL(ch) < level) {
   send_to_char(ch, "You do not know that %s!\r\n", (spell->type == SPELL) ? "spell" : "skill");
   return;
 }
 if ((GET_LEVEL(ch) < LVL_IMMORT) && (GET_SKILL(ch, spell->vnum) == 0)) {
   send_to_char (ch, "You are unfamilliar with that %s.\r\n", (spell->type == SPELL) ? "spell" : "skill");
   return;
 }
 if (GET_POS(ch) < spell->min_pos) {
   send_to_char (ch, "%s", wrong_pos[(int) GET_POS(ch)]);
   return;
 }

/* Find the target */
  if (targ != NULL) {
    char arg[MAX_INPUT_LENGTH];

    strlcpy(arg, targ, sizeof(arg));
    one_argument(arg, targ);
    skip_spaces(&targ);

    /* Copy target to global cast_arg2, for use in spells like locate object */
    strcpy(cast_arg2, targ);
  }

  if (spell->targ_flags & TAR_IGNORE) {
    target = TRUE;
  } else if (targ != NULL && *targ) {
    number = get_number(&targ);
    if (!target && (spell->targ_flags & TAR_CHAR_ROOM)) {
      if ((tch = get_char_vis(ch, targ, &number, FIND_CHAR_ROOM)) != NULL)
        target = TRUE;
    }
    if (!target && (spell->targ_flags & TAR_CHAR_WORLD))
      if ((tch = get_char_vis(ch, targ, &number, FIND_CHAR_WORLD)) != NULL)
        target = TRUE;

    if (!target && (spell->targ_flags & TAR_OBJ_INV))
      if ((tobj = get_obj_in_list_vis(ch, targ, &number, ch->carrying)) != NULL)
        target = TRUE;

    if (!target && (spell->targ_flags & TAR_OBJ_EQUIP)) {
      for (i = 0; !target && i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && isname(targ, GET_EQ(ch, i)->name)) {
          tobj = GET_EQ(ch, i);
          target = TRUE;
        }
    }
    if (!target && (spell->targ_flags & TAR_OBJ_ROOM))
      if ((tobj = get_obj_in_list_vis(ch, targ, &number, world[IN_ROOM(ch)].contents)) != NULL)
        target = TRUE;

    if (!target && (spell->targ_flags & TAR_OBJ_WORLD))
      if ((tobj = get_obj_vis(ch, targ, &number)) != NULL)
        target = TRUE;

  } else {                      /* if target string is empty */
 if (!target && (spell->targ_flags & TAR_FIGHT_SELF))
      if (FIGHTING(ch) != NULL) {
        tch = ch;
        target = TRUE;
      }
    if (!target && (spell->targ_flags & TAR_FIGHT_VICT))
      if (FIGHTING(ch) != NULL) {
        tch = FIGHTING(ch);
        target = TRUE;
      }
    /* if no target specified, and the spell isn't violent, default to self */
    if (!target && ((spell->targ_flags & TAR_CHAR_ROOM) || (spell->targ_flags & TAR_SELF_ONLY)) &&
        !(spell->mag_flags & MAG_VIOLENT)) {
      tch = ch;
      target = TRUE;
    }
    if (!target) {
      send_to_char(ch, "Upon %s should the spell be cast?\r\n",
                (spell->targ_flags & (TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)) ? "what" : "who");
      return;
    }
  }

  if (target && (tch == ch) && (spell->damages) && (spell->mag_flags & MAG_VIOLENT)) {
    send_to_char(ch, "You shouldn't cast that on yourself -- could be bad for your health!\r\n");
    return;
  }
  if (!target) {
    send_to_char(ch, "Cannot find the target of your spell!\r\n");
    return;
  }
  mana = mag_manacost(ch, spell->vnum);
  if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
    return;
  }

 if (spell->effectiveness)
   effectiveness = GET_SKILL(ch, spell->vnum) * 
                   formula_interpreter (ch, vict, spell->vnum, TRUE, spell->effectiveness, &rts_code) / 100;

 if (rand_number (0, 101) > effectiveness) {
   send_to_char (ch, "You lost your concentration!\r\n");
   if (mana > 0)
     GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana / 2)));

   // if you lost your concentration and the spell is MAG_VIOLENT, you still hit a mobile to start the fight.
   if ((spell->mag_flags & MAG_VIOLENT) && tch && IS_NPC(tch))
     hit(tch, ch, TYPE_UNDEFINED);

   return;
 }

 // check logical with above IS_SPELL_LEARNED, maybe it should return assign or 0 then?!?
 /*
 if ((assign = find_spell_assign (ch, spell)) == -1) {
   send_to_char (ch, "You are unfamilliar with that %s.\r\n", (spell->type == SPELL) ? "spell" : "skill");
   return;
 }
 */
 if ((spell->type == SPELL) && (GET_LEVEL(ch) < LVL_IMMORT)) {
   assign = find_spell_assign (ch, spell); 

   mana = formula_interpreter (ch, vict, spell->vnum, TRUE, spell->assign[assign].num_mana, &rts_code);
     
   if ((mana > 0) && (GET_MANA(ch) < mana)) {
     send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
     return;
   }
 }

 if (spell->delay) {
   delay = formula_interpreter (ch, vict, spell->vnum, TRUE, spell->delay, 
                                &rts_code);
   WAIT_STATE (ch, (delay > MAX_SPELL_DELAY) ? MAX_SPELL_DELAY : delay);
 }

 if (cast_spell(ch, tch, tobj, spell->vnum)) {
    WAIT_STATE(ch, PULSE_VIOLENCE);
    if (mana > 0)
      GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
 }

 if (spell->script)
   if (!perform_script (spell->script, ch, vict, ovict, spell->vnum, 0) && !rts_code)
     send_to_char (ch, "%s", NOEFFECT);   
}


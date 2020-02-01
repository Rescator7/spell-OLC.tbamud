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
extern struct str_spells *get_spell_by_vnum (int vnum);
extern struct str_spells *get_spell_by_name (char *name, char type);

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

int find_spell_assign (struct char_data *ch, struct str_spells *ptr)
{
 int i;
 
 for (i=0; i<6; i++)
   if ( ((ptr->assign[i].class_num  < NUM_CLASSES)    && 
         (ptr->assign[i].class_num == GET_CLASS(ch))) )
    return i;
 return -1;
}

// FIX ME... using num_prac is wrong
int IS_SPELL_LEARNED (struct char_data *ch, int vnum) {
  struct str_plrspells *Q;
  struct str_spells *ptr;
  int x, rts_code;

  for (ptr = list_spells; ptr && (ptr->vnum != vnum); ptr = ptr->next);
  if (!ptr) {
    log("SYSERR: (IS_SPELL_LEARNED): spells %d doesn't exist!", vnum);
    return FALSE;
  }
  for (Q = ch->plrspells; Q; Q = Q->next)
    if (Q->vnum == vnum) {
      if ((x = find_spell_assign (ch, ptr)) == -1)  
        return FALSE; 
      if (Q->num_prac >= formula_interpreter (ch, ch, vnum, TRUE, ptr->assign[x].num_prac, 
                                              &rts_code))
        return TRUE;
    }
  return FALSE;   
}

char *get_spell_wear_off (int vnum) 
{
 struct str_spells *Q;

 for (Q = list_spells; Q; Q=Q->next)
   if (Q->vnum == vnum)
     return (Q->wear_off_msg);
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
 int VNUM = 0;
 char buf[MAX_STRING_LENGTH];
 struct str_spells *Q;
 struct str_spell_assign {
    char *name;
    void *function;
    char type;
    int  manamax,
         manamin,
         manachng,
         min_pos,
         targ_flags,
         violent_flags,
         mag_flags,
         FirstClass, FirstLevel,
         SecondClass, SecondLevel;
    int  fapply_num;
    char *fapply_modifier,
         *fapply_duration;
    int  sapply_num;
    char *sapply_modifier,
         *sapply_duration;
    char *damages;
    char *wear_off_msg;
 } default_spells[] = {
{"armor",          NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_ACCDUR, 
                                         CLASS_MAGIC_USER, 4,
                                         CLASS_CLERIC, 1,
                                         APPLY_AC, "-20", "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         "You feel less protected."},
{"teleport",       spell_teleport      , SPELL, 75, 50, 3, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
                                         CLASS_UNDEFINED, 0,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"bless",          NULL                , SPELL, 35, 5, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, 
                                                                        MAG_AFFECTS | MAG_ALTER_OBJS  | MAG_ACCDUR,
                                         CLASS_CLERIC, 5,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_HITROLL, "2", "6",
                                         APPLY_SAVING_SPELL, "-1", "6",
                                         NULL,
	                                 "You feel less righteous."},
{"blindness",      NULL                , SPELL, 35, 25, 1, POS_STANDING,  TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS,
                                         CLASS_MAGIC_USER, 9,
                                         CLASS_CLERIC, 6,
                                         APPLY_HITROLL, "-4", "2",
                                         APPLY_AC, "40", "2",
                                         NULL,
	                                 "You feel a cloak of blindness dissolve."},
{"burning hands",  NULL                , SPELL, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_MAGIC_USER, 5,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(3, self.class == 0 ? 8 : 6) + 3",
	                                 NULL},
{"call lightning", NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_CLERIC, 15,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(7, 8) + 7",
                                         NULL},
{"charm person",   spell_charm         , SPELL, 75, 50, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 16,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel more self-confident."},
{"chill touch",    NULL                , SPELL, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 
                                                                         TRUE, MAG_DAMAGE | MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 3,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_STR, "-1", "4",
                                         APPLY_NONE, NULL, NULL,
	                                 "dice(1, self.class == 0 ? 8 : 6) + 1", 
	                                 "You feel your strength return."},
{"clone",          NULL                , SPELL, 80, 65, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_SUMMONS,
                                         CLASS_MAGIC_USER, 30,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"color spray",    NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_MAGIC_USER, 11,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(9, self.class == 0 ? 8 : 6) + 9",
	                                 NULL},
{"control weather",NULL                , SPELL, 75, 25, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_MANUAL,
                                         CLASS_CLERIC, 17,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL}, // VERIFY MAG_MANUAL
{"create food",    NULL                , SPELL, 30, 5, 4, POS_STANDING, TAR_IGNORE, FALSE, MAG_CREATIONS,
                                         CLASS_CLERIC, 2,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"create water",   spell_create_water  , SPELL,  30, 5, 4, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL,
                                         CLASS_CLERIC, 2,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"cure blind",     NULL                , SPELL, 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
                                         CLASS_CLERIC, 4,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"cure critic",    NULL                , SPELL, 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS,
                                         CLASS_CLERIC, 9,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"cure light",     NULL                , SPELL, 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS,
                                         CLASS_CLERIC, 1,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"curse",          NULL                , SPELL, 80, 50, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, TRUE, 
                                                           MAG_AFFECTS | MAG_ALTER_OBJS | MAG_ACCDUR | MAG_ACCMOD,
                                         CLASS_MAGIC_USER, 14,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_HITROLL, "-1", "1 + (self.level / 2)",
                                         APPLY_DAMROLL, "-1", "1 + (self.level / 2)",
                                         NULL,
	                                 "You feel more optimistic."},
{"detect alignment",NULL               , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 4,
                                         CLASS_UNDEFINED, 0,
                                         AFF_DETECT_ALIGN, NULL, "self.level + 12", // check code, it's not always self.level...
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel less aware."},
{"detect invisibility", NULL           , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 2,
                                         CLASS_CLERIC, 6,
                                         AFF_DETECT_INVIS, NULL, "self.level + 12",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "Your eyes stop tingling."},
{"detect magic",   NULL                , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 2,
                                         CLASS_UNDEFINED, 0,
                                         AFF_DETECT_MAGIC, 0, "self.level + 12",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "The detect magic wears off."},
{"detect poison",  spell_detect_poison , SPELL, 15, 5, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                          TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 10,
                                         CLASS_CLERIC, 3,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "The detect poison wears off."},
{"dispel evil",    NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_CLERIC, 14,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(6, 8) + 6", // SEE: magic.c, this is more complicated than this, add spec_proc instead?
	                                 NULL},
{"earthquake",     NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_IGNORE, TRUE, MAG_AREAS,
                                         CLASS_CLERIC, 12,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(2, 8) + self.level",
	                                 NULL},
{"enchant weapon", spell_enchant_weapon, SPELL, 150, 100, 10, POS_STANDING, TAR_OBJ_INV, FALSE, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 26,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"energy drain",   NULL                , SPELL, 40, 25, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, 
                                                           MAG_DAMAGE | MAG_MANUAL,
                                         CLASS_MAGIC_USER, 13,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "vict.level <= 2 ? 100 : dice(1, 10)",
	                                 NULL},
{"fireball",       NULL                , SPELL, 40, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_MAGIC_USER, 15,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(11, self.class == 0 ? 8 : 6) + 11",
	                                 NULL},
{"harm",           NULL                , SPELL, 75, 45, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_CLERIC, 19,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(8, 8) + 8",
	                                 NULL},
{"heal",           NULL                , SPELL, 60, 40, 3, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_UNAFFECTS,
                                         CLASS_CLERIC, 16,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"invisibility",   NULL                , SPELL, 35, 25, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                           TAR_OBJ_ROOM, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 4,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_AC, "-40", "12 + (self.level / 4)",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel yourself exposed."},
{"lightning bolt", NULL                , SPELL, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_MAGIC_USER, 9,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(7, self.class == 0 ? 8 : 6) + 7",
	                                 NULL},
{"locate object",  spell_locate_object,  SPELL, 25, 20, 1, POS_STANDING, TAR_OBJ_WORLD, FALSE, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 6,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"magic missile",  NULL                , SPELL, 25, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_MAGIC_USER, 1,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
	                                 "dice(1, self.class == 0 ? 8 : 6) + 1", 
                                         NULL},
{"poison",         NULL                , SPELL, 50, 20, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
	                                                   MAG_AFFECTS | MAG_ALTER_OBJS,
                                         CLASS_MAGIC_USER, 14,
                                         CLASS_CLERIC, 8,
                                         APPLY_STR, "-2", "self.level",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel less sick."},
{"protection from evil", NULL          , SPELL, 40, 10, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 8,
                                         CLASS_UNDEFINED, 0,
                                         AFF_PROTECT_EVIL, NULL, "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel less protected."},
{"remove curse",   NULL                , SPELL, 45, 25, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
	                                                   MAG_UNAFFECTS | MAG_ALTER_OBJS,
                                         CLASS_CLERIC, 26,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"sanctuary",      NULL                , SPELL, 110, 85, 5, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 15,
                                         CLASS_UNDEFINED, 0,
                                         AFF_SANCTUARY, NULL, "4",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "The white aura around your body fades."},
{"shocking grasp", NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_MAGIC_USER, 7,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(5, self.class == 0 ? 8 : 6) + 5",
	                                 NULL},
{"sleep",          NULL                , SPELL, 40, 25, 5, POS_STANDING, TAR_CHAR_ROOM, TRUE, MAG_AFFECTS,
                                         CLASS_MAGIC_USER, 8,
                                         CLASS_UNDEFINED, 0,
                                         AFF_SLEEP, NULL, "self.level / 4",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel less tired."},
{"strength",       NULL                , SPELL, 35, 30, 1, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_ACCDUR | MAG_ACCMOD,
                                         CLASS_MAGIC_USER, 6,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_STR, "1 + (self.level > 18)", "(self.level / 2) + 4",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel weaker."},
{"summon",         spell_summon        , SPELL, 75, 50, 3, POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
                                         CLASS_CLERIC, 10,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"ventriloquate",  NULL                , SPELL, 0, 0, 0, 0, 0, 0, 0, CLASS_UNDEFINED, 0, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"word of recall", spell_recall        , SPELL, 20, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
                                         CLASS_CLERIC, 12,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"remove poison",  NULL                , SPELL, 40, 8, 4, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                          TAR_OBJ_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS,
                                         CLASS_CLERIC, 10,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"sense life",     NULL                , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 18,
                                         CLASS_UNDEFINED, 0,
                                         AFF_SENSE_LIFE, NULL, "self.level",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You feel less aware of your surroundings."},
{"animate dead",   NULL                , SPELL, 35, 10, 3, POS_STANDING, TAR_OBJ_ROOM, FALSE, MAG_SUMMONS, 
                                         CLASS_UNDEFINED, 0,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL},
{"dispel good",    NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
                                         CLASS_CLERIC, 14,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(6, 8) + 6", // SEE: magic.c, this is more complicated than this, add spec_proc instead?
                                         NULL},
{"group armor",    NULL                , SPELL, 50, 30, 2, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS,
                                         CLASS_CLERIC, 9,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"group heal",     NULL                , SPELL, 80, 60, 5, POS_STANDING, TAR_IGNORE, FALSE, MAG_GROUPS,
                                         CLASS_CLERIC, 22,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"group recall",   NULL                , SPELL, 0, 0, 0, 0, 0, 0, 0, CLASS_UNDEFINED, 0, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"infravision",    NULL                , SPELL, 25, 10, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 3,
                                         CLASS_CLERIC, 7,
                                         AFF_INFRAVISION, NULL, "12 + self.level",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "Your night vision seems to fade."},
{"waterwalk",      NULL                , SPELL, 40, 20, 2, POS_STANDING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_UNDEFINED, 0,
                                         CLASS_UNDEFINED, 0,
                                         AFF_WATERWALK, NULL, "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "Your feet seem less buoyant."},
{"identify",       spell_identify      , SPELL, 50, 25, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                         TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 20,
                                         CLASS_CLERIC, 11,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL},
{"fly",            NULL                , SPELL, 40, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 22,
                                         CLASS_UNDEFINED, 0,
                                         AFF_FLYING, NULL, "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 "You drift slowly to the ground."},
{"darkness",       NULL                , SPELL, 30, 5, 4, POS_STANDING, TAR_IGNORE, FALSE, MAG_ROOMS,
                                         CLASS_CLERIC, 12,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
	                                 NULL},
{"backstab",       do_backstab         , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 3, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"bash",           do_bash             , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 12, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"hide",           do_hide             , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 5, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"kick",           do_kick             , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 1, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"pick lock",      NULL                , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 2, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"whirlwind",      do_whirlwind        , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 16, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"rescue",         do_rescue           , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 3, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"sneak",          do_sneak            , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 1, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"steal",          do_steal            , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 4, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"track",          do_track            , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 6, 
                                                                     CLASS_WARRIOR, 9, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{"bandage",        do_bandage          , SKILL, 0, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 7, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL},
{ NULL,            NULL                , SPELL, 0, 0, 0, 0, 0, 0, 0, CLASS_UNDEFINED, 0, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, NULL}
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

 while (default_spells[i].name) {
   CREATE(Q, struct str_spells, 1);
   spedit_init_new_spell (Q);
   free (Q->name);    /* spedit_init_new_spell will strdup("undefined"); */

   Q->name     = strdup (default_spells[i].name);
   Q->type     = default_spells[i].type;
   Q->function = default_spells[i].function; 
   Q->min_pos  = default_spells[i].min_pos;
   Q->targ_flags = default_spells[i].targ_flags;
   Q->mag_flags = default_spells[i].mag_flags;
   Q->effectiveness = strdup("100");

   if (default_spells[i].wear_off_msg)
     Q->wear_off_msg = strdup(default_spells[i].wear_off_msg);

   if (default_spells[i].damages) {
     Q->damages = strdup(default_spells[i].damages);
     Q->max_dam = 50;
   }
   snprintf(buf, sizeof(buf), "(%d - (%d * self.level)) > %d ? (%d - (%d * self.level)) : %d",
                              default_spells[i].manamax, default_spells[i].manachng, 
                              default_spells[i].manamin,
                              default_spells[i].manamax, default_spells[i].manachng, 
                              default_spells[i].manamin);
   if (default_spells[i].FirstClass != CLASS_UNDEFINED) {
     Q->assign[0].class_num = default_spells[i].FirstClass;
     Q->assign[0].level = default_spells[i].FirstLevel;
     Q->assign[0].num_mana = strdup(buf);
   }
   if (default_spells[i].SecondClass != CLASS_UNDEFINED) {
     Q->assign[1].class_num = default_spells[i].SecondClass;
     Q->assign[1].level = default_spells[i].SecondLevel;
     Q->assign[1].num_mana = strdup(buf);
   }
   if (default_spells[i].fapply_num != APPLY_NONE) {
     Q->applies[0].appl_num = default_spells[i].fapply_num;
     if (default_spells[i].fapply_modifier)
       Q->applies[0].modifier = strdup(default_spells[i].fapply_modifier);
     if (default_spells[i].fapply_duration)
       Q->applies[0].duration = strdup(default_spells[i].fapply_duration);
   }
   if (default_spells[i].sapply_num != APPLY_NONE) {
     Q->applies[1].appl_num = default_spells[i].sapply_num;
     if (default_spells[i].sapply_modifier)
       Q->applies[1].modifier = strdup(default_spells[i].sapply_modifier);
     if (default_spells[i].sapply_duration)
       Q->applies[1].duration = strdup(default_spells[i].sapply_duration);
   }
   if (i++ == NUM_SPELLS)
     VNUM = MAX_SPELLS - NUM_SPELLS;

   Q->vnum = VNUM + i;
   Q->status  = available;

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

 struct char_data *vict = NULL;
 struct obj_data *ovict = NULL;
 struct str_spells *spell = NULL;
 struct affected_type af;
 int i, delay, rts_code = TRUE;
 int effectiveness = 0;
 int assign;

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

 send_to_char(ch, "searching for %s\r\n", s);
 
 if (subcmd == SCMD_SPELL)
   spell = get_spell_by_name(s, SPELL); 
 else
   spell = get_spell_by_vnum(subcmd);

 // debuging only, remove
 if (spell)
   send_to_char(ch, "found %d\r\n", spell->vnum);

 if (!spell || ((spell->status != available) && (GET_LEVEL(ch) < LVL_IMMORT))) {
   send_to_char (ch, "There's no no such %s available to you.\r\n", (subcmd == SCMD_SPELL) ? "spell" : "skill");
   return;
 }

 // for spells and skills with a function, we only check if status == available, and
 // we return the control to it function.
 if (spell->function) {
   if (spell->type == SPELL)
     call_ASPELL (spell->function, GET_LEVEL(ch), ch, vict, ovict);
   else
     call_ACMD (spell->function, ch, argument, 0, 0);
   return;
 }

 if (((spell->type == SPELL) && (subcmd != SCMD_SPELL)) ||
     ((spell->type == SKILL) && (subcmd == SCMD_SPELL))) {
   send_to_char (ch, "Huh?!?\r\n");
   return;
 }
 if ((GET_LEVEL(ch) < LVL_IMMORT) && !IS_SPELL_LEARNED (ch, spell->vnum)) {
   send_to_char (ch, "You are unfamilliar with that %s.\r\n", (spell->type == SPELL) ? "spell" : "skill");
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

 if (spell->effectiveness)
   effectiveness = GET_SKILL(ch, spell->vnum) * 
                   formula_interpreter (ch, vict, spell->vnum, TRUE, spell->effectiveness, &rts_code) / 100;

 if (rand_number (0, 101) > effectiveness) {
   send_to_char (ch, "You lost your concentration!\r\n");
   return;
 }

 // check logical with above IS_SPELL_LEARNED, maybe it should return assign or 0 then?!?
 if ((assign = find_spell_assign (ch, spell)) == -1) {
   send_to_char (ch, "You are unfamilliar with that %s.\r\n", (spell->type == SPELL) ? "spell" : "skill");
   return;
 }

 if (spell->type == SPELL) {
   int mana = formula_interpreter (ch, vict, spell->vnum, TRUE, spell->assign[assign].num_mana, &rts_code);
    if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_IMMORT)) {
      send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
      return;
    }
 }

 if (spell->delay) {
   delay = formula_interpreter (ch, vict, spell->vnum, TRUE, spell->delay, 
                                &rts_code);
   WAIT_STATE (ch, (delay > MAX_SPELL_DELAY) ? MAX_SPELL_DELAY : delay);
 }

/*
 if (affected_by_spell (vict, spell->vnum) && (!IS_SPELL_ACCDUR(spell->flags)) &&
                                                (!IS_SPELL_ACCAFF(spell->flags))) {
   send_to_char (ch, "%s", NOEFFECT);
   rts_code = TRUE;
 }
 else
*/
   for (i=0; i<6; i++) 
     if (spell->applies[i].appl_num != -1) {
       af.spell = spell->vnum;
       af.duration = spell->applies[i].duration ? 
                     formula_interpreter (ch, vict, spell->vnum, TRUE, spell->applies[i].duration, &rts_code) : 0;
       af.modifier = spell->applies[i].modifier ?  formula_interpreter (ch, vict, spell->vnum, TRUE,
                                                   spell->applies[i].modifier, &rts_code) : 0; 
       af.bitvector[0] = spell->applies[i].appl_num < NUM_APPLIES ? 0 : 
                      (1 << (spell->applies[i].appl_num - NUM_APPLIES));
       af.location = spell->applies[i].appl_num < NUM_APPLIES ?  spell->applies[i].appl_num : APPLY_NONE;
       affect_join (vict, &af, IS_SPELL_ACCDUR(spell) == 1, FALSE, IS_SPELL_ACCMOD(spell) == 1, FALSE); 
       rts_code = TRUE; 
     }


 if (spell->script)
   if (!perform_script (spell->script, ch, vict, ovict, spell->vnum, 0) && !rts_code)
     send_to_char (ch, "%s", NOEFFECT);   
}


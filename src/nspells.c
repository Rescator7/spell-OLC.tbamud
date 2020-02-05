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

extern struct str_spells *get_spell_by_vnum (int vnum);
extern struct str_spells *get_spell_by_name (char *name, char type);
extern int get_spell_level_by_vnum(int vnum, int class);
extern int mag_manacost(struct char_data *ch, int spellnum);

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
 
 for (i=0; i<NUM_CLASSES; i++)
   if (ptr->assign[i].class_num == GET_CLASS(ch))
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
     return (Q->messages.wear_off);
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
    char *to_vict,
         *to_room;
    char *wear_off_msg;
 } default_spells[] = {
{"armor",          NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM, MAG_AFFECTS | MAG_ACCDUR, 
                                         CLASS_MAGIC_USER, 4,
                                         CLASS_CLERIC, 1,
                                         APPLY_AC, "-20", "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         "You feel someone protecting you.",
                                         NULL,
                                         "You feel less protected."},
{"teleport",       spell_teleport      , SPELL, 75, 50, 3, POS_STANDING, TAR_CHAR_ROOM, MAG_MANUAL,
                                         CLASS_UNDEFINED, 0,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"bless",          NULL                , SPELL, 35, 5, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, 
                                                                        MAG_AFFECTS | MAG_ALTER_OBJS  | MAG_ACCDUR,
                                         CLASS_CLERIC, 5,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_HITROLL, "2", "6",
                                         APPLY_SAVING_SPELL, "-1", "6",
                                         NULL,
                                         "You feel righteous.",
                                         NULL,
	                                 "You feel less righteous."},
{"blindness",      NULL                , SPELL, 35, 25, 1, POS_STANDING,  TAR_CHAR_ROOM | TAR_NOT_SELF, MAG_AFFECTS,
                                         CLASS_MAGIC_USER, 9,
                                         CLASS_CLERIC, 6,
                                         APPLY_HITROLL, "-4", "2",
                                         APPLY_AC, "40", "2",
                                         NULL,
                                         "You have been blinded!",
                                         "$n seems to be blinded!",
	                                 "You feel a cloak of blindness dissolve."},
{"burning hands",  NULL                , SPELL, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 5,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(3, self.class == 0 ? 8 : 6) + 3",
                                         NULL,
                                         NULL,
	                                 NULL},
{"call lightning", NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_CLERIC, 15,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(7, 8) + 7",
                                         NULL,
                                         NULL,
                                         NULL},
{"charm person",   spell_charm         , SPELL, 75, 50, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_NOT_SELF, MAG_MANUAL | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 16,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 "You feel more self-confident."},
{"chill touch",    NULL                , SPELL, 30, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 
                                                                         MAG_DAMAGE | MAG_VIOLENT | MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 3,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_STR, "-1", "4",
                                         APPLY_NONE, NULL, NULL,
	                                 "dice(1, self.class == 0 ? 8 : 6) + 1", 
                                         "You feel your strength wither!",
                                         NULL,
	                                 "You feel your strength return."},
{"clone",          NULL                , SPELL, 80, 65, 5, POS_STANDING, TAR_IGNORE, MAG_SUMMONS,
                                         CLASS_MAGIC_USER, 30,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"color spray",    NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 11,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(9, self.class == 0 ? 8 : 6) + 9",
                                         NULL,
                                         NULL,
	                                 NULL},
{"control weather",NULL                , SPELL, 75, 25, 5, POS_STANDING, TAR_IGNORE, MAG_MANUAL,
                                         CLASS_CLERIC, 17,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL}, // VERIFY MAG_MANUAL
{"create food",    NULL                , SPELL, 30, 5, 4, POS_STANDING, TAR_IGNORE, MAG_CREATIONS,
                                         CLASS_CLERIC, 2,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"create water",   spell_create_water  , SPELL,  30, 5, 4, POS_STANDING, TAR_OBJ_INV | TAR_OBJ_EQUIP, MAG_MANUAL,
                                         CLASS_CLERIC, 2,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"cure blind",     NULL                , SPELL, 30, 5, 2, POS_STANDING, TAR_CHAR_ROOM, MAG_UNAFFECTS,
                                         CLASS_CLERIC, 4,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "Your vision returns!",
                                         NULL,
	                                 "There's a momentary gleam in $n's eyes."},
{"cure critic",    NULL                , SPELL, 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, MAG_POINTS,
                                         CLASS_CLERIC, 9,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"cure light",     NULL                , SPELL, 30, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, MAG_POINTS,
                                         CLASS_CLERIC, 1,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"curse",          NULL                , SPELL, 80, 50, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV, 
                                                           MAG_VIOLENT | MAG_AFFECTS | MAG_ALTER_OBJS | MAG_ACCDUR | MAG_ACCMOD,
                                         CLASS_MAGIC_USER, 14,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_HITROLL, "-1", "1 + (self.level / 2)",
                                         APPLY_DAMROLL, "-1", "1 + (self.level / 2)",
                                         NULL,
                                         "You feel very uncomfortable.",
                                         "$n briefly glows red!",
	                                 "You feel more optimistic."},
{"detect alignment",NULL               , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 4,
                                         CLASS_UNDEFINED, 0,
                                         AFF_DETECT_ALIGN + NUM_APPLIES, NULL, "self.level + 12", // check code, it's not always self.level...
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "Your eyes tingle.",
                                         NULL,
	                                 "You feel less aware."},
{"detect invisibility", NULL           , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 2,
                                         CLASS_CLERIC, 6,
                                         AFF_DETECT_INVIS + NUM_APPLIES, NULL, "self.level + 12",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "Your eyes tingle.",
                                         NULL,
	                                 "Your eyes stop tingling."},
{"detect magic",   NULL                , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 2,
                                         CLASS_UNDEFINED, 0,
                                         AFF_DETECT_MAGIC + NUM_APPLIES, 0, "self.level + 12",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "Your eyes tingle.",
                                         NULL,
	                                 "The detect magic wears off."},
{"detect poison",  spell_detect_poison , SPELL, 15, 5, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                          TAR_OBJ_ROOM, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 10,
                                         CLASS_CLERIC, 3,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 "The detect poison wears off."},
{"dispel evil",    NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_CLERIC, 14,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(6, 8) + 6", // SEE: magic.c, this is more complicated than this, add spec_proc instead?
                                         NULL,
                                         NULL,
	                                 NULL},
{"earthquake",     NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_IGNORE, MAG_AREAS | MAG_VIOLENT,
                                         CLASS_CLERIC, 12,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(2, 8) + self.level",
                                         NULL,
                                         NULL,
	                                 NULL},
{"enchant weapon", spell_enchant_weapon, SPELL, 150, 100, 10, POS_STANDING, TAR_OBJ_INV, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 26,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"energy drain",   NULL                , SPELL, 40, 25, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, 
                                                           MAG_DAMAGE | MAG_VIOLENT | MAG_MANUAL,
                                         CLASS_MAGIC_USER, 13,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "vict.level <= 2 ? 100 : dice(1, 10)",
                                         NULL,
                                         NULL,
	                                 NULL},
{"fireball",       NULL                , SPELL, 40, 30, 2, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 15,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(11, self.class == 0 ? 8 : 6) + 11",
                                         NULL,
                                         NULL,
	                                 NULL},
{"harm",           NULL                , SPELL, 75, 45, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_CLERIC, 19,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(8, 8) + 8",
                                         NULL,
                                         NULL,
	                                 NULL},
{"heal",           NULL                , SPELL, 60, 40, 3, POS_FIGHTING, TAR_CHAR_ROOM, MAG_POINTS | MAG_UNAFFECTS,
                                         CLASS_CLERIC, 16,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"invisibility",   NULL                , SPELL, 35, 25, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                           TAR_OBJ_ROOM, MAG_AFFECTS | MAG_ALTER_OBJS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 4,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_AC, "-40", "12 + (self.level / 4)",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "You vanish.",
                                         "$n slowly fades out of existence.",
	                                 "You feel yourself exposed."},
{"lightning bolt", NULL                , SPELL, 30, 15, 1, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 9,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(7, self.class == 0 ? 8 : 6) + 7",
                                         NULL,
                                         NULL,
	                                 NULL},
{"locate object",  spell_locate_object,  SPELL, 25, 20, 1, POS_STANDING, TAR_OBJ_WORLD, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 6,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"magic missile",  NULL                , SPELL, 25, 10, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 1,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
	                                 "dice(1, self.class == 0 ? 8 : 6) + 1", 
                                         NULL,
                                         NULL,
                                         NULL},
{"poison",         NULL                , SPELL, 50, 20, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV,
	                                                   MAG_VIOLENT | MAG_AFFECTS | MAG_ALTER_OBJS,
                                         CLASS_MAGIC_USER, 14,
                                         CLASS_CLERIC, 8,
                                         APPLY_STR, "-2", "self.level",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "You feel very sick.",
                                         "$n gets violently ill!",
	                                 "You feel less sick."},
{"protection from evil", NULL          , SPELL, 40, 10, 3, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 8,
                                         CLASS_UNDEFINED, 0,
                                         AFF_PROTECT_EVIL + NUM_APPLIES, NULL, "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "You feel invulnerable!",
                                         NULL,
	                                 "You feel less protected."},
{"remove curse",   NULL                , SPELL, 45, 25, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP,
	                                                   MAG_UNAFFECTS | MAG_ALTER_OBJS,
                                         CLASS_CLERIC, 26,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "You don't feel so unlucky.",
                                         NULL,
	                                 NULL},
{"sanctuary",      NULL                , SPELL, 110, 85, 5, POS_STANDING, TAR_CHAR_ROOM, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 15,
                                         CLASS_UNDEFINED, 0,
                                         AFF_SANCTUARY + NUM_APPLIES, NULL, "4",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "A white aura momentarily surrounds you.",
                                         "$n is surrounded by a white aura.",
	                                 "The white aura around your body fades."},
{"shocking grasp", NULL                , SPELL, 30, 15, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 7,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(5, self.class == 0 ? 8 : 6) + 5",
                                         NULL,
                                         NULL,
	                                 NULL},
{"sleep",          NULL                , SPELL, 40, 25, 5, POS_STANDING, TAR_CHAR_ROOM, MAG_AFFECTS | MAG_VIOLENT,
                                         CLASS_MAGIC_USER, 8,
                                         CLASS_UNDEFINED, 0,
                                         AFF_SLEEP + NUM_APPLIES, NULL, "self.level / 4",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 "You feel less tired."},
{"strength",       NULL                , SPELL, 35, 30, 1, POS_STANDING, TAR_CHAR_ROOM, MAG_AFFECTS | MAG_ACCDUR | MAG_ACCMOD,
                                         CLASS_MAGIC_USER, 6,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_STR, "1 + (self.level > 18)", "(self.level / 2) + 4",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "You feel stronger!",
                                         NULL,
	                                 "You feel weaker."},
{"summon",         spell_summon        , SPELL, 75, 50, 3, POS_STANDING, TAR_CHAR_WORLD | TAR_NOT_SELF, MAG_MANUAL,
                                         CLASS_CLERIC, 10,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"ventriloquate",  NULL                , SPELL, 0, 0, 0, 0, 0, 0, CLASS_UNDEFINED, 0, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"word of recall", spell_recall        , SPELL, 20, 10, 2, POS_FIGHTING, TAR_CHAR_ROOM, MAG_MANUAL,
                                         CLASS_CLERIC, 12,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"remove poison",  NULL                , SPELL, 40, 8, 4, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                                          TAR_OBJ_ROOM, MAG_UNAFFECTS | MAG_ALTER_OBJS,
                                         CLASS_CLERIC, 10,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "A warm feeling runs through your body!",
                                         NULL,
	                                 "$n looks better."},
{"sense life",     NULL                , SPELL, 20, 10, 2, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_CLERIC, 18,
                                         CLASS_UNDEFINED, 0,
                                         AFF_SENSE_LIFE + NUM_APPLIES, NULL, "self.level",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "Your feel your awareness improve.",
                                         NULL,
	                                 "You feel less aware of your surroundings."},
{"animate dead",   NULL                , SPELL, 35, 10, 3, POS_STANDING, TAR_OBJ_ROOM, MAG_SUMMONS, 
                                         CLASS_UNDEFINED, 0,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL},
{"dispel good",    NULL                , SPELL, 40, 25, 3, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, MAG_DAMAGE | MAG_VIOLENT,
                                         CLASS_CLERIC, 14,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         "dice(6, 8) + 6", // SEE: magic.c, this is more complicated than this, add spec_proc instead?
                                         NULL,
                                         NULL,
                                         NULL},
{"group armor",    NULL                , SPELL, 50, 30, 2, POS_STANDING, TAR_IGNORE, MAG_GROUPS,
                                         CLASS_CLERIC, 9,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"group heal",     NULL                , SPELL, 80, 60, 5, POS_STANDING, TAR_IGNORE, MAG_GROUPS,
                                         CLASS_CLERIC, 22,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"group recall",   NULL                , SPELL, 0, 0, 0, 0, 0, 0, CLASS_UNDEFINED, 0, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"infravision",    NULL                , SPELL, 25, 10, 1, POS_STANDING, TAR_CHAR_ROOM | TAR_SELF_ONLY, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 3,
                                         CLASS_CLERIC, 7,
                                         AFF_INFRAVISION + NUM_APPLIES, NULL, "12 + self.level",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "Your eyes glow red.",
                                         "$n's eyes glow red.",
	                                 "Your night vision seems to fade."},
{"waterwalk",      NULL                , SPELL, 40, 20, 2, POS_STANDING, TAR_CHAR_ROOM, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_UNDEFINED, 0,
                                         CLASS_UNDEFINED, 0,
                                         AFF_WATERWALK + NUM_APPLIES, NULL, "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "You feel webbing between your toes.",
                                         NULL,
	                                 "Your feet seem less buoyant."},
{"identify",       spell_identify      , SPELL, 50, 25, 5, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | 
                                         TAR_OBJ_ROOM, MAG_MANUAL,
                                         CLASS_MAGIC_USER, 20,
                                         CLASS_CLERIC, 11,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL},
{"fly",            NULL                , SPELL, 40, 20, 2, POS_FIGHTING, TAR_CHAR_ROOM, MAG_AFFECTS | MAG_ACCDUR,
                                         CLASS_MAGIC_USER, 22,
                                         CLASS_UNDEFINED, 0,
                                         AFF_FLYING + NUM_APPLIES, NULL, "24",
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         "You float above the ground.",
                                         NULL,
	                                 "You drift slowly to the ground."},
{"darkness",       NULL                , SPELL, 30, 5, 4, POS_STANDING, TAR_IGNORE, MAG_ROOMS,
                                         CLASS_CLERIC, 12,
                                         CLASS_UNDEFINED, 0,
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL,
                                         NULL,
                                         NULL,
	                                 NULL},
{"backstab",       do_backstab         , SKILL, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 3, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"bash",           do_bash             , SKILL, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 12, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"hide",           do_hide             , SKILL, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 5, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"kick",           do_kick             , SKILL, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 1, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"pick lock",      NULL                , SKILL, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 2, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"whirlwind",      do_whirlwind        , SKILL, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 16, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"rescue",         do_rescue           , SKILL, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 3, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"sneak",          do_sneak            , SKILL, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 1, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"steal",          do_steal            , SKILL, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 4, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"track",          do_track            , SKILL, 0, 0, 0, 0, 0, 0, CLASS_THIEF, 6, 
                                                                  CLASS_WARRIOR, 9, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{"bandage",        do_bandage          , SKILL, 0, 0, 0, 0, 0, 0, CLASS_WARRIOR, 7, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL},
{ NULL,            NULL                , SPELL, 0, 0, 0, 0, 0, 0, CLASS_UNDEFINED, 0, CLASS_UNDEFINED, 0, 
                                         APPLY_NONE, NULL, NULL,
                                         APPLY_NONE, NULL, NULL,
                                         NULL, 
                                         NULL,
                                         NULL,
                                         NULL}
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
     Q->messages.wear_off = strdup(default_spells[i].wear_off_msg);

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
     if (default_spells[i].type == SPELL)
       Q->assign[0].num_mana = strdup(buf);
   }
   if (default_spells[i].SecondClass != CLASS_UNDEFINED) {
     Q->assign[1].class_num = default_spells[i].SecondClass;
     Q->assign[1].level = default_spells[i].SecondLevel;
     if (default_spells[i].type == SPELL)
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

   if (default_spells[i].to_vict)
    Q->messages.to_vict = strdup(default_spells[i].to_vict);

   if (default_spells[i].to_room)
     Q->messages.to_room = strdup(default_spells[i].to_room);

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

 send_to_char(ch, "searching for %s\r\n", s);
 
 if (subcmd == SCMD_SPELL)
   spell = get_spell_by_name(s, SPELL); 
 else
   spell = get_spell_by_vnum(subcmd);

 // debuging only, remove
 if (spell)
   send_to_char(ch, "found %d\r\n", spell->vnum);

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
 if ((GET_LEVEL(ch) < LVL_IMMORT) && !IS_SPELL_LEARNED (ch, spell->vnum)) {
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


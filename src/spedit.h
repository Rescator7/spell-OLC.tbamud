/* all by castillo */
#define unavailable             0
#define available               1
#define NUM_CHAR_POSITION       9
#define NUM_SPELL_FLAGS         13
#define MAX_SPELL_DELAY         50   /* this equal to 5 sec */
#define SPELL                   'P'
#define SKILL                   'K'
#define MAX_SPELL_AFFECTS       6

struct str_spells *get_spell_by_vnum(int vnum);

struct str_prot {
   int  prot_num;
   char *duration;
   char *resist;
};

struct str_appl {
   int  appl_num;
   char *modifier;
   char *duration;  
};

struct str_assign {
   int  class_num;
   int  level;
   char *num_prac;
   char *num_mana;
};

struct str_mesg {
   char *wear_off;
   char *to_self;
   char *to_vict;
   char *to_room;
};

struct str_spells {
   char type;
   int  vnum;
   int  status;
   int  min_pos;
   int  max_dam;
   int  violent;
   char *name;
   char *delay;
   int  targ_flags;
   int  mag_flags;
   char *damages;
   char *effectiveness;
   char *script;
   struct str_prot protfrom [6];
   struct str_appl applies [6];
   struct str_assign assign [NUM_CLASSES]; 
   struct str_mesg messages;
   void *function;
   struct str_spells *next;
};

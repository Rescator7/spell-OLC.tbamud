/* all by castillo */
#define unavailable             0
#define available               1
#define NUM_CHAR_POSITION       9
#define NUM_SPELL_FLAGS         8
#define MAX_SPELL_DELAY         50   /* this equal to 5 sec */
#define IS_SPELL_OBJ(flags)     (flags & 1)
#define IS_SPELL_SELF(flags)    (flags & (1 << 1))
#define IS_SPELL_GROUP(flags)   (flags & (1 << 2))
#define IS_SPELL_VICT(flags)    (flags & (1 << 3))
#define IS_SPELL_VICTGRP(flags) (flags & (1 << 4))
#define IS_SPELL_ROOM(flags)    (flags & (1 << 5))
#define IS_SPELL_ACCDUR(flags)  (flags & (1 << 6))    
#define IS_SPELL_ACCAFF(flags)  (flags & (1 << 7))
#define SPELL                   'P'
#define SKILL                   'K'

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

struct str_spells {
   int  saved;
   char type;
   int  serial;
   int  status;
   int  min_pos;
   int  max_dam;
   char *name;
   char *delay;
   int  flags;
   char *damages;
   char *effectiveness;
   char *wear_off;
   char *script;
   struct str_prot protfrom [6];
   struct str_appl applies [6];
   struct str_assign assign [NUM_CLASSES]; 
   void *function;
   struct str_spells *next;
};

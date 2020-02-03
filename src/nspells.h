extern struct str_spells *list_spells;
extern const char *position_types[];
int perform_script (char *str, struct char_data *self, 
                               struct char_data *vict, 
                               struct obj_data  *ovict, 
                    int    from,
                    int    param);
int formula_interpreter (struct char_data *self, struct char_data *vict, int sepll_vnum,
                         int syserr, char *cmd, int *rts_code);
int GET_ALIGN_TYPE (struct char_data *ch);
struct str_plrspells {
  int    vnum;
  int    num_prac;
  struct str_plrspells *next;
};


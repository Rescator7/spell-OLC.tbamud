#define ASCRIPT(name) int name(char **str, struct char_data *self, \
                                           struct char_data *vict, \
                                           struct obj_data  *ovict, \
                                           int    from, \
                                           int    param)
struct str_script {
   char *name;
   void *function;   
   int  param;
};

/* external function */
int formula_interpreter (struct char_data *self, struct char_data *vict, 
                         int spell_vnum, int syserr, char *cmd, int *rts_code);
void char_from_room (struct char_data *ch);
void char_to_room (struct char_data *ch, room_rnum room);
sh_int find_target_room (struct char_data *ch, char *rawroomstr);
void look_at_room (struct char_data *ch, int ignore_brief);


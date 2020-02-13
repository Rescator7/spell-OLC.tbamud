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

int perform_script (char *str, struct char_data *self,
                               struct char_data *vict,
                               struct obj_data  *ovict,
                    int   from,
                    int   param);

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h" 
#include "db.h"
#include "comm.h"
#include "code.h"
#include "string.h"

int call_ASCRIPT (int (*function)(), char *str, struct char_data *self,
                                                struct char_data *vict,
                                                struct obj_data  *ovict,
                                                int    from, 
                                                int    param) 
{
 return ((*function) (str, self, vict, ovict, from, param));
}  

/* let's declare our scripts functions */
ASCRIPT (scr_act_to);
ASCRIPT (scr_teleport);
ASCRIPT (scr_load_mobile);

struct str_script list_script[] = {
  {"SAY_TO_ROOM", scr_act_to,      TO_ROOM},
  {"SAY_TO_SELF", scr_act_to,      TO_CHAR},
  {"SAY_TO_CHAR", scr_act_to,      TO_CHAR},
  {"SAY_TO_VICT", scr_act_to,      TO_VICT},
  {"TELEPORT",    scr_teleport,    0},
  {"LOAD_MOBILE", scr_load_mobile, 0},
  {"\n", NULL, 0}
};

/* this functions is similar to strstr, but will ignore space in str1 and
   it will move the pointer of str1 */
int strlstr (char **str1, char *str2)
{ 
 int cpt = 0;

 if ((!*str1) || (!str2))
   return 0;

 for (; *str1; (*str1)++)
   if ((**str1 == ' ') || (**str1 == str2[cpt])) 
     if ((**str1 == str2[cpt]) && (++cpt == strlen(str2))) {
       (*str1)++;
       return 1;
     }
     else;
   else
     return 0;         
 return 0;
} 

char *strext (char *str1, char c)
{
 char lbuf[MAX_STRING_LENGTH];
 int i;

 for (i=0; i<strlen(str1); i++)
   if (str1[i] == c)
     break;
   else
     lbuf[i] = str1[i];
 lbuf[i] = '\0';
 return (strdup (lbuf));
}

ASCRIPT(scr_act_to)
{
 char *s;

 strlstr (str, "{\"");
 s = strext (*str, '\"');
 *str += strlen (s);
 act (s, TRUE, self, NULL, vict, param); 
 free (s);
 strlstr (str, "\"};");
 return TRUE;
}  

ASCRIPT(scr_teleport)
{
 int rts_code = TRUE, location;
 char *where;

 strlstr (str, "{");
 where = strext (*str, '}');
 *str += strlen (where);
 strlstr (str, "};");

 if (((location = find_target_room (vict, where)) >= 0) && vict) {
   char_from_room (vict);
   char_to_room (vict, location);
   if (!IS_NPC (vict))
     look_at_room (vict, 1);
 } else
     rts_code = FALSE;
 free (where);
 return (rts_code);
}

ASCRIPT(scr_load_mobile)
{
 struct char_data *mob;
 int value, rts_code = TRUE, r_num, location;
 char *who, *where;

 strlstr (str, "{");
 who   = strext (*str, ',');
 *str += strlen (who);
 strlstr (str, ",");
 where = strext (*str, '}');
 *str += strlen (where);
 strlstr (str, "};");

 value = formula_interpreter (self, vict, from, TRUE, who, &rts_code);
 if (((r_num = real_mobile (value)) >= 0) &&
     ((location = find_target_room (vict, where)) >= 0)) {
   mob = read_mobile (r_num, REAL);
   char_to_room (mob, location);
 } else
     rts_code = FALSE;

 free (who);
 free (where);
 return (rts_code);
}

int perform_script (char *str, struct char_data *self, 
                               struct char_data *vict, 
                               struct obj_data  *ovict, 
                    int    from,
                    int    param)
{
 int cpt = 0, ptr = 0, rts_code = FALSE;
 char *p, *b;

 p = str;
 b = (char *) &p;

 for (ptr=0; ptr<strlen(p); ptr++) {
   cpt = 0;
   while (list_script[cpt].function) 
     if (!(strncmp (list_script[cpt].name, p, strlen (list_script[cpt].name)))) {
       p += strlen(list_script[cpt].name);
       if (call_ASCRIPT (list_script[cpt].function, b, self, vict,
                         ovict, from, list_script[cpt].param))
         rts_code = TRUE;
       break;
     }
     else
       cpt++;
   if ((*p != '\r') && (*p != '\n') && (*p != ' '))
     return FALSE;
   p++;
 };
 return rts_code;
}

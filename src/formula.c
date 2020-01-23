/* *************************************************************************** */
/* code:    formula.c                                                          */
/* desc:    perform mathematics formula and return a value or an error message.*/
/*          (using operators, variables, conditional ? :, and logical)         */
/* author:  Bob Castillo (castillo7@hotmail.com)                               */
/* version: 0.5b      (for circle MUD 3.x with Oasis OLC)                      */
/* date:    25 dec 1999 : Revised 2020                                         */
/* *************************************************************************** */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "spells.h"
#include "oasis.h"
#include "spedit.h"
#include "db.h"
#include "formula.h"

void add_to_formula (struct formula **head_formula, int command, int value)
{
 struct formula *Q, *ptr, *prev = NULL;

 CREATE (Q, struct formula, 1);
 for (ptr = *head_formula; ptr; prev = ptr, ptr = ptr->next);
 if (prev == NULL) 
   *head_formula = Q;
 else
   prev->next = Q;
 Q->prev = prev;
 Q->next = NULL;

 Q->command = command;
 Q->value = value;
 Q->sign = 1;
 Q->next = NULL;
}

void free_formula (struct formula **head_formula) 
{
 struct formula *Q, *next = NULL;

 for (Q = *head_formula; Q; Q = next) {
   next = Q->next;
   free (Q);
 }
 *head_formula = NULL;
}

int get_formula_typeact (struct char_data *ch, char *str, int *ptr, int spell_vnum, 
                         int syserr) 
{
 int cpt = 0;

 if (str[*ptr] == ' ')
   return CODE_SPACE;
 else
   if ((str[*ptr] >= '0') && (str[*ptr] <= '9'))
     return CODE_DIGIT;
 while (list_codes[++cpt][0] != '\n') 
   if (!strncmp(list_codes[cpt], (char *) &str[*ptr], strlen(list_codes[cpt]))) {
     *ptr += strlen(list_codes[cpt]) - 1;
     return cpt;      
   }
 return -1;
}

int check_formula_syntax (int otype_act, int type_act) 
{
 // CHAR_TYPE[0][] can't follow [1][]
 const char *test [4][2] = {
 {"abcdefghjklmnopqstux",  "abcdefghjkloprstxz"},          // can follow: imnquvwy  
                                                           //     !,-,+,*,rand(,self.,vict.,digit
 {"i",                     "abcdefghijkloprst"},           // '!' can follow mnquvwz = -,+,(,rand(,self.,vict., digit 
 {"yrz",                   "iquvwyz"},                     // digit, ')', variables CAN'T follow: 
                                                           //             digit, !, (, rand(, self., vict., variables.
 {"vw",                    "abcdefghijklmnopqrstuvwxy"}    // self., vict. can follow z = variables.
 };
 int i;

 for (i=0; i<4; i++) {
   if (strchr (test[i][0], CHAR_CODE(otype_act))) {
     if (strchr (test[i][1], CHAR_CODE(type_act))) 
       return (otype_act * 100 + type_act);
     else
       return (0); 
   }
 }
 return 0;
}

void send_formula_error (struct char_data *ch, int error, int spell_vnum, int syserr)
{
 char buf[2048];

 char *error_5000[] = {
 "Unknown variable or operator",               /* 5000 */
 "it should have as much '(' and ')'",         /* 5001 */
 "it should have as much '?' and ':'",         /* 5002 */
 "can't perform formula. (level 6 reached!)",  /* 5003 */
 "':' was expected but not found",             /* 5004 */
 "A formula can't end by ':'",                 /* 5005 */
 "division by 0",                              /* 5006 */
 "Unsupported +++",                            /* 5007 */
 "Unsupported ---"                             /* 5008 */
 };                 

 if (syserr)
   sprintf (buf, "SYSERR: got error (%d) from spell (%d) : ", error, spell_vnum);
 else
   strcpy (buf, "SYNTAX : ");
 if ((error >= 5000) && (error <= 5008))
   strcat (buf, error_5000 [error-5000]);
 else
   if ((error >= 9900) || (error < 6000))
      sprintf (buf, "%s%s can't follow %s", buf, 
             ((error/100) == CODE_DIGIT) ? "A number" : list_codes[error/100],
             ((error%100) == CODE_DIGIT) ? "a number" : list_codes[error%100]);
   else if (error >= 7000)
          sprintf (buf, "%sA formula can't end by %s", buf, list_codes[error-7000]);
        else if (error >= 6000)
               sprintf (buf, "%sA formula can't start by %s", buf,
                             list_codes[error-6000]);
 if (syserr == TRUE) 
   mudlog (BRF, LVL_BUILDER, TRUE, "%s", buf);
 else {
   send_to_char (ch, "%s\r\n", buf);
 }
}

#ifdef DEBUG_FORMULA
  void show_formula (struct char_data *ch, struct formula *head_formula)  
  {
    int cpt = 0;
    struct formula *Q; 

    strcpy (buf, "formula : ");
    for (Q = head_formula; Q; Q=Q->next, cpt++) { 
       if (Q->command == CODE_DIGIT) 
         sprintf (buf, "%s%d", buf, Q->value);
       else
         sprintf (buf, "%s%s", buf, ((Q->command    == CODE_ART_SUB) &&
                                     (Q->next) && 
                                     (Q->next->sign == -1)) ? "+" :
                                    ((Q->command    == CODE_ART_ADD) &&
                                     (Q->next) &&  
                                     (Q->next->sign == -1)) ? "-" : 
                                    list_codes[Q->command]);
    }
    sprintf (buf, "%s, nodes: %d\r\n", buf, cpt);
    send_to_char (ch, buf);
  }
#endif

void remove_node (struct formula **head_formula, struct formula *ptr)
{
 if (ptr->prev) 
   ptr->prev->next = ptr->next;
 else 
   *head_formula = ptr->next;
 if (ptr->next) 
   ptr->next->prev = ptr->prev;
 free (ptr);
}

int remove_brace (struct formula **head_formula, struct formula *C, 
                  struct char_data *ch)
{
 struct formula *A, *B;

 if (!C)
   return 0;
 B = C->prev;
 A = B->prev;
 if ((C->command == CODE_ART_CBRACE) && 
    ((A->command == CODE_ART_OBRACE) ||
     (A->command == CODE_ART_RAND))) {
   if (A->command == CODE_ART_RAND)
     B->value = (B->sign == -1) ? 0 : rand_number(1, B->value);
   remove_node (head_formula, C);
   remove_node (head_formula, A);

#ifdef DEBUG_FORMULA
     show_formula(ch, *head_formula);
#endif
   return 1; 
 }
 return 0; 
}

struct formula *formula_do_oper (struct formula **head_formula, struct formula *B,
                                 int spell_vnum, struct char_data *ch, int
                                 *rts_code)
{ 
 struct formula *A, *C;

 if (!(A = B->prev) && (B->command != CODE_ART_NOT) 
                    && (B->command != CODE_ART_ADD)
                    && (B->command != CODE_ART_SUB)) {
   *rts_code = 6000 + B->command;
   return NULL;
  } 

 if (!(C = B->next)) {
   *rts_code = 7000 + B->command;
   return NULL;
 }

 switch (C->command) {
   case CODE_ART_SUB : if (C->next)
                         C->next->sign = -1;  
                       remove_node (head_formula, C);
                       return (B); 
   case CODE_ART_ADD : remove_node (head_formula, C);
                       return (B);
 }

 if (A) {               /* !20 or -10 + 10 there's no A*/
   A->value *= A->sign;
   A->sign = 1;
 }
 C->value *= C->sign;
 C->sign = 1;
 switch (B->command) {
   case CODE_CON_IF    : if (!C->next || (C->next->command != CODE_CON_ELSE)) {
                           *rts_code = ERROR_5004;
                           return NULL;
                         } else {
                             if (!C->next->next) {
                               *rts_code = ERROR_5005; 
                               return NULL;
                             }
                             A->value = A->value ? C->value :
                                        C->next->next->value;
                             remove_node (head_formula, C->next->next);
                             remove_node (head_formula, C->next);
                             remove_node (head_formula, C);
                             remove_node (head_formula, B);
                           }
                         return (A);   
   case CODE_LOG_OR    : A->value = (A->value || C->value) ? 1 : 0 ; break;
   case CODE_LOG_AND   : A->value = (A->value && C->value) ? 1 : 0 ; break;
   case CODE_LOG_GREQ  : A->value = (A->value >= C->value) ? 1 : 0 ; break;
   case CODE_LOG_GR    : A->value = (A->value >  C->value) ? 1 : 0 ; break;
   case CODE_LOG_LTEQ  : A->value = (A->value <= C->value) ? 1 : 0 ; break;
   case CODE_LOG_LT    : A->value = (A->value <  C->value) ? 1 : 0 ; break;
   case CODE_LOG_EQ    : A->value = (A->value == C->value) ? 1 : 0 ; break;
   case CODE_LOG_NOTEQ : A->value = (A->value != C->value) ? 1 : 0 ; break;
   case CODE_ART_XOR   : A->value = (A->value ^  C->value); break;
   case CODE_ART_AND   : A->value = (A->value &  C->value); break;
   case CODE_ART_OR    : A->value = (A->value |  C->value); break;
   case CODE_ART_MOD   : A->value = (A->value %  C->value); break;
   case CODE_ART_SUB   : if (!A) { // || (A->command != CODE_DIGIT)) {
                           C->sign = -1;
                           remove_node (head_formula, B);
                           return (C);
                         } else
                             A->value = (A->value - C->value);
                         break; // here
   case CODE_ART_ADD   : if (!A) {// || (A->command != CODE_DIGIT)) {
                           remove_node (head_formula, B);
                           return (C);
                         } else
                             A->value = (A->value + C->value); 
                         break;
   case CODE_ART_DIV   : if (C->value)
                           A->value = (A->value / C->value);
                         else {
                           *rts_code = ERROR_5006;
                           return NULL;
                         } 
                         break;
   case CODE_ART_MUL : A->value = (A->value * C->value); break; 
   case CODE_ART_NOT : C->sign  = 1;
                       C->value = !C->value ? 1 : 0;
                       remove_node (head_formula, B);
                       return (C);
 }
 if ((A->next = C->next))
   (C->next)->prev = A;
 free (C);
 free (B);
#ifdef DEBUG_FORMULA
   show_formula(ch, *head_formula);
#endif
 return (A);
}

int perform_formula (struct formula **head_formula, int spell_vnum,
                     struct char_data *ch, int syserr, int *rts_code) 
{
  char *priority_check[5] = {"ijkl", "opx", "nm", "abcdefgh", "s"};
  int exit = 0, brace, result, mode;
  struct formula *Q, *ptr, *tmp;

  if (!*head_formula)
    return 0;

  do {
    for (ptr = NULL, Q = *head_formula; Q; Q = Q->next)
      if ((Q->command == CODE_ART_OBRACE) || (Q->command == CODE_ART_RAND)) 
        ptr = Q;
    if (ptr == NULL) {
      ptr = *head_formula;
      exit = 1;
    };
    brace = 1; 
    mode = 0;
    tmp = ptr;
    do {
      while (ptr && ptr->command != CODE_ART_CBRACE) {
        if (strchr (priority_check[mode], CHAR_CODE(ptr->command))) {
          ptr = formula_do_oper (head_formula, ptr, spell_vnum, ch, rts_code);
          if (*rts_code) {
            send_formula_error (ch, *rts_code, spell_vnum, syserr);
            free_formula(head_formula);
            return 0; 
          }
        }
        else
          ptr = ptr->next;
      }
      if (!remove_brace(head_formula, ptr, ch))
        if (++mode > 4)  
          if (!exit) { 
            *rts_code = ERROR_5003;
            send_formula_error (ch, *rts_code, spell_vnum, syserr);
            free_formula(head_formula);
            return 0; 
          }
          else
            brace = 0; 
        else
          ptr = tmp;
      else
        brace = 0; 
    } while (brace);
  } while (!exit);

  result = (*head_formula)->value * (*head_formula)->sign;
  free_formula(head_formula);
  return (result);
}

int formula_interpreter (struct char_data *self, struct char_data *vict, 
                         int spell_vnum, int syserr, char *cmd, int *rts_code)
{
 char buf[2048];

 struct formula *head_formula = NULL;
 int i = 0, num = 0, otype_act  = 0, type_act = 0, self_vict = -1; 
 int cpt_obrace = 0, cpt_cbrace = 0, cpt_if   = 0, cpt_else  = 0;

 sprintf (buf, "%s ", cmd);

 if (strstr(buf, "+++")) {
   *rts_code = ERROR_5007;
   send_formula_error (self, *rts_code, spell_vnum, syserr);
   return 0;
 }

 if (strstr(buf, "---")) {
   *rts_code = ERROR_5008;
   send_formula_error (self, *rts_code, spell_vnum, syserr);
   return 0;
 }

 for (i=0; i<strlen(buf); i++) 
   switch (buf [i]) {
     case '(' : cpt_obrace++; break;
     case ')' : cpt_cbrace++; break;
     case '?' : cpt_if++;     break;
     case ':' : cpt_else++;   break;
     default  : buf[i] = toupper(buf[i]);
   }

 if ((cpt_obrace != cpt_cbrace) || 
     (cpt_if     != cpt_else)) {
   *rts_code = (cpt_obrace != cpt_cbrace) ? ERROR_5001 : ERROR_5002;
   send_formula_error (self, *rts_code, spell_vnum, syserr);
   return 0;
 }

 for (i=0; i<strlen(buf); i++) {
   if (type_act)
     otype_act = type_act;
   if (!(type_act = get_formula_typeact (self, buf, &i, spell_vnum, syserr)) &&
        (i<strlen(buf)-1)) 
     continue;              
   if (type_act == -1) {
     *rts_code = ERROR_5000;
     send_formula_error (self, *rts_code, spell_vnum, syserr);
     free_formula (&head_formula);
     return 0;  
   } else
       if (type_act && ((type_act != CODE_DIGIT) || (otype_act != CODE_DIGIT))) 
         if ((*rts_code = check_formula_syntax (otype_act, type_act))) { 
           send_formula_error (self, *rts_code, spell_vnum, syserr);
           free_formula (&head_formula);
           return 0;
         }
   if ((otype_act == CODE_IDE_SELF) || (otype_act == CODE_IDE_VICT))
     switch (type_act) {
       case CODE_VAR_STR :       num = GET_STR(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_DEX :       num = GET_DEX(self_vict == CODE_IDE_SELF ? self : vict); 
                                 break;
       case CODE_VAR_INT :       num = GET_INT(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_WIS :       num = GET_WIS(self_vict == CODE_IDE_SELF ? self : vict); 
                                 break;
       case CODE_VAR_CON :       num = GET_CON(self_vict == CODE_IDE_SELF ? self : vict);
                                 break; 
       case CODE_VAR_CHA :       num = GET_CHA(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_CLASS :     num = GET_CLASS(self_vict == CODE_IDE_SELF ? self : vict);
                                 break; 
       case CODE_VAR_LEVEL :     num = GET_LEVEL(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_AGE :       num = GET_AGE(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_WEIGHT :    num = GET_WEIGHT(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_HEIGHT :    num = GET_HEIGHT(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_MAXMANA :   num = GET_MAX_MANA(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_MAXHIT :    num = GET_MAX_HIT(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_MAXMOVE :   num = GET_MAX_MOVE(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_GOLD :      num = GET_GOLD(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_EXP :       num = GET_EXP(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_ARMOR :     num = GET_AC(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_HITROLL :   num = GET_HITROLL(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_DAMROLL :   num = GET_DAMROLL(self_vict == CODE_IDE_SELF ? self : vict);
                                 break;
       case CODE_VAR_SAVPARA :   num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_PARA);
                                 break;
       case CODE_VAR_SAVROD :    num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_ROD);
                                 break;
       case CODE_VAR_SAVPETRI :  num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_PETRI);
                                 break;
       case CODE_VAR_SAVBREATH : num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_BREATH);
                                 break;
       case CODE_VAR_SAVSPELL :  num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_SPELL); 
                                 break;  
     } else {
         if (otype_act && ((otype_act != CODE_DIGIT) || (type_act != CODE_DIGIT))) {
           add_to_formula (&head_formula, otype_act, num);
           num = 0; 
         } 
         switch (type_act) {
            case CODE_IDE_SELF :
            case CODE_IDE_VICT : self_vict = type_act; break;
            case CODE_DIGIT    : num = num * 10 + (cmd[i] - '0');  
         }
       }
  }

#ifdef DEBUG_FORMULA
   show_formula (self, head_formula);
#endif
 return (perform_formula(&head_formula, spell_vnum, self, syserr, rts_code));
}

ACMD(do_formula) {
 int rts_code;

 send_to_char (ch, "value: %d\r\n", formula_interpreter (ch, ch, 0, FALSE, argument, &rts_code));
}
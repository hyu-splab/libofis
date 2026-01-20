#ifndef make_DFA_H
#define make_DFA_H


extern int     state_num;
extern int     char_set_size;
extern char    char_set_DPU[256];
extern int    char_hash[256];
extern int     is_any_char;
extern int**    DFA;
extern char    acceptable_states[256];

#ifdef __cplusplus
extern "C" {
#endif
    
void make_DFA(char*);
int make_BUF(int*, char*);

#ifdef __cplusplus
}
#endif

#endif
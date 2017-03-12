#ifndef __FILTER_T_H__
#define __FILTER_T_H__

#include <sys/types.h>


typedef struct List {

  int  Not;
  char *Subscription;

} list_t;


typedef struct Filter {
  
  pid_t pid;
  
  char *Name;
  char *Program;

  list_t **Subscription;

  int   Body;
  int   Fds[3];
  int   Timeout;
  size_t   MaxLength;
 
  /* Statistics */
  int   Accepted;
  int   Refused;
  int   OverLength;      /* over max length */

  int   GotHFields;
  int   Responses;

  int   WaitInput;

  boolean_t   Dead; /* number of reply waiting to be received */

  char *CurrentMessageID;

  int             ID;
  pthread_t       ID_t;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;

  struct Filter *next;
  struct Filter *prev;

} filter_t;

/* Prototypes */
filter_t *NewFilter(void);
void   FreeFilter(filter_t *FilterPtr);
void   AllFreeFilter(filter_t *FilterHead);
int    InsertFilter(filter_t *Head, filter_t *NewFilter);
void   FilterLog();

#endif

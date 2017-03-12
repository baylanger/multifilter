/************************************************************************
 *
 * FILE: extfilter.c
 *
 * DESC: functions to handle stuct Filter
 *
**/

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/filter_t.h"
#include "include/parse.h"
#include "include/fd.h"
#include "include/func.h"
#include "include/pid.h"


extern filter_t Head;



/************************************************************************
 *
 * NAME: NewFilter
 *
 * DESC: Allocate memory for a filter_t
 *
**/
filter_t *
NewFilter(void)
{
  filter_t *New;


  New = (filter_t *)malloc(sizeof(filter_t));

  if ((filter_t *)NULL == New) {

    syslog(LOG_CRIT, "NewFilter - malloc() error");
    return((filter_t *)NULL);

  }

  New->Name     = (char *)NULL;
  New->Program  = (char *)NULL;
  New->Subscription  = (list_t **)NULL;
  New->Body     = ERROR;
  New->Fds[0]   = ERROR;
  New->Fds[1]   = ERROR;
  New->Fds[2]   = ERROR;
  New->Timeout  = ERROR;
  New->MaxLength= ERROR;

  New->Accepted    = 0;
  New->Refused     = 0;
  New->OverLength  = 0;

  New->next = New;
  New->prev = New;

  New->pid  = ERROR;
  New->GotHFields = 0;
  New->Responses = 0;

  New->Dead = TRUE;
  New->WaitInput = 0;

  New->ID_t = ERROR;

  pthread_mutex_init(&New->mutex, NULL);
  pthread_cond_init(&New->cond, NULL);

  return((filter_t *)New);

}



/************************************************************************
 *
 * NAME: FreeFilter
 *
 * DESC: Free an object and close Fds
 *
**/
void
FreeFilter(filter_t *FilterPtr)
{
  filter_t *PtrNext,
           *PtrPrev;


  if ((filter_t *)NULL != FilterPtr->next) {

    PtrNext = FilterPtr->next;
    PtrPrev = FilterPtr->prev;
    PtrPrev->next = PtrNext;
    PtrNext->prev = PtrPrev;

    pthread_mutex_lock(&FilterPtr->mutex);
    FilterPtr->Dead = TRUE;
    pthread_mutex_unlock(&FilterPtr->mutex);

    if (ERROR != FilterPtr->pid) {

      (void)close(FilterPtr->Fds[0]);
      (void)close(FilterPtr->Fds[1]);
      (void)sigsend(P_PID, FilterPtr->pid, SIGTERM);

    }

  }

  if ((char *)NULL != FilterPtr->Name) {

    free(FilterPtr->Name);

  }

  if ((char *)NULL != FilterPtr->Program) {

    free(FilterPtr->Program);

  }

/*
  if ((char *)NULL != FilterPtr->Subscription) {

    free(FilterPtr->Subscription);

  }
*/

  free(FilterPtr);

}



/************************************************************************
 *
 * NAME: AllFreeFilter
 *
 * DESC: Free all objects
 *
**/
void
AllFreeFilter(filter_t *FilterHead)
{
  filter_t *FilterPtr,
           *TmpFilter;


  for(FilterPtr = FilterHead->next; FilterPtr != FilterHead; ) {

    TmpFilter = FilterPtr;
    FilterPtr = FilterPtr->next;
    FreeFilter(TmpFilter);

  }

}



/************************************************************************
 *
 * NAME: InsertFilter
 *
 * DESC: Insert a filter object
 *
**/
int
InsertFilter(filter_t *Head, filter_t *NewFilter)
{
  filter_t *PtrCurrent,
           *PtrPrev;


  PtrCurrent = Head->next;

  for(; PtrCurrent != Head; PtrCurrent = PtrCurrent->next) {

    if (0 == strcmp(PtrCurrent->Name, NewFilter->Name)) {

      Log(LOG_ERR, "Duplicated Directive Name <%s>",  PtrCurrent->Name);
      return(ERROR);

    }

    if (0 == strcmp(PtrCurrent->Program, NewFilter->Program)) {

      Log(LOG_ERR, "Duplicated Directive Program <%s>", PtrCurrent->Program);
      return(ERROR);

    }

  }

  PtrPrev = Head->prev;
  NewFilter->next = Head;
  NewFilter->prev = PtrPrev;
  PtrPrev->next = NewFilter;
  Head->prev = NewFilter;

  return(OK);

}



/************************************************************************
 *
 * NAME: FilterLog
 *
 * DESC: Log some stats
 *
**/
void
FilterLog()
{
  FILE *FD = (FILE *)NULL;
  filter_t *FilterPtr;
  extern char *LogFile;


  if ((char *)NULL != LogFile) {

    if ((FILE *)NULL == (FD = FDOpen(LogFile, "a+"))) {

      Log(LOG_ERR, "Couldn't open & create log file [%s]", LogFile);

    }

  }


  for(FilterPtr = Head.next;
      FilterPtr != &Head;
      FilterPtr = FilterPtr->next) {

    pthread_mutex_lock(&FilterPtr->mutex);

    if(TRUE == FilterPtr->Dead) {

      pthread_mutex_unlock(&FilterPtr->mutex);
      continue;

    }

    Log(LOG_INFO, "filter <%s> accepted %d refused %d overlength %d",
                  FilterPtr->Name, FilterPtr->Accepted,
                  FilterPtr->Refused, FilterPtr->OverLength);

    if ((FILE *)NULL != FD) {

      char strtime[64];
      time_t curtime;
      struct tm *curtm;

      time(&curtime);
      curtm = localtime(&curtime);
      strftime(strtime, sizeof(strtime), "%c", curtm);

      if (0 == fprintf(FD, "%s: %s accepted %d refused %d overlength %d\n",
          strtime, FilterPtr->Name, FilterPtr->Accepted,
          FilterPtr->Refused, FilterPtr->OverLength )) {

        Log(LOG_ERR, "Couldn't write to log file [%s]", LogFile);
        (void)FDClose(FD);

      }

    }
    
    FilterPtr->Accepted = 0;
    FilterPtr->Refused = 0;
    FilterPtr->OverLength = 0;

    pthread_mutex_unlock(&FilterPtr->mutex);

  }

  if ((FILE *)NULL != FD) {

    (void)FDClose(FD);

  }

}

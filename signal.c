/************************************************************************
 *
 * FILE: signal.c
 *
 * DESC: Set and handle signals
 *
**/

#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/filter_t.h"
#include "include/func.h"
#include "include/main.h"
#include "include/parse.h"
#include "include/runprg.h"
#include "include/signal.h"

int sig_number;

extern int LogTime;
extern int TotalResponses;
extern int TotalArticles;
extern int GotHFields;
extern int nFilters;
extern char* ConfigFile;
extern filter_t Head;

int ThreadsGotSignal = 0;
int ReloadAtArticle = 0;

pthread_cond_t Signal_c;
pthread_mutex_t Signal_m;

extern pthread_mutex_t FI_m;
extern pthread_cond_t  FI_c;

extern pthread_mutex_t GAH_m;
extern pthread_cond_t  GAH_c;


void *
ReadyForReload(void)
{

  pthread_mutex_lock(&Signal_m);

  ThreadsGotSignal++;
  pthread_cond_signal(&Signal_c);

  pthread_mutex_unlock(&Signal_m);

}



/**********************************************************************
 *
 * NAME: sig_handler()
 *
 * DESC: Function to handle signals
 *
**/
void *
Signal(void *arg)
{

  int i;
  sigset_t set;
  int LastArticleNumber = -1;


  sigfillset(&set);
  sigignore(SIGCHLD);
  alarm(LogTime);

  while(1) {

    ThreadsGotSignal = 0;


/*
    sig_number = sigwait(&set);
*/
    sigwait(&set, &sig_number);

    switch (sig_number) {

      case SIGALRM:

        if ((-1 != LastArticleNumber) &&
            ( 0 != TotalArticles) &&
            (LastArticleNumber == TotalArticles)) {

          Log(LOG_CRIT, "MAJOR ERROR, possible mutex_lock");

          exit(EXIT_FAILURE);

        } else {

          LastArticleNumber = TotalArticles;

        }

      case SIGUSR2:

        alarm(LogTime);
        FilterLog();

      break;

      case SIGUSR1:

        Sig_Reload();

      break;

      case SIGINT:
      case SIGTERM:

        Sig_Term();

      break;

      case SIGPIPE:

        Log(LOG_CRIT, "Couldn't write to a pipe");

      break;

      default:

        Log(LOG_ERR, "Trapped Signal [%d]", sig_number);

      break;

    }

  }

}


/***
 *
**/
static void
Sig_Reload(void)
{

  filter_t NewHead, *TmpPtr1, *TmpPtr2;

  Log(LOG_INFO, "Reload - file <%s>", ConfigFile);

  NewHead.next = &NewHead;
  NewHead.prev = &NewHead;

  if (ERROR == Parse(&NewHead, ConfigFile)) {

    Log(LOG_ERR, "Reload Abord - Error in configuration file");
    return;

  }

Log(LOG_ERR, "Allo1\n");
  pthread_mutex_lock(&GAH_m);
  ReloadAtArticle = TotalArticles + 1;
  pthread_mutex_unlock(&GAH_m);

  pthread_mutex_lock(&Signal_m);
  while((nFilters+1) != ThreadsGotSignal) {
Log(LOG_ERR, "Allo2\n");

    while(0 != pthread_cond_wait(&Signal_c, &Signal_m));

  }
Log(LOG_ERR, "Allo3\n");

  alarm(LogTime);
  FilterLog();
  AllFreeFilter(&Head);
Log(LOG_ERR, "Allo4\n");

  GotHFields = 0;
  TotalArticles = 0;
  TotalResponses = 0;
  ThreadsGotSignal = 0;

Log(LOG_ERR, "Allo7\n");

  /* lets give 2 seconds for all children to exit */
  (void)sleep(5);

  TmpPtr1 = NewHead.next;
  TmpPtr2 = NewHead.prev;

  Head.next = TmpPtr1;
  TmpPtr1->prev = &Head;
  Head.prev = TmpPtr2;
  TmpPtr2->next = &Head;
Log(LOG_ERR, "Allo8\n");

  StartFilters(&Head);
Log(LOG_ERR, "Allo9\n");
  Log(LOG_INFO, "Reload succeeded");

Log(LOG_ERR, "Allo10\n");
  pthread_cond_broadcast(&Signal_c);

  pthread_mutex_unlock(&Signal_m);
Log(LOG_ERR, "END END\n");

  pthread_mutex_lock(&GAH_m);
  ReloadAtArticle = 0;
  pthread_cond_broadcast(&GAH_c);
  pthread_mutex_unlock(&GAH_m);

}


/****
 *
**/
static void
Sig_Term(void)
{
  timespec_t tv;

  FilterLog();

Log(LOG_ERR, "sigterm/int-1\n");

  tv.tv_sec = time(NULL) + 3;
  tv.tv_nsec = 0;

  pthread_mutex_lock(&GAH_m);
  ReloadAtArticle = TotalArticles + 1;
  pthread_mutex_unlock(&GAH_m);

  pthread_mutex_lock(&Signal_m);
  while((nFilters+1) != ThreadsGotSignal) {

Log(LOG_ERR, "sigterm-1\n");

    while(0 != pthread_cond_wait(&Signal_c, &Signal_m));

  }
  pthread_mutex_unlock(&Signal_m);

Log(LOG_ERR, "sigterm/int-2\n");

  AllFreeFilter(&Head);

  DoExit(EXIT_SUCCESS);

}

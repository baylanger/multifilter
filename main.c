/************************************************************************
 *
 * FILE: multifilter.c
 *
 * DESC: Main program & loop
 *
**/

#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <strings.h>
#include <syslog.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/article-read.h"
#include "include/filter_t.h"
#include "include/func.h"
#include "include/filters.h"
#include "include/init.h"
#include "include/main.h"
#include "include/pid.h"
#include "include/signal.h"

filter_t Head;

int     Response = ERROR;

extern int     Validate;
extern char   *ConfigFile;
extern char   *LogFile;
extern char   *PidFile;
extern char    Version[];
extern char   *Article;
extern size_t  ArticleLength;
extern size_t  HeaderLength;

extern pthread_cond_t AR_c;
extern pthread_mutex_t AR_m;

int Ready;

void DoExit(int Status);



/************************************************************************
 *
 * NAME: DoExit
 *
 * DESC: Do the things before exit()
 *
**/
void
DoExit(int Status)
{

  /* attendre que totalarticles == totalresponses */

  if (FALSE == Validate) {

    FilterLog();
    Log(LOG_INFO, "Goodbye - Version %s", Version);

    if ((char *)NULL != PidFile) {

      (void)PID_Close();

    }

    free(PidFile);

    if ((char *)NULL != LogFile) {

      free(LogFile);

    }

  }

  AllFreeFilter(&Head);

  closelog();

  free(ConfigFile);

  exit(Status);

}



/************************************************************************
 *
 * NAME: main
 *
 * DESC: Init program / main loop
 *
**/
int
main(int argc, char **argv)
{
  sigset_t  signalSet;
  pthread_t Signal_t;
  pthread_t AH_t;
  pthread_t AR_t;

  sigfillset(&signalSet);
  pthread_sigmask(SIG_BLOCK, &signalSet, NULL);

  init(argc, argv);

  if (TRUE == Validate) {

    DoExit(EXIT_SUCCESS);

  }

  pthread_create(&Signal_t, NULL, Signal, (void *)NULL);

  pthread_create(&AH_t, NULL, ArticleHeader, (void *)NULL);

  pthread_create(&AR_t, NULL, ArticleRead, (void *)NULL);
  
  pthread_mutex_lock(&AR_m);
  pthread_cond_broadcast(&AR_c);
  pthread_mutex_unlock(&AR_m);

  pthread_exit(NULL);

}

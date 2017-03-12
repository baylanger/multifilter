/************************************************************************
 * FILE: init.c
 *
 * DESC: Initialization functions
 *
 *
**/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/resource.h>
#include <syslog.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/fd.h"
#include "include/filter_t.h"
#include "include/func.h"
#include "include/init.h"
#include "include/main.h"
#include "include/parse.h"
#include "include/pid.h"
#include "include/runprg.h"
#include "include/signal.h"


size_t   PageSize;
int      Validate = FALSE;
int      LogTime = ERROR;
char    *ConfigFile = (char*)NULL;
char    *LogFile = (char *)NULL;
char    *ArticleLogFile = (char *)NULL;
char    *PidFile = (char *)NULL;

FILE *ArticleLogFD;

boolean_t Sync = FALSE;

extern filter_t Head;
extern const char Version[];

extern pthread_mutex_t AH_m;
extern pthread_mutex_t AR_m;
extern pthread_mutex_t GAH_m;
extern pthread_mutex_t GHF_m;
extern pthread_mutex_t nR_m;
extern pthread_mutex_t FO_m;
extern pthread_mutex_t FI_m;
extern pthread_mutex_t Signal_m;

extern pthread_cond_t AH_c;
extern pthread_cond_t AR_c;
extern pthread_cond_t GAH_c;
extern pthread_cond_t GHF_c;
extern pthread_cond_t nR_c;
extern pthread_cond_t FO_c;
extern pthread_cond_t FI_c;
extern pthread_cond_t Signal_c;


void
pthread_mutex_cond_init()
{

  pthread_mutex_init(&AH_m, NULL);
  pthread_mutex_init(&AR_m, NULL);
  pthread_mutex_init(&GAH_m, NULL);
  pthread_mutex_init(&GHF_m, NULL);
  pthread_mutex_init(&nR_m, NULL);
  pthread_mutex_init(&FO_m, NULL);
  pthread_mutex_init(&FI_m, NULL);
  pthread_mutex_init(&Signal_m, NULL);

  pthread_cond_init(&AH_c, NULL);
  pthread_cond_init(&AR_c, NULL);
  pthread_cond_init(&GAH_c, NULL);
  pthread_cond_init(&GHF_c, NULL);
  pthread_cond_init(&nR_c, NULL);
  pthread_cond_init(&FO_c, NULL);
  pthread_cond_init(&FI_c, NULL);
  pthread_cond_init(&Signal_c, NULL);
}

/***************************************************************************
 *
 * NAME: Usage
 * 
 * DESC: Print usage
 *
**/
static void
Usage(void)
{

  fprintf(stderr, "multifilter (C) 1998 Pierre Belanger");
  fprintf(stderr, " (belanger@pobox.com)\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"usage: multifilter [-h|-v|-V] | [options]\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -h            : Print help and exit (this message)\n");
  fprintf(stderr,"  -v            : Show version\n");
  fprintf(stderr,"  -V            : Validate configuration and exit\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Options:\n");
  fprintf(stderr,"  -a <filename> : Location of the article log file\n");
  fprintf(stderr,"  -c <filename> : Location of system configuration file.\n");
  fprintf(stderr,"                    Default = \"%s\"\n", DEFAULTCONFIGFILE);
  fprintf(stderr,"  -l <filename> : Location of the log file\n");
  fprintf(stderr,"  -p <filename> : Location of pid file.\n");
  fprintf(stderr,"                    Default = \"%s\"\n", DEFAULTPIDFILE);
  fprintf(stderr,"  -u <seconds>  : Number of seconds between update\n");
  fprintf(stderr,"                    Default = \"%d\"\n", DEFAULTLOGTIME);

}



/************************************************************************
 *
 * NAME: init
 *
**/
void
init(int argc, char *argv[])
{
  int   c;
  char *progname = (char *)NULL;
  extern int   optind;
  extern char *optarg;


  Head.next = &Head;
  Head.prev = &Head;

  /*
   * find program's name
   */
  if ((char *)NULL != *argv) {
  
    if ((char *)NULL == (progname = (char *)strrchr(argv[0], '/'))) {

      progname = (char *)argv[0];

    } else {

      progname++;

    }

  }
  
  PageSize = (size_t)getpagesize();

  SetLimit();

  while ((1 < argc) && (-1 != (c = getopt (argc, argv, "a:c:l:u:p:vVh")))) {

    switch (c) {

      case 'h':

        Usage();
        exit(EXIT_SUCCESS);

      break;

      case 'v':

        fprintf(stderr, "multifilter (C) 1998 Pierre Belanger");
        fprintf(stderr, " (belanger@pobox.com)\n");
        fprintf(stderr, "Version: %s\n", Version);
        exit(EXIT_SUCCESS);

      break;

      case 'V':

        Validate = TRUE;

      break;

      case 'a':

        ArticleLogFile = (char *)strdup(optarg);

        if ((FILE *)NULL == (ArticleLogFD = FDOpen(ArticleLogFile, "a+"))) {

          Log(LOG_ERR, "Couldn't open & create log file [%s]", ArticleLogFile);

          free(ArticleLogFile);
          ArticleLogFile = (char *)NULL;

        }

      break;

      case 'c':

        ConfigFile = (char *)strdup(optarg);

      break;

      case 'l':

        LogFile = (char *)strdup(optarg);

      break;

      case 'u':

        LogTime = atoi(optarg);

        if (1 >= LogTime) {

          Log(LOG_ERR, "-l LogTime is too small - Setting to default value");

        }

      break;

      case 'p':

        PidFile = (char *)strdup(optarg);

      break;

      default:

        Usage();
        exit(EXIT_SUCCESS);

      break;

    }

  }


  if (ERROR == LogTime) {

    LogTime = DEFAULTLOGTIME;

  }

  if ((char *)NULL == ConfigFile) {

    ConfigFile = (char *)strdup(DEFAULTCONFIGFILE);

  }

  openlog(progname, LOG_PID|LOG_NDELAY|LOG_PERROR, LOG_NEWS);

  if (ERROR == Parse(&Head, ConfigFile)) {
  
    Log(LOG_ERR, "Bad configuration file <%s>", ConfigFile);

    DoExit(EXIT_FAILURE);
    
  }

  if (TRUE == Validate) {

    return;

  }

  if ((char *)NULL == PidFile) {

    PidFile = (char *)strdup(DEFAULTPIDFILE);

  }

  if (ERROR == PID_Open()) {

    Log(LOG_ERR, "Error Opening PID file [%s]", PidFile);
    DoExit(EXIT_FAILURE);

  }


  pthread_mutex_cond_init();

  if (ERROR == StartFilters(&Head)) {

    Log(LOG_ERR, "Error in StartFilters()");
    exit(EXIT_FAILURE);

  }

  Log(LOG_INFO,  "Starting - Version %s", Version);
  fprintf(stderr, "Starting - Version %s", Version);
  fprintf(stderr, "\n");

}



/***************************************************************************
 *
 * NAME: SetLimit
 * 
 * DESC: Set system's limit
 *
**/
static void
SetLimit(void)
{
  int resource;
  struct rlimit limit;


  for (resource = 0; resource < RLIM_NLIMITS; ++resource) {

    if (-1 == getrlimit(resource, &limit)) {

        Log(LOG_ERR, "Couldn't get rlimit [%d]", resource);

    }

    limit.rlim_cur = limit.rlim_max;

    if (-1 == setrlimit(resource, &limit)) {

        Log(LOG_ERR, "Couldn't set rlimit [%d]", resource);

    }

  }

}

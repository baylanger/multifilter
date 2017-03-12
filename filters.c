/****
 * FILE: filters.c
**/

#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/errno.h>
#include <time.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/fd.h"
#include "include/filters.h"
#include "include/filter_t.h"
#include "include/signal.h"


extern struct filter_t Head;

extern int ArticleLength;
extern int HeaderLength;

extern int ReloadAtArticle;

extern int nFilters;

int nFiltersOut;

int TotalResponses = 0;
extern int TotalArticles;
extern boolean_t Sync;
extern timespec_t tv;

extern char *Article;

boolean_t Accept = TRUE;

extern boolean_t GotArticle;
extern int GotHFields;
extern char *ArticleLogFile;

extern pthread_cond_t  AH_c;
extern pthread_cond_t  AR_c;
extern pthread_cond_t  GHF_c;
extern pthread_cond_t  nR_c;

extern pthread_mutex_t AH_m;
extern pthread_mutex_t AR_m;
extern pthread_mutex_t GHF_m;
extern pthread_mutex_t nR_m;

extern pthread_cond_t  FO_c;
extern pthread_mutex_t FO_m;

extern char *MessageID;
extern char *Newsgroups;
extern FILE *ArticleLogFD;

pthread_mutex_t FI_m;
pthread_cond_t  FI_c;

int nFiltersIn = 0;


void
MessageLog(int Accept, char *MessageID) {

  if ((FILE *)NULL != ArticleLogFD) {

    if (TRUE == Accept) {

      fprintf(ArticleLogFD, "Accept:%s\n", MessageID);

    } else {

      fprintf(ArticleLogFD, "Refuse:%s\n", MessageID);

    }

  }

}


static int
DoFilter(filter_t *FilterPtr)
{
  static FILE *FD;

  if ((FILE *)NULL == FD) {

    if ((FILE *)NULL == (FD = FDOpen("../../log/groups", "a+"))) {

      Log(LOG_ERR, "Couldn't open & create groups");

    }

  }


  if (900000 < ArticleLength) {

    fprintf(FD, "%s %d\n", Newsgroups, ArticleLength);

  }

  if ((0 != FilterPtr->MaxLength) &&
       (ArticleLength >= FilterPtr->MaxLength)) {
       
    FilterPtr->OverLength++;
    return(FALSE);
    
  } 

  if (FALSE == Mulfnmatch(Newsgroups, FilterPtr->Subscription)) {

/*    FilterPtr->fnmatch++; */
    return(FALSE);

  }

  return(TRUE);

}



void *
FilterOut(void *voidFilter)
{

  int FdsOut;
  int ID;
  int Responses;
  boolean_t Body;
  filter_t *FilterPtr;
  pthread_t ID_t;
  pthread_attr_t attr;
  pthread_cond_t cond;
  pthread_mutex_t mutex;

  FilterPtr = voidFilter;
  FdsOut = FilterPtr->Fds[1];
  Body = FilterPtr->Body;
  ID = FilterPtr->ID;
  Responses = FilterPtr->Responses;
  cond = FilterPtr->cond;
  mutex = FilterPtr->mutex;
  ID_t = pthread_self();

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  while(1) {

    int nbytes;
    int sig_number;
    pthread_t ID1_t;

    nbytes = 0;


    pthread_mutex_lock(&GHF_m);
    while(FilterPtr->GotHFields >= GotHFields) {

      while(0 != pthread_cond_wait(&GHF_c, &GHF_m));

    }
    FilterPtr->GotHFields++;
    pthread_mutex_unlock(&GHF_m);

    FilterPtr->CurrentMessageID = (char *)strdup(MessageID);

    /* Est-ce vraiment bon? */
    if (TRUE == Sync) {

      pthread_mutex_lock(&FI_m);

      while(ID != nFiltersIn) {

        while(0 != pthread_cond_wait(&FI_c, &FI_m));

      }

      pthread_mutex_unlock(&FI_m);

    }


    /* Ready to start parsing the header */
    if (FALSE == DoFilter(FilterPtr)) {

      pthread_mutex_lock(&FI_m);
      while(Responses > TotalResponses) {

        while(0 != pthread_cond_wait(&FI_c, &FI_m));

      }
      Responses++;
      pthread_mutex_unlock(&FI_m);

      /* Skip the article */
      pthread_mutex_lock(&FO_m);
      nFiltersOut++;
      pthread_cond_signal(&FO_c);
      pthread_mutex_unlock(&FO_m);

      pthread_mutex_lock(&AR_m);
      while(Responses > TotalArticles) {

        while(0 != pthread_cond_wait(&AR_c, &AR_m));

      }
      pthread_mutex_unlock(&AR_m);

      ResponseWrite(FilterPtr);

      continue;

    } else {

      if (FALSE == Body) {

        if (HeaderLength != write(FdsOut, Article, HeaderLength)) {

          Log(LOG_ERR, "Couldn't write to <%s>", FilterPtr->Name);
          pthread_mutex_lock(&FI_m);
          while(Responses > TotalResponses) {

            while(0 != pthread_cond_wait(&FI_c, &FI_m));

          }
          Responses++;
          pthread_mutex_unlock(&FI_m);

          pthread_mutex_lock(&FO_m);
          nFiltersOut++;
          pthread_cond_signal(&FO_c);
          pthread_mutex_unlock(&FO_m);

          FilterPtr->Dead = TRUE;

          ResponseWrite(FilterPtr);

        }

        if (3 != write(FdsOut, ".\r\n", 3)) {

          Log(LOG_ERR, "Couldn't write to <%s>!", FilterPtr->Name);
          pthread_mutex_lock(&FI_m);
          while(Responses > TotalResponses) {

            while(0 != pthread_cond_wait(&FI_c, &FI_m));

          }
          Responses++;
          pthread_mutex_unlock(&FI_m);

          pthread_mutex_lock(&FO_m);
          nFiltersOut++;
          pthread_cond_signal(&FO_c);
          pthread_mutex_unlock(&FO_m);

          FilterPtr->Dead = TRUE;

          ResponseWrite(FilterPtr);

        }

      } else {

        while(1) {

          int Result;

          pthread_mutex_lock(&AR_m);

          if ((nbytes == ArticleLength) && (TRUE == GotArticle)) {

            pthread_mutex_unlock(&AR_m);

            break;

          }

          while((nbytes == ArticleLength) && (FALSE == GotArticle)) {

            while (0 != pthread_cond_wait(&AR_c, &AR_m)); 

          }

          Result = write(FdsOut, &Article[nbytes], ArticleLength - nbytes);

          pthread_mutex_unlock(&AR_m);

          nbytes += Result;

          if (ERROR == Result) {

            Log(LOG_ERR, "Couldn't write to <%s>!!", FilterPtr->Name);
            pthread_mutex_lock(&FI_m);
            while(Responses > TotalResponses) {

              while(0 != pthread_cond_wait(&FI_c, &FI_m));

            }
            Responses++;
            pthread_mutex_unlock(&FI_m);

            Log(LOG_ERR, "Couldn't write to <%s>!!!", FilterPtr->Name);
            pthread_mutex_lock(&FO_m);
            nFiltersOut++;
            pthread_cond_signal(&FO_c);
            pthread_mutex_unlock(&FO_m);

            FilterPtr->Dead = TRUE;

            Log(LOG_ERR, "Couldn't write to <%s>!!!!", FilterPtr->Name);

            ResponseWrite(FilterPtr);

          }


        } /* while (1) */

      } /* if */

    } /* else */

    pthread_mutex_lock(&FI_m);
    while(Responses > TotalResponses) {

      while(0 != pthread_cond_wait(&FI_c, &FI_m));

    }
    Responses++;
    pthread_mutex_unlock(&FI_m);

    pthread_mutex_lock(&FO_m);
    nFiltersOut++;
    pthread_cond_signal(&FO_c);
    pthread_mutex_unlock(&FO_m);

    pthread_mutex_lock(&FilterPtr->mutex);

    FilterPtr->WaitInput++;

    pthread_create(&ID1_t, &attr, FilterIn, FilterPtr);

    while (0 != FilterPtr->WaitInput) {

      if (ETIMEDOUT == pthread_cond_timedwait(&FilterPtr->cond,
                                              &FilterPtr->mutex,
                                              &tv)) {

        FilterPtr->Dead = TRUE;
        pthread_mutex_unlock(&FilterPtr->mutex);
        Log(LOG_ERR, "Forced to close external program <%s>",
                      FilterPtr->Name);
        break;

      }
      pthread_mutex_unlock(&FilterPtr->mutex);

    }

    ResponseWrite(FilterPtr);

  } /* End while(1) */

}



void *
FilterIn(void *voidFilter)
{

  char    response[11];
  ssize_t nbytes;
  int     result;

  filter_t *FilterPtr = (filter_t *)voidFilter;


  if (5 != (nbytes = read(FilterPtr->Fds[0], response, 10))) {

    if (0 >= nbytes) {

      pthread_mutex_lock(&FilterPtr->mutex);

      if (TRUE == FilterPtr->Dead) {

        FilterPtr->WaitInput--;
        pthread_cond_broadcast(&FilterPtr->cond);
        pthread_mutex_unlock(&FilterPtr->mutex);
        pthread_exit(NULL);

      }

      pthread_mutex_unlock(&FilterPtr->mutex);

      Log(LOG_CRIT, "FO[%d]: Dropping filter <%s> - read() == %d",
                     pthread_self(), FilterPtr->Name, nbytes);
     
      pthread_mutex_lock(&FilterPtr->mutex);
      FilterPtr->Dead = TRUE;
      FilterPtr->WaitInput--;
      pthread_cond_broadcast(&FilterPtr->cond);
      pthread_mutex_unlock(&FilterPtr->mutex);
      pthread_exit(NULL);

    } else {

      response[nbytes] = '\0';

      Log(LOG_CRIT, "FO:[%d] Dropping filter <%s> - Got %d chars <%s>",
                     pthread_self(), FilterPtr->Name, nbytes, response);

      pthread_mutex_lock(&FilterPtr->mutex);
      FilterPtr->Dead = TRUE;
      FilterPtr->WaitInput--;
      pthread_cond_broadcast(&FilterPtr->cond);
      pthread_mutex_unlock(&FilterPtr->mutex);
      pthread_exit(NULL);

    }

  }

  pthread_mutex_lock(&FilterPtr->mutex);
  if (TRUE == FilterPtr->Dead) {

    FilterPtr->WaitInput--;
    pthread_cond_broadcast(&FilterPtr->cond);
    pthread_mutex_unlock(&FilterPtr->mutex);
    pthread_exit(NULL);

  }
  pthread_mutex_unlock(&FilterPtr->mutex);


response[nbytes] = '\0';

  if (0 == strncmp(response, REFUSE, RESPONSE_LENGTH)) {

      pthread_mutex_lock(&FilterPtr->mutex);
      FilterPtr->Refused++;
      FilterPtr->WaitInput--;
      pthread_mutex_lock(&FI_m);
      if (TRUE == Accept) {

        Accept = FALSE;

        write(STDOUT_FILENO, REFUSE, RESPONSE_LENGTH);

        MessageLog(Accept, FilterPtr->CurrentMessageID);
        free(FilterPtr->CurrentMessageID);


      }
      pthread_cond_broadcast(&FI_c);
      pthread_mutex_unlock(&FI_m);
      pthread_cond_broadcast(&FilterPtr->cond);
      pthread_mutex_unlock(&FilterPtr->mutex);
      pthread_exit(NULL);

  }

  if (0 == strncmp(response, ACCEPT, RESPONSE_LENGTH)) {

    pthread_mutex_lock(&FilterPtr->mutex);
    FilterPtr->Accepted++;
    FilterPtr->WaitInput--;
    pthread_cond_broadcast(&FilterPtr->cond);
    pthread_mutex_unlock(&FilterPtr->mutex);
    pthread_exit(NULL);

  } else {

    response[5] = '\0';

    Log(LOG_CRIT, "Dropping filter <%s> - Bad response <%s>",
                   FilterPtr->Name, response);

    pthread_mutex_lock(&FilterPtr->mutex);
    FilterPtr->Dead = TRUE;
    FilterPtr->WaitInput--;
    pthread_cond_broadcast(&FilterPtr->cond);
    pthread_mutex_unlock(&FilterPtr->mutex);
    pthread_exit(NULL);

  }

  pthread_cond_broadcast(&FilterPtr->cond);
  pthread_mutex_unlock(&FilterPtr->mutex);
  pthread_exit(NULL);

}



/*********************************************************************
 *
 *
**/
static void
ResponseWrite(filter_t *FilterPtr)
{

  pthread_t ID_t = pthread_self();
    
  pthread_mutex_lock(&FI_m);

  nFiltersIn++;

  if (nFiltersIn == nFilters) {

    if (Accept == TRUE) { 

      write(STDOUT_FILENO, ACCEPT, RESPONSE_LENGTH);

      MessageLog(Accept, FilterPtr->CurrentMessageID);
      free(FilterPtr->CurrentMessageID);

    } else { 
      
      Accept = TRUE;
      
    } 

    nFiltersIn = 0;
    TotalResponses++;

    if (ReloadAtArticle == TotalResponses) { 

      pthread_cond_broadcast(&FI_c);
      pthread_mutex_unlock(&FI_m);
      ReadyForReload();
      pthread_exit(NULL);
    
    }
    
  } else { 

    if (ReloadAtArticle == (TotalResponses + 1)) { 

      pthread_cond_broadcast(&FI_c);
      pthread_mutex_unlock(&FI_m);
      ReadyForReload();
      close(FilterPtr->Fds[0]);
      close(FilterPtr->Fds[1]);

      pthread_exit(NULL);
       
    }
     
  } 

  if (TRUE == FilterPtr->Dead) {
     
    if (0 != nFiltersIn) {

      nFiltersIn--;

    }

    nFilters--;

    pthread_cond_broadcast(&FI_c);
    pthread_mutex_unlock(&FI_m);

    close(FilterPtr->Fds[0]);
    close(FilterPtr->Fds[1]);

    FilterPtr->ID = ERROR;
    FilterPtr->ID_t = ERROR;

    pthread_exit(NULL);
  
  }
    
  pthread_cond_broadcast(&FI_c);
  pthread_mutex_unlock(&FI_m);

}

/************************************************************************
 *
 * article-read: article-read.c
 *
 * DESC: Read an article
 *
**/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <syslog.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "include/defaults.h"
#include "include/article-read.h"
#include "include/func.h"
#include "include/main.h"


extern size_t PageSize;

extern int nFilters;
extern int ReloadAtArticle;
extern int TotalResponses;

char      *Article;
size_t     ArticleLength = (size_t)0;
boolean_t  GoArticleHeader = FALSE;
boolean_t  GotArticle = FALSE;

pthread_cond_t AH_c;  # Article Headers
pthread_cond_t AR_c;  # Article Read
pthread_cond_t GAH_c; # Got Article Header
pthread_cond_t GHF_c; # Got Header Fields
pthread_cond_t nR_c;
pthread_cond_t FO_c;

pthread_mutex_t AH_m;
pthread_mutex_t AR_m;
pthread_mutex_t GAH_m;
pthread_mutex_t GHF_m;
pthread_mutex_t nR_m;
pthread_mutex_t FO_m;

extern pthread_cond_t  FI_c;
extern pthread_mutex_t FI_m;

size_t HeaderLength;

extern int nFiltersOut;
int        TotalArticles = 0;
int        GotHFields = 0;

timespec_t tv;

char *Subject = (char *)NULL;
char *Newsgroups = (char *)NULL;
char *MessageID = (char *)NULL;


/************************************************************************
 *
 * NAME: ArticleHeaderParse
 * 
 * DESC: 
 * 
**/
int
ArticleHeaderParse()
{
  size_t i, j;
  char *end;
  int n_object = 3; /* Newsgroups + Subject */

  i = 0;

  pthread_mutex_lock(&AR_m);

  if (0 == strncasecmp(Article, "Newsgroups:", 11)) {

    end = strchr(&Article[11], '\r');
    *end = '\0';
    for(j = 11; ('\0' != Article[j]) && (' ' == Article[j]); j++);

    Newsgroups = strdup(&Article[j]);
    *end = '\r';
    n_object--;
    i = 11 + strlen(Newsgroups);

  }

  if (0 == strncasecmp(Article, "Subject:", 8)) {
 
    end = strchr(&Article[8], '\r');
    *end = '\0';
    for(j = 8; ('\0' != Article[j]) && (' ' == Article[j]); j++);

    Subject = strdup(&Article[j]);

    *end = '\r';
    n_object--;
    i = 8 + strlen(Subject);

  }

  if (0 == strncasecmp(Article, "Message-ID:", 11)) {

    end = strchr(&Article[11], '\r');
    *end = '\0';
    for(j = 11; ('\0' != Article[j]) && (' ' == Article[j]); j++);

    MessageID = strdup(&Article[j]);

    *end = '\r';
    n_object--;
    i = 11 + strlen(MessageID);

  }


  for (; (i < ((HeaderLength - 10)) && (0 != n_object)); i++) {

    if (((char *)NULL == Newsgroups) && ((ArticleLength - i) > 13)) {

      if (0 == strncasecmp(&Article[i], "\r\nNewsgroups:", 13)) {

        end = strchr(&Article[i+1], '\r');
        *end = '\0';
        for(j = i + 13; ('\0' != Article[j]) && (' ' == Article[j]); j++);

        Newsgroups = strdup(&Article[j]);
        *end = '\r';
        n_object--;
        i += 13;

        continue;

      }

    }

    if (((char *)NULL == Subject) && ((ArticleLength - i) > 10)) {

      if (0 == strncasecmp(&Article[i], "\r\nSubject:", 10)) {
 
        end = strchr(&Article[i+1], '\r');
        *end = '\0';
        for(j = i + 10; ('\0' != Article[j]) && (' ' == Article[j]); j++);

        Subject = strdup(&Article[j]);

        *end = '\r';
        n_object--;
        i += 10;

        continue;

      }

    }

    if (((char *)NULL == MessageID) && ((ArticleLength - i) > 13)) {

      if (0 == strncasecmp(&Article[i], "\r\nMessage-ID:", 13)) {
 
        end = strchr(&Article[i+1], '\r');
        *end = '\0';
        for(j = i + 13; ('\0' != Article[j]) && (' ' == Article[j]); j++);

        MessageID = strdup(&Article[j]);

        *end = '\r';
        n_object--;
        i += 13;

        continue;

      }

    }

  }

  if (0 != n_object) {

      Log(LOG_ERR, "-MAJOR ERROR-\n");
    if (NULL != Subject)
      Log(LOG_ERR, "-Subject [%s]\n", Subject);
    if (NULL != MessageID)
      Log(LOG_ERR, "-MessageID [%s]\n", MessageID);
    if (NULL != Newsgroups)
      Log(LOG_ERR, "-Newsgroups [%s]\n", Newsgroups);

    exit(EXIT_FAILURE);

  }

  pthread_mutex_unlock(&AR_m);
  return(OK);

}


/************************************************************************
 * 
 * NAME: ArticleHeader
 * 
 * DESC: Get the length of the header
 *
**/
void *
ArticleHeader(void *argv)
{ 
  size_t i;

  while(1) {

    size_t LastCheck;

    LastCheck = (size_t)0;
    HeaderLength = (size_t)0;

    /* Wait for ArticleRead to read first block from the article */
    pthread_mutex_lock(&GAH_m);
    while(FALSE == GoArticleHeader) {

      while(0 != pthread_cond_wait(&GAH_c, &GAH_m));

    }
    GoArticleHeader = FALSE;
    pthread_mutex_unlock(&GAH_m);

    while(0 == HeaderLength) {

      pthread_mutex_lock(&AR_m);

      if ((LastCheck == ArticleLength) && (TRUE == GotArticle)) {

        break;

      }

      while((LastCheck == ArticleLength) || (ArticleLength < EOHL)) {

        while(0 != pthread_cond_wait(&AR_c, &AR_m));

      }
      pthread_mutex_unlock(&AR_m);

      LastCheck = (LastCheck > EOHL) ? LastCheck - EOHL -1  : 0;
      for (i = LastCheck; (i <= (ArticleLength - EOHL)); i++) {

        if (0 == strncmp(&Article[i], EOH, EOHL)) {

          HeaderLength = i + EOHL - 1;

          break;

        }

        LastCheck = i + EOHL;

      }

    }

    if (0 == HeaderLength) {

      int checkEOA = LastCheck - EOAL;

      if(0 == strncmp(&Article[checkEOA], EOA, EOAL)) {

        HeaderLength = checkEOA + 1;

      } else {

Log(LOG_ERR, "AH - SOMETHING IS WRONG\n");
        TotalArticles--;
        DoExit(EXIT_FAILURE);

      }

    }

/*
write(STDERR_FILENO, Article, HeaderLength);
*/

    ArticleHeaderParse();

    pthread_mutex_lock(&GHF_m);
    GotHFields++;
    pthread_cond_broadcast(&GHF_c);
    pthread_mutex_unlock(&GHF_m);

  }

}


/************************************************************************
 * 
 * NAME: ArticleRead
 * 
 * DESC: Read an article from the STDIN_FILENO and return
 *       a char * to the buffer
 *
**/
void *
ArticleRead(void *arg)
{ 
  ssize_t ReadLength;
  size_t  TotalAllocated;
  
  pthread_mutex_unlock(&nR_m);
  
  /* Ready */
  while (1) {
    
    int nRead;
    
    nRead = 0;
    
    if ((char *)NULL == (Article = (char *)malloc(PageSize))) {
      
      syslog(LOG_CRIT, "ArticleRead - malloc() error");
      DoExit(EXIT_FAILURE);
    
    }


    if (ReloadAtArticle == TotalArticles) {

Log(LOG_ERR, "RESTART: Article-read [%d]\n", ReloadAtArticle);
fflush(stderr);

      ReadyForReload();

      pthread_mutex_lock(&GAH_m);

      while(0 != ReloadAtArticle) {

        while(0 != pthread_cond_wait(&GAH_c, &GAH_m));

      }
      pthread_mutex_unlock(&GAH_m);

    }

    
    TotalAllocated = PageSize;
    
    while(1) {
      
      nRead++;
      
      if (0 >= (ReadLength = read(STDIN_FILENO,
                                  &Article[ArticleLength], PageSize))){

        pthread_mutex_lock(&FI_m);
        while(TotalArticles < TotalResponses) {

    Log(LOG_ERR, "AR: TotalArticles != TotalResponses");
          while(0 != pthread_cond_wait(&FI_c, &FI_m));

        }
        pthread_mutex_unlock(&FI_m);
    Log(LOG_ERR, "AR: TotalArticles == TotalResponses");

        if (0 > ReadLength) {

          Log(LOG_ERR, "ArticleRead - read() returned [%d]", ReadLength);
          free(Article);
          free(Subject);

          DoExit(EXIT_FAILURE);

        }

        free(Article);
        free(Subject);

        DoExit(EXIT_SUCCESS);

      }
      pthread_mutex_lock(&AR_m);
      ArticleLength += ReadLength;
      pthread_cond_broadcast(&AR_c);
      pthread_mutex_unlock(&AR_m);

      if ((1 == nRead) && (0 != nFilters)) {

        pthread_mutex_lock(&GAH_m);
        GoArticleHeader = TRUE;
        pthread_cond_signal(&GAH_c);
        pthread_mutex_unlock(&GAH_m);

      }

      /* check for EOA */
      if ((ArticleLength >= EOAL) &&
          (0 == strncmp(&Article[ArticleLength - EOAL], EOA, EOAL))) {

        tv.tv_sec = time(NULL) + SERVERTIMEOUT - 5;
        tv.tv_nsec = 0;

        pthread_mutex_lock(&AR_m);
        GotArticle = TRUE;
        TotalArticles++;

/*
write(STDERR_FILENO, Article, ArticleLength);
*/

        pthread_cond_broadcast(&AR_c);
        pthread_mutex_unlock(&AR_m);

        pthread_mutex_lock(&FO_m);

        if (0 == nFilters) {

          write(STDOUT_FILENO, ACCEPT, RESPONSE_LENGTH);

        } else {

          while(nFiltersOut < nFilters) {

            while (0 != pthread_cond_wait(&FO_c, &FO_m));

          }

          nFiltersOut = 0;

        }

        ArticleLength = 0;
        GotArticle = FALSE;
        free(Article);
        Article = (char *)NULL;
        free(Newsgroups);
        Newsgroups = (char *)NULL;
        free(Subject);
        Subject = (char *)NULL;
        free(MessageID);
        MessageID = (char *)NULL;
        TotalAllocated = 0;

        pthread_mutex_unlock(&FO_m);

        break;

      } else {

        if (ArticleLength + PageSize > TotalAllocated) {

          TotalAllocated += PageSize;

          pthread_mutex_lock(&AR_m);
          if ((char *)NULL==(Article=(char *)realloc(Article,TotalAllocated))) {

            syslog(LOG_CRIT, "ArticleRead - realloc() error");

            pthread_mutex_lock(&AR_m);
            DoExit(EXIT_FAILURE);

          }
          pthread_mutex_unlock(&AR_m);

        }

      }

    }

  }

}

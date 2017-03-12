#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/errno.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/filter_t.h"

/******************************************************************
*
 * Case sensitive
*/
int
strwildcmp(const char *pattern, const char *string)
{

  if ((((char *)NULL) == pattern) || ((char *)NULL == string)) {

    return(1);

  }

  if (0 == fnmatch(pattern, string, 0)) {

    return(0);

  } else {

    return(1);

  }

}

int
Mulfnmatch(char *Newsgroups, list_t *Subscription[])
{
  int n;
  list_t **list;

  for(n = 0, list = Subscription;
      NULL != list[n];
      n++) {

    if (0 == strwildcmp(list[n]->Subscription, Newsgroups)) {

      if  (TRUE == list[n]->Not) {

        return(FALSE);

      } else {

        return(TRUE);

      }

    }

  }

  return(FALSE);

}


list_t **
MakeList(char *Subscription)
{
  int n;
  boolean_t Not;
  char *xtoken;
  list_t **List;

  List = (list_t **)malloc(sizeof(list_t **));
  List[0] = NULL;


  for(n = 0, xtoken = (char *)strtok(Subscription, ",");
      xtoken != (char *)NULL;
      xtoken = (char *)strtok(NULL, ","), n++) {

    List = (list_t **)realloc(List, sizeof(list_t **) * (n + 1));

    List[n] = (list_t *)malloc(sizeof(list_t));

    if ('!' == *xtoken) {

      List[n]->Not = TRUE;
      xtoken++;
  
    } else {
    
      List[n]->Not = FALSE;
    
    }

    List[n]->Subscription = strdup(xtoken);

  }

  List = (list_t **)realloc(List, sizeof(list_t *) * (n + 1));
  List[n] = NULL;

  return(List);

}

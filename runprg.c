/************************************************************************
 *
 * FILE: runprg.c
 *
 * DESC: exec external program and set pipes
 *
**/

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/filters.h"
#include "include/func.h"
#include "include/runprg.h"


int nFilters;



/************************************************************************
 *
 * NAME: StartFilters
 *
 * DESC: Try to start programs one by one
 *
**/
int
StartFilters(filter_t *Head)
{
  filter_t *FilterPtr;

 
  nFilters = 0;

  for(FilterPtr = Head->next;
      FilterPtr != Head;
      FilterPtr = FilterPtr->next) {

    pid_t pid;
    char *argv[3];

    argv[0] = FilterPtr->Program;
    argv[1] = FilterPtr->Program;
    argv[2] = (char *)NULL;
  
    if (0 >= (pid = RunPrg(FilterPtr->Fds, argv))){

      Log(LOG_ERR, "Couldn't start filter program <%s>", FilterPtr->Program);

      FreeFilter(FilterPtr);

      if (0 == pid) {

        exit(127);

      }

      continue;

    } else {

      pthread_attr_t attr;

      pthread_t threadID;

      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

      FilterPtr->pid = pid;
      FilterPtr->Dead = FALSE;

fprintf(stderr, "runprg: create FilterOut [%s]\n", FilterPtr->Name);

      pthread_create(&threadID, &attr, FilterOut, FilterPtr);

      FilterPtr->ID = nFilters++;
      FilterPtr->ID_t = threadID;

    }

  }

  return(OK);

}



/************************************************************************
 *
 * NAME: RunPrg
 *
 * DESC: exec a program
 *
**/
static pid_t
RunPrg(int rfds[3], char *argv[])
{
  int fds[4] = { -1, -1, -1, -1 };
  pid_t pid = 0;

 
  if (0 != pipe(&fds[0])) {

    Log(LOG_CRIT, "pipe() != 0");
    return(ERROR);

  }

  if (0 != pipe(&fds[2])) {

    Log(LOG_CRIT, "pipe() != 0");
    return(ERROR);

  }

  (void)fflush(stdout);
  (void)fflush(stderr);

  if (0 == (pid = fork())) {

    int i,
        fd;

    if (-1 == dup2(fds[2], 0)) {

      Log(LOG_CRIT, "dup2() == -1");
      return(ERROR);

    }

    if (-1 == dup2(fds[1], 1)) {

      Log(LOG_CRIT, "dup2() == -1");
      return(ERROR);

    }

    fd = open("/dev/null", O_RDWR);

    if (0 <= fd) {

      if (2 != fd) {

        if (-1 == dup2(fd, 2)) {

          Log(LOG_CRIT, "dup2() == -1");
          return(ERROR);

        }

        (void)close(fd);

      }

    }

    {
      int i;

      for (i = 0; 4 > i; i++) {

        close(fds[i]);

      }

    }


    if (-1 == execv(argv[0], argv + 1)) {

      return(0);

    }

    exit(EXIT_FAILURE);

  }

  if (0 < pid) {

      rfds[0] = fds[0];
      fds[0] = -1;
      rfds[1] = fds[3];
      fds[3] = -1;

  }

  if (0 > pid) {

    return(ERROR);

  }

  (void)close(fds[1]);
  (void)close(fds[2]);

  return(pid);

}

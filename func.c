/************************************************************************
 *
 * FILE: func.c
 *
 * DESC: All kind of functions
 *
**/

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>

#include "include/defaults.h"
#include "include/func.h"



/************************************************************************
 *
 * NAME: Log
 *
 * DESC: Do the log / fprintf to stderr except LOG_INFO
 *
**/
void
Log(int priority, const char *ctl, ...)
{
  int nbyte;
  char message[2048];
  va_list va;


  va_start(va, ctl);
  nbyte = vsnprintf(message, sizeof(message), ctl, va);
  va_end(va);

  syslog(priority, "%s", message);

  if (LOG_INFO != priority) {

    (void)write(STDERR_FILENO, message, nbyte);
    (void)write(STDERR_FILENO, "\n", 1);

  }

}

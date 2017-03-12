/************************************************************************
 *
 * FILE: pid.c
 *
 * DESC: Create/delete the PID file
 *
**/

#include <stdio.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include "include/defaults.h"
#include "include/fd.h"
#include "include/func.h"
#include "include/pid.h"


#define CREATE 0
#define DELETE 1

extern char *PidFile;



/************************************************************************
 * FUNC: PID
 *
 * DESC: Open the PID file, on success return OK
 *
 *  ERR: return ERROR
**/
static int
PID(const int Mode)
{
  static int created = FALSE;
  FILE *FD;
  struct stat Stat;


  switch (Mode) {

    case CREATE:

      if (0 == stat(PidFile, &Stat)) {

        Log(LOG_ERR, "PID file [%s] Exists", PidFile);
        return(ERROR);

      }
  
      if ((FILE *)NULL == (FD = FDOpen(PidFile, "w"))) {

        Log(LOG_ERR, "Couldn't open & create PID file [%s]", PidFile);
        return(ERROR);

      }
  
      if (0 == fprintf(FD, "%d\n", (int)getpid())) {

        Log(LOG_ERR, "Couldn't write to PID file [%s]", PidFile);

        if (ERROR == FDClose(FD)) {

          Log(LOG_ERR, "Couldn't close PID file [%s]", PidFile);

        }

        (void)unlink(PidFile);
        (void)FDClose(FD);
        return(ERROR);

      }

      if (ERROR == FDClose(FD)) {

        Log(LOG_ERR, "Couldn't close PID file [%s]!", PidFile);
        return(ERROR);

      }

      created = TRUE;

    break;

    case DELETE:

      if (FALSE == created) {

        return(OK);

      }

      if (stat(PidFile, &Stat) < 0) {

        Log(LOG_ERR, "Couldn't find [%s].  PID file is gone", PidFile);
        return(ERROR);

      }

      if (0 != unlink(PidFile)) {

        Log(LOG_ERR, "Couldn't unlink PID [%s]", PidFile);
        return(ERROR);

      }

    break;

    default:

      Log(LOG_ERR, "PID unknown switch <%d>", Mode);
      return(ERROR);

    break;

  }

  return(OK);

}



/************************************************************************
 *
 * FUNC: PID_Open
 *
 * DESC: Open the PID file, on success return OK
 *
**/
int
PID_Open()
{

  return(PID(CREATE));

}



/************************************************************************
 *
 * FUNC: PID_Close
 *
 * DESC: ClosePID the PID file, on success return OK
 *
**/
int
PID_Close()
{

  return(PID(DELETE));

}

/************************************************************************
 *
 * FILE: fd.c
 *
 * DESC: Functions to open, read, close
 *
 *
**/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "include/defaults.h"
#include "include/fd.h"
#include "include/func.h"
#include "include/main.h"



/************************************************************************
 *
 * NAME: FDOpen()
 *
 * DESC: Open a file
 *
**/
FILE *
FDOpen(char *filename, char *mode)
{
  FILE *File;


  if ((FILE *)NULL != (File = fopen(filename, mode))) {

    return(File);

  } else {

    Log(LOG_CRIT, "FDOpen - Couldn't fopen() file <%s>", filename);
    return((FILE *)NULL);

  }

}



/************************************************************************
 *
 * NAME: FDClose()
 * 
 * DESC: Close a file
 *
**/
int
FDClose(FILE *fd)
{

  if (EOF == fclose(fd)) {

    Log(LOG_CRIT, "FDClose - Couldn't close FD");
    return(ERROR);

  } else {

    return(OK);

  }

}



/************************************************************************
*
 * NAME: FDReadLine()
 *
 * DESC: Read from a File Descriptor until a End Of Line.
 *
**/
size_t
FDReadLine(char **Buffer, FILE *fd)
{
  extern size_t PageSize;

  if ((char *)NULL == (*Buffer = (char *)malloc((size_t)PageSize))) {

    syslog(LOG_CRIT, "FDReadLine - Couldn't malloc");
    DoExit(EXIT_FAILURE);

  }

  if ((char *)NULL == fgets(*Buffer, (int)PageSize-1, fd)) {

    free(*Buffer);
    return(0);

  }

  return(strlen(*Buffer));

}

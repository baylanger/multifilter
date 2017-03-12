#ifndef __FD_H__
#define __FD_H__

#include <stdio.h>

/* Prototypes */
FILE  *FDOpen(char *filename, char *mode);
int    FDClose(FILE *fd);
size_t FDReadLine(char **Buffer, FILE *fd);

#endif

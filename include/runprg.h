#ifndef __RUNPRG_H__
#define __RUNPRG_H__

#include "filter_t.h"

/* Prototypes */
       int   StartFilters(filter_t *Head);
static pid_t RunPrg(int rfds[3], char *argv[]);

#endif

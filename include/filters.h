#ifndef __Filters_H__
#define __Filters_H__

#include "filter_t.h"

static int DoFilter(filter_t *FilterPtr);
static void ResponseWrite(filter_t *FilterPtr);

void *FilterOut(void *voidFilter);
void *FilterIn(void *voidFilter);

#endif

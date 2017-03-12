/************************************************************************
 *
 * FILE: parse.c
 *
 * DESC: Parse/Validate the configuration file
 *
 *
**/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#include "include/defaults.h"
#include "include/filter_t.h"
#include "include/fd.h"
#include "include/func.h"
#include "include/main.h"
#include "include/parse.h"



/************************************************************************
 *
 * NAME: FilterValidate
 *
 * DESC: Validate a filter object
 *
**/
static int
FilterValidate(filter_t *FilterPtr)
{
  struct stat Stat;


  if ((char *)NULL == FilterPtr->Name) {

    Log(LOG_ERR, "Missing Name in filter object");
    return(ERROR);

  }

  if (ERROR == FilterPtr->Body) {

    Log(LOG_ERR, "Missing Body in filter <%s>", FilterPtr->Name);
    return(ERROR);

  }

  if (ERROR == FilterPtr->Timeout) {

    Log(LOG_ERR, "Missing Timeout in filter <%s>", FilterPtr->Name);
    return(ERROR);

  }

  if (ERROR == FilterPtr->MaxLength) {

    Log(LOG_ERR, "Missing MaxLength in filter <%s>", FilterPtr->Name);
    return(ERROR);

  }

  if ((char *)NULL == FilterPtr->Program) {

    Log(LOG_ERR, "Missing Program in filter <%s>", FilterPtr->Name);
    return(ERROR);

  }

  if ((list_t **)NULL == FilterPtr->Subscription) {

    Log(LOG_ERR, "Missing Subscription in filter <%s>", FilterPtr->Name);
    return(ERROR);

  }

  if (ERROR == stat((const char *)FilterPtr->Program, &Stat)) {

    Log(LOG_ERR, "Couldn't open program <%s> in filter <%s>",
                  FilterPtr->Program, FilterPtr->Name);
    return(ERROR);

  } else {

    int mode = Stat.st_mode;

    if (!((mode & S_IFREG) && ((mode & S_IXUSR) || (mode & S_IXGRP)))) {

      Log(LOG_ERR, "Can't execute program <%s> in filter <%s>!",
                    FilterPtr->Program, FilterPtr->Name);
      return(ERROR);

    }

  }

  return(OK);

}


/************************************************************************
 *
 * NAME: Parse
 *
 * DESC: Parse the configuration file and fill the filter objects
 *
**/
int
Parse(filter_t *HeadPtr, char *configfile)
{
  int    InObject = 0,
         LineCount = 1,
         BeginFilter = 1,
         TotalTimeout = 0;
  char  *Buffer,
        *Strip,
        *Directive,
        *Value;
  size_t Length;
  filter_t *FilterPtr = (filter_t *)NULL;
  FILE  *File;


  if ((FILE *)NULL == (File = FDOpen(configfile ,"r"))) {

    Log(LOG_CRIT, "FDOpen - Couldn't open file <%s>", configfile);
    return(ERROR);

  }

  for(; (0 < FDReadLine(&Buffer, File)); LineCount++) {

    Strip = Buffer;
    for(; (0 != isspace(*Strip)); Strip++);

    if (('#' == *Strip) || ('\0' == *Strip)) {

      free(Buffer);
      continue;

    }

    Length = strlen(Strip);

    for(Length--;
        ((0 <= Length) && (0 != isspace(Strip[Length])));
        Strip[Length]='\0',Length--);

    if (FALSE == InObject) {

      if (0 == strcasecmp(Strip, "<filter>")) {

        free(Buffer);

        InObject = TRUE;
        BeginFilter = LineCount;

        if ((filter_t *)NULL ==
                   (FilterPtr = (filter_t *)NewFilter())) {


          syslog(LOG_CRIT, "Parse NewFilter() error");

          (void)FDClose(File);

          DoExit(EXIT_FAILURE);

        }

        continue;

      } else {

        free(Buffer);
        Log(LOG_ERR, "Missing <Filter> / Unknown Directive line #%d",
                      LineCount);

        (void)FDClose(File);

        return(ERROR);

      }

    } else {

      if (0 == strcasecmp(Strip, "</filter>")) {

        free(Buffer);

        if (ERROR == FilterValidate(FilterPtr)) {

          Log(LOG_ERR, "Invalid filter line #%d", BeginFilter);
          FreeFilter(FilterPtr);
          AllFreeFilter(HeadPtr);
          (void)FDClose(File);
          return(ERROR);

        } else {

          if (ERROR == InsertFilter(HeadPtr, FilterPtr)) {

            Log(LOG_ERR, "Invalid filter line #%d", BeginFilter);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          } else {

            InObject = FALSE;
            continue;

          }

        }

      }

      if (((char *)NULL != (Directive = strtok(Strip, " \t")))) {

        Value = &(Directive[strlen(Directive) + 1]);
        Value = strtok(Value, " \t");

        if (0 == strcasecmp(Directive, "name")) {

          if ((char *)NULL == Value) {

            Log(LOG_ERR, "No value for Directive Name in line #%d",
                         LineCount);
            free(Buffer);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          } else {

            FilterPtr->Name = strdup(Value);
            free(Buffer);
            continue;

          }

        }

        if (0 == strcasecmp(Directive, "program")) {

          
          if ((char *)NULL == Value) {

            Log(LOG_ERR, "No value for Directive Program in line #%d",
                         LineCount);
            free(Buffer);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          } else {

            FilterPtr->Program = strdup(Value);
            free(Buffer);
            continue;

          }

        }

        if (0 == strcasecmp(Directive, "subscription")) {

          
          if ((char *)NULL == Value) {

            Log(LOG_ERR, "No value for Directive Subscription in line #%d",
                         LineCount);
            free(Buffer);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          } else {

            /* FilterPtr->Subscription = strdup(Value); */
            FilterPtr->Subscription = MakeList(Value);
            free(Buffer);
            continue;

          }

        }

        if (0 == strcasecmp(Directive, "timeout")) {

          if ((char *)NULL == Value) {

            Log(LOG_ERR, "No value for Directive Timeout in line #%d",
                         LineCount);
            free(Buffer);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          } else {

            FilterPtr->Timeout = atoi(Value);
            free(Buffer);

            if (0 >= FilterPtr->Timeout) {

              Log(LOG_ERR, "Illegal value - Timeout line #%d", LineCount);
              FreeFilter(FilterPtr);
              AllFreeFilter(HeadPtr);
              (void)FDClose(File);
              return(ERROR);

            }

            TotalTimeout += FilterPtr->Timeout;

            continue;

          }

        }

        if (0 == strcasecmp(Directive, "maxlength")) {

          if ((char *)NULL == Value) {

            Log(LOG_ERR, "No value for Directive MaxLength in line #%d",
                         LineCount);
            free(Buffer);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          } else {

            FilterPtr->MaxLength = atoi(Value);
            free(Buffer);

            if (0 > FilterPtr->MaxLength) {

              Log(LOG_ERR, "Illegal value - MaxLength line #%d", LineCount);
              FreeFilter(FilterPtr);
              AllFreeFilter(HeadPtr);
              (void)FDClose(File);
              return(ERROR);

            }

            continue;

          }

        }

        if (0 == strcasecmp(Directive, "body")) {

          if ((char *)NULL == Value) {

            Log(LOG_ERR, "No value for Directive Body in line #%d", LineCount);

            free(Buffer);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          } else {

            if ((0 == strcasecmp(Value, "true") ||
                (0 == strcasecmp(Value, "yes")))) {

              free(Buffer);
              FilterPtr->Body = TRUE;
              continue;

            }

            if ((0 == strcasecmp(Value, "false") ||
                (0 == strcasecmp(Value, "no")))) {

              free(Buffer);
              FilterPtr->Body = FALSE;            
              continue;

            } 

            Log(LOG_ERR, "Error Body <%s> line #%d", Value, LineCount);

            free(Buffer);
            FreeFilter(FilterPtr);
            AllFreeFilter(HeadPtr);
            (void)FDClose(File);
            return(ERROR);

          }

        }

      }

      if (0 == strcasecmp(Directive, "<filter>")) {

        Log(LOG_ERR, "Missing </filter> for filter starting in line #%d",
                     BeginFilter);
        free(Buffer);
        FreeFilter(FilterPtr);
        AllFreeFilter(HeadPtr);
        (void)FDClose(File);
        return(ERROR);

      } else {

        Log(LOG_ERR, "Unknown Directive <%s> line #%d", Directive, LineCount);
        free(Buffer);
        FreeFilter(FilterPtr);
        AllFreeFilter(HeadPtr);
        (void)FDClose(File);
        return(ERROR);

      }

    }

  }

  (void)FDClose(File);

  if (TRUE == InObject) {

    FreeFilter(FilterPtr);
    AllFreeFilter(HeadPtr);
    Log(LOG_ERR, "Missing </filter> for filter starting at line #%d",
                 BeginFilter);
    return(ERROR);

  }

  if (TotalTimeout > SERVERTIMEOUT) {

    Log(LOG_INFO, "Warning: TotalTimeout [%d] > ServerTimeout [%d]",
                  TotalTimeout, SERVERTIMEOUT);

  }

  return(OK);

}

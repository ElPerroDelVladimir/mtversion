#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "mtversion.h"

static inline const char* get_time(void)
{
  static char tbuf[32];
  time_t curtime = time(NULL);
  struct tm *ptm = localtime(&curtime);

  if(ptm == NULL || strftime(tbuf, sizeof(tbuf), "%F %T", ptm) == 0) {
      sprintf(tbuf, "%s", "GETTIMEERROR");
  }
  return tbuf;
}



int mtv_logger(log_level_t lvl, const char *msg, ...)
{
  FILE *flog = fopen(MTV_FILE_LOG, "a+");

  if(likely(flog)) {
    char *loglvl;
    va_list args;
    const char *curtime = get_time();
    switch(lvl) {
      case ERROR:
        loglvl = "  ERROR  ";
        break;
      case WARN:
        loglvl = " WARNING ";
        break;
      case INFO:
        loglvl = "   INFO  ";
        break;
      default:
        loglvl = " UNKNOWN ";
        break;
    }
    fprintf(flog, "%s @ %s : ", curtime, loglvl);
    va_start(args, msg);
    vfprintf(flog, msg, args);
    va_end(args);
    fprintf(flog, "\n");
    fclose(flog);

    return 0;
  }
  return 1;
}

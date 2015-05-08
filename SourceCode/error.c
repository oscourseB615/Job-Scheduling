#include <string.h>
#include <errno.h>
#include "job.h"

/* ´íÎó´¦Àí */
void error_doit(int errnoflag,const char *fmt,va_list ap)
{
  int errno_save;
  char buf[BUFLEN];

  errno_save=errno;
  vsprintf(buf,fmt,ap);
  if (errnoflag)
    sprintf(buf+strlen(buf),":%s",strerror(errno_save));
  strcat(buf,"\n");
  fflush(stdout);
  fputs(buf,stderr);
  fflush(NULL);
  return;
}

void error_sys(const char *fmt,...)
{
  va_list ap;
  
  va_start(ap,fmt);
  error_doit(1,fmt,ap);
  va_end(ap);
  exit(1);
}

void error_quit(const char *fmt,...)
{
  va_list ap;
  
  va_start(ap,fmt);
  error_doit(0,fmt,ap);
  va_end(ap);
  exit(1);
}

void error_msg(const char *fmt,...)
{
  va_list ap;
  
  va_start(ap,fmt);
  error_doit(0,fmt,ap);
  va_end(ap);
  return;
}

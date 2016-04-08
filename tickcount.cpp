
#include <time.h>

unsigned getTickCount()
{
  unsigned val ;
  struct timespec ts ;
  static struct timespec starttime ;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  if (starttime.tv_sec == 0)
    starttime = ts ;
  ts.tv_sec -= starttime.tv_sec ;
  val = ts.tv_sec * 1000 ;
  val += ts.tv_nsec / 1000000 ;
  return val;
}

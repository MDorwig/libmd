
#ifdef __linux__
	#include <time.h>
#else
	#include <windows.h>
#endif

unsigned getTickCount()
{
  unsigned val ;
#ifdef __linux__
  struct timespec ts ;
  static struct timespec starttime ;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  if (starttime.tv_sec == 0)
    starttime = ts ;
  ts.tv_sec -= starttime.tv_sec ;
  val = ts.tv_sec * 1000 ;
  val += ts.tv_nsec / 1000000 ;
#else
  val = GetTickCount();
#endif
  return val;
}

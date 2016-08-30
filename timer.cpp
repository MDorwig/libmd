/*
 * timer.cpp
 *
 *  Created on: 26.08.2016
 *      Author: dorwig
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "timer.h"
#include "thread.h"

class TimerList : public LockedTypedItemList<Timer,offsetof(Timer,m_item)>
{
  typedef LockedTypedItemList<Timer,offsetof(Timer,m_item)> Base;
public:
            TimerList() { m_twait = 0; }
  void      Add(Timer *t);
  Timer *   Find(Timer * t);
  void      Remove(Timer * t, bool signal_change = true);
  int       Wait();
  void      Show(bool reverse);
  CEvent    m_listchanged;
  unsigned  m_twait;
};

static TimerList * timers;

Timer::Timer()
{
  m_interval = 0;
}

Timer::~Timer()
{

}

void Timer::Start(int interval)
{
  m_interval = interval;
  if (m_item.m_list != NULL)
    timers->Remove(this);
  timers->Add(this);
}

void Timer::Stop()
{
  timers->Remove(this);
}

void Timer::OnExpired()
{
}

void TimerList::Add(Timer *t)
{
  unsigned now = getTickCount();
  Lock();
  if (Count() != 0)
  {
    /* Anpassung : timeout verlängern
       Beispiel:
       wenn der letzte Wait() um 1000(m_twait) war, die waittime 500 ist und now ist 1200 und interval = 1000
       dann muss das interval um 1200(now) - 1000(m_twait) = 200 verlängert werden, damit der timer um 1200(now) + 1000(interval) = 2200 abläuft
    */
    t->m_interval += now - m_twait;
  }
  else
  {
    /*
     * m_twait auf now setzen, damit es gültig ist, wenn Add nochmal aufgerufen wird bevor der Timerthread wieder laufen konnte
     */
    m_twait = now;
  }
  Timer * i ;
  for (i = GetHead(); i != NULL ; i = GetNext(i))
  {
    if (t->m_interval < i->m_interval)
    {
      AddBefore(i,t);
      break;
    }
  }
  if (i == NULL)
    AddTail(t);
  m_listchanged.Set();
  Unlock();
}

Timer * TimerList::Find(Timer * t)
{
  Timer * i;
  Lock();
  for (i = GetHead() ; i != NULL ; i = GetNext(i))
  {
    if (i == t)
    {
      break ;
    }
  }
  Unlock();
  return i ;
}

void TimerList::Remove(Timer * t,bool signal_change)
{
  if (Find(t) != NULL)
  {
    Lock();
    Base::Remove(t);
    if (signal_change)
      m_listchanged.Set();
    Unlock();
  }
}

int TimerList::Wait()
{
  unsigned waittime;
  int      dt ;
  do
  {
    waittime = WAIT_INFINITE;
    Lock();
    Timer * t = GetHead();
    if (t != NULL)
      waittime = t->m_interval;
    Unlock();
    m_twait = getTickCount();
#ifdef LIBMD_TIMER_DEBUG
    printf("%10d wait %d\n",m_twait,waittime);
#endif
    m_listchanged.Wait(waittime); // kommt zurück, wenn waittime abgelaufen ist, oder die Liste veränder wurde
    dt = getTickCount() - m_twait ; // wie lange gewartet wurde
  } while (waittime == WAIT_INFINITE);
  return dt ;
}

void TimerList::Show(bool reverse)
{
  Timer * t;
  printf("Start { ");
  if (!reverse)
  {
    listforeach(t,*this)
    {
      printf("t=%4d ",t->m_interval);
    }
  }
  else
  {
    listforeachrev(t,*this)
    {
      printf("t=%4d ",t->m_interval);
    }
  }
  printf("} End\n");
}

class TimerService : public CThread
{
public:
  TimerService() : CThread("TimerService")
  {
  }

  void Main()
  {
    while(1)
    {
      Timer * t ;
      int dt = timers->Wait();
#ifdef LIBMD_TIMER_DEBUG
      printf("%10d {",getTickCount());
#endif
      for(t = timers->GetHead() ; t != NULL ; )
      {
        Timer * n = timers->GetNext(t);
        t->m_interval -= dt ;
#ifdef LIBMD_TIMER_DEBUG
        printf("rest %d ",t->m_interval);
#endif
        if (t->m_interval <= 0)
        {
          timers->Remove(t,false);
          t->OnExpired();
        }
        t = n;
      }
#ifdef LIBMD_TIMER_DEBUG
      printf("}\n");
#endif
    }
  }
} ;


static TimerService * timerService;

void StartTimerService()
{
  if (timers == NULL)
  {
    getTickCount();
    timers = new TimerList();
  }
  if (timerService == NULL)
  {
    timerService = new TimerService();
    timerService->Create();
  }
}

void ListTimer(bool rev)
{
  if (timers != NULL)
  {
    timers->Show(rev);
  }
}

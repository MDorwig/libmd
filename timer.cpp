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

class TimerList : protected CItemList
{
public:
            TimerList()
            {
              m_twait = 0;
            }

  void      Remove(CListItem * l);

  void      Add(Timer *t);
  Timer *   Find(Timer * t);
  void      Remove(Timer * t, bool signal_change = true);
  Timer *   First();
  int       Wait();
  void      Show(bool reverse);
  CMutex    m_lock;
  CEvent    m_listchanged;
  unsigned  m_twait;
};

static TimerList * timers;

Timer * Timer::Next()
{
  Timer * n = NULL;
  if (m_item.m_next != NULL)
    n = fromitem(m_item.m_next,Timer,m_item);
  return n;
}

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

void TimerList::Remove(CListItem * l)
{
  m_lock.Lock();
  CItemList::Remove(l);
  m_lock.Release();
}

void TimerList::Add(Timer *t)
{
  CListItem * item;
  unsigned now = getTickCount();
  m_lock.Lock();
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
  listforeach(item,*this)
  {
    Timer * i = fromitem(item,Timer,m_item);
    if (t->m_interval < i->m_interval)
    {
      AddBefore(&i->m_item,&t->m_item);
      break;
    }
  }
  if (item == NULL)
    AddTail(&t->m_item);
  m_listchanged.Set();
  m_lock.Release();
}

Timer * TimerList::Find(Timer * t)
{
  Timer * r = NULL;
  CListItem * item;
  m_lock.Lock();
  listforeach(item,*this)
  {
    if (item == &t->m_item)
    {
      r = t ;
      break ;
    }
  }
  m_lock.Release();
  return r ;
}

void TimerList::Remove(Timer * t,bool signal_change)
{
  if (Find(t) != NULL)
  {
    m_lock.Lock();
    Remove(&t->m_item);
    if (signal_change)
      m_listchanged.Set();
    m_lock.Release();
  }
}

Timer * TimerList::First()
{
  Timer * t = NULL;
  if (m_first != NULL)
    t = fromitem(m_first,Timer,m_item);
  return t ;
}

int TimerList::Wait()
{
  unsigned waittime;
  int      dt ;
  do
  {
    waittime = WAIT_INFINITE;
    m_lock.Lock();
    Timer * t = First();
    if (t != NULL)
      waittime = t->m_interval;
    m_lock.Release();
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
  CListItem * item ;
  printf("Start { ");
  if (!reverse)
  {
    listforeach(item,*this)
    {
      Timer * t = fromitem(item,Timer,m_item);
      printf("t=%4d ",t->m_interval);
    }
  }
  else
  {
    for (item = m_last ; item != NULL ; item = item->m_prev)
    {
      Timer * t = fromitem(item,Timer,m_item);
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
      for(t = timers->First() ; t != NULL ; )
      {
        Timer * n = t->Next();
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

void ListTimer()
{
  if (timers != NULL)
  {
    timers->Show(false);
  }
}

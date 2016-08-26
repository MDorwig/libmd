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

static class TimerList : protected CItemList
{
public:

  void    Remove(CListItem * l);

  void    Add(Timer *t);
  Timer * Find(Timer * t);
  void    Remove(Timer * t);
  Timer * First();
  int     Wait();
  void    Show(bool reverse);

  CMutex m_lock;
  CEvent m_no_empty;

} timers;

Timer * Timer::Next()
{
  Timer * n = NULL;
  if (m_item.m_next != NULL)
    n = fromitem(m_item.m_next,Timer,m_item);
  return n;
}

void Timer::Start(int interval)
{
  m_current = m_interval = interval;
  timers.Add(this);
}

bool Timer::Timeout(int dt)
{
  m_current -= dt;
  return m_current <= 0;
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
  m_lock.Lock();
  listforeach(item,*this)
  {
    Timer * i = fromitem(item,Timer,m_item);
    if (t->m_current < i->m_current)
    {
      AddBefore(&i->m_item,&t->m_item);
      break;
    }
  }
  if (item == NULL)
    AddTail(&t->m_item);
  m_no_empty.Set();
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

void TimerList::Remove(Timer * t)
{
  if (Find(t) != NULL)
  {
    m_lock.Lock();
    Remove(&t->m_item);
    if (Count() == 0)
      m_no_empty.Reset();
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
  if (Count() == 0)
    m_no_empty.Wait();
  Timer * t = First();
  int wt = t->m_current;
  usleep(wt*1000);
  return wt ;
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
      printf("t=%4d/%4d ",t->m_current,t->m_interval);
    }
  }
  else
  {
    for (item = m_last ; item != NULL ; item = item->m_prev)
    {
      Timer * t = fromitem(item,Timer,m_item);
      printf("t=%4d/%4d ",t->m_current,t->m_interval);
    }
  }
  printf("} End\n");
}

class TimerService : public CThread
{
public:
  TimerService() : CThread("TimerService")
  {
    Create();
  }

  void Main()
  {
    while(1)
    {
      Timer * t ;
      //unsigned start = getTickCount();
      int dt = timers.Wait();
      //unsigned now = getTickCount();
      //printf("dt %8d ",now-start);
      for(t = timers.First() ; t != NULL ; )
      {
        Timer * n = t->Next();
        if (t->Timeout(dt))
        {
          timers.Remove(t);
          t->OnExpired();
        }
        t = n;
      }
      //printf("\n");
    }
  }
} ;


static TimerService timerService;


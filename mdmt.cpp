/*
 * mdmt.cpp
 *
 *  Created on: 11.04.2013
 *      Author: dorwig
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include "msgqueue.h"
#include "thread.h"

CMutex::CMutex()
{
#if 1
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_lock,&attr);
#else
	m_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif
}

CMutex::~CMutex()
{
	pthread_mutex_destroy(&m_lock);
}

int CMutex::Lock()
{
  int res = pthread_mutex_lock(&m_lock);
  if (res != 0)
  {
    printf("mutex_lock returns %d - %s\n",res,strerror(res));
    fflush(stdout);
    assert(res == 0);
  }
	return res ;
 }

int CMutex::Release()
{
  int res = pthread_mutex_unlock(&m_lock);
  assert(res == 0);
  return res ;
}

CCondition::CCondition()
{
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	pthread_cond_init(&m_cond,&attr);
}

CCondition::~CCondition()
{
	pthread_cond_destroy(&m_cond);
}

int CCondition::Signal()
{
	return pthread_cond_signal(&m_cond) ;
}

int CCondition::Wait(CMutex & mtx,unsigned time)
{
  int res ;
  if (time != UINT32_MAX)
  {

    timespec ts ;

    clock_gettime(CLOCK_REALTIME,&ts);

    ts.tv_sec += time / 1000;
    ts.tv_nsec+= (time % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000)
    {
      ts.tv_sec += 1 ;
      ts.tv_nsec-= 1000000000;
    }

    res = pthread_cond_timedwait(&m_cond,&mtx.m_lock,&ts);
    assert (res == 0 || res == ETIMEDOUT);
  }
  else
    res = pthread_cond_wait(&m_cond,&mtx.m_lock);
  return res ;
}

CEvent::CEvent()
{
	m_state = 0 ;
}

CEvent::~CEvent()
{
}

int CEvent::Set()
{
	int res ;
	m_lock.Lock();
	m_state = 1 ;
	res = m_cond.Signal();
	m_lock.Release();
	return res ;
}

int CEvent::Reset()
{
	m_lock.Lock();
	m_state = 0 ;
	m_lock.Release();
	return 0;
}

int CEvent::Wait(unsigned timeout)
{
  int res = 0;
  m_lock.Lock();
  if (m_state == 0)
    res = m_cond.Wait(m_lock,timeout);
  m_state = 0;
  m_lock.Release();
  return res ;
}

CThread::CThread(const char * name)
{
  m_name = strdup(name);
}

void * CThread::Run(void * pthis)
{
  ((CThread*)pthis)->Main();
  return NULL;
}

void CThread::Create()
{
  pthread_create(&m_id,NULL,Run,this);
}

void CThread::Main()
{

}

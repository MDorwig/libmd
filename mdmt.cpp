/*
 * mdmt.cpp
 *
 *  Created on: 11.04.2013
 *      Author: dorwig
 */

#include <string.h>
#include <time.h>
#include "msgqueue.h"
#include "thread.h"

CMutex::CMutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m_lock,&attr);
}

CMutex::~CMutex()
{
	pthread_mutex_destroy(&m_lock);
}

int CMutex::Lock()
{
	return pthread_mutex_lock(&m_lock);
}

int CMutex::Release()
{
	return pthread_mutex_unlock(&m_lock);
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

int CCondition::Wait(CMutex & mtx)
{
	return pthread_cond_wait(&m_cond,&mtx.m_lock);
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

int CEvent::Wait()
{
	int res = 0;
	m_lock.Lock();
	if (m_state == 0)
	  res = m_cond.Wait(m_lock);
	Reset();
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

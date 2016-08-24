/*
 * mdmt.h
 *
 *  Created on: 11.04.2013
 *      Author: dorwig
 */

#ifndef MDMT_H_
#define MDMT_H_

#include <pthread.h>

#if defined __CYGWIN__
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

class CMutex
{
public:
			CMutex();
			~CMutex();
	int Lock();
	int	Release();
private:
	friend class CCondition;
	pthread_mutex_t m_lock ;
};


class CCondition
{
public:
	CCondition();
	~CCondition();
	int Signal();
	int Wait(CMutex & mtx);
private:
	pthread_cond_t m_cond ;
};

class CEvent
{
public:
	CEvent();
	~CEvent();
	int Set();
	int Reset();
	int	IsSet();
	int	Wait();
private:
	CMutex 			m_lock ;
	CCondition 	m_cond ;
	int					m_state;
};

class CTimer
{

};

#include "msgqueue.h"

/*
class CThread
{
public:
  CThread(const char * name);
  void       Create();
  virtual void Main();
private:
  static void * Run(void * arg);
  char *    m_name;
  pthread_t m_id ;
  CMsgQueue m_msgqueue;
};
*/
#endif /* MDMT_H_ */

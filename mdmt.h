/*
 * mdmt.h
 *
 *  Created on: 11.04.2013
 *      Author: dorwig
 */

#ifndef MDMT_H_
#define MDMT_H_

#include <pthread.h>
#include <stdint.h>

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

#define WAIT_INFINITE UINT32_MAX

class CCondition
{
public:
	CCondition();
	~CCondition();
	int Signal();
	int Wait(CMutex & mtx,unsigned time = WAIT_INFINITE);

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
	int	Wait(unsigned time = WAIT_INFINITE);
private:
	CMutex 			m_lock ;
	CCondition 	m_cond ;
	int					m_state;
};

#endif /* MDMT_H_ */

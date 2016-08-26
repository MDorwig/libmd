/*
 * thread.h
 *
 *  Created on: 21.02.2015
 *      Author: dorwig
 */

#ifndef THREAD_H_
#define THREAD_H_


#include "msgqueue.h"

class CThread
{
public:
  CThread(const char * name);
  virtual ~CThread() {}
  void       Create();
  virtual void Main() = 0;
protected:
  static void * Run(void * arg);
  char *    m_name;
  pthread_t m_id ;
  CMsgQueue m_msgqueue;
};

unsigned getTickCount();



#endif /* THREAD_H_ */

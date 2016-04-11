/*
 * msgqueue.h
 *
 *  Created on: 27.10.2013
 *      Author: dorwig
 */

#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include "itemlist.h"
#include "mdmt.h"

typedef void (*MsgQueueDelegate)(unsigned long p1,unsigned p2,unsigned p3);

class CMsg : public CListItem
{
public:
  CMsg();
  CMsg(unsigned id, long unsigned p1 = 0, long unsigned p2 =0, long unsigned p3 = 0);
  CMsg(MsgQueueDelegate d, long unsigned p1 = 0, long unsigned p2 =0, long unsigned p3 = 0);
  CMsg(MsgQueueDelegate d,void * pthis, long unsigned p2 =0, long unsigned p3 = 0);
  unsigned      Id() const { return m_id;}
  long unsigned P1() const { return m_p1;}
  long unsigned P2() const { return m_p2;}
  long unsigned P3() const { return m_p3;}
  bool isDelegate()  const { return m_d != NULL;}
  void Invoke() const { m_d(m_p1,m_p2,m_p3);}
private:
  unsigned         m_id ;
  long unsigned    m_p1 ;
  long unsigned    m_p2 ;
  long unsigned    m_p3 ;
  MsgQueueDelegate m_d;
};

class CMsgQueue
{
public:
	CMsgQueue();
	~CMsgQueue();
  CItemList m_list ;
  CMutex  m_lock;
  CEvent  m_notempty;
  void PostMessage(const CMsg & item);
  void PostMessage(unsigned id, long unsigned p1 = 0, long unsigned p2 = 0, long unsigned p3 = 0);
  void PostMessage(MsgQueueDelegate d,void * pthis, long unsigned p2 = 0, long unsigned p3 = 0);
  void GetMessage(CMsg & item);
  static unsigned CreateMessageId();
private:
  void PostMessage(CMsg * item);
};


#endif /* MSGQUEUE_H_ */

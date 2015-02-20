/*
 * msgqueue.h
 *
 *  Created on: 27.10.2013
 *      Author: dorwig
 */

#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include "list.h"

enum MsgId
{
  MSG_INVOKE = 1,
  MSG_TIMER  = 2,
	MSG_SCEMSG = 3,
};

typedef void (*delegate)(unsigned long p1,unsigned p2,unsigned p3);

class CMsg : public CListItem
{
public:
  CMsg();
  CMsg(MsgId   id, long unsigned p1 = 0, long unsigned p2 =0, long unsigned p3 = 0);
  CMsg(delegate d, long unsigned p1 = 0, long unsigned p2 =0, long unsigned p3 = 0);
  CMsg(delegate d,void * pthis, long unsigned p2 =0, long unsigned p3 = 0);
  MsgId Id()    const { return m_id;}
  long unsigned P1() const { return m_p1;}
  long unsigned P2() const { return m_p2;}
  long unsigned P3() const { return m_p3;}
  void Invoke() const { m_d(m_p1,m_p2,m_p3);}
private:
  MsgId    m_id ;
  long unsigned m_p1 ;
  long unsigned m_p2 ;
  long unsigned m_p3 ;
  delegate m_d;
};

class CMutex;
class CEvent;

class CMsgQueue
{
public:
	CMsgQueue();
	~CMsgQueue();
  CItemList m_list ;
  CMutex  * m_lock;
  CEvent  * m_notempty;
  void PostMessage(const CMsg & item);
  void PostMessage(MsgId id, long unsigned p1 = 0, long unsigned p2 = 0, long unsigned p3 = 0);
  void PostMessage(delegate d,void * pthis, long unsigned p2 = 0, long unsigned p3 = 0);
  void GetMessage(CMsg & item);
private:
  void PostMessage(CMsg * item);
};

#endif /* MSGQUEUE_H_ */

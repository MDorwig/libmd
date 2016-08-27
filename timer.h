/*
 * timer.h
 *
 *  Created on: 26.08.2016
 *      Author: dorwig
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "itemlist.h"
#include "mdmt.h"

class Timer
{
public:
              Timer();
  virtual    ~Timer();
  Timer *      Next();
  virtual void Start(int interval);
  void         Stop();
  virtual void OnExpired();

  int  m_interval;
  CListItem m_item;
};

void StartTimerService();
void ListTimer();

#endif /* TIMER_H_ */

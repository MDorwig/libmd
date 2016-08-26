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
  Timer(int interval = 0)
  {
    m_interval = m_current = interval;
  }

  Timer * Next();

  virtual void Start(int interval);

  bool Timeout(int dt);

  virtual void OnExpired();

  int m_interval;
  int m_current ;
  CListItem m_item;
};

void StartTimerService();

#endif /* TIMER_H_ */

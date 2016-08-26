/*
 * itemlist.cpp
 *
 *  Created on: 26.08.2016
 *      Author: dorwig
 */
#include "itemlist.h"

CListItem::~CListItem()
{
  if (m_list != NULL)
  {
    m_list->Remove(this);
  }
}

void CItemList::AddTail(CListItem * t)
{
  assert(t->m_list == NULL);
  if (m_first == NULL)
    m_first = t ;
  else
  {
    m_last->m_next = t ;
    t->m_prev = m_last;
  }
  t->m_list = this;
  m_last = t ;
  m_count++;
}

/*
 * Add item b before item a
 */
void CItemList::AddBefore(CListItem * a,CListItem * b)
{
  assert(a->m_list != NULL);
  assert(b->m_list == NULL);
  if (a->m_prev != NULL)
  {
    a->m_prev->m_next = b ;
    b->m_prev = a->m_prev;
  }
  else
  {
    m_first = b ;
  }
  a->m_prev = b ;
  b->m_next = a;
  b->m_list = this;
  m_count++;
}

void CItemList::AddHead(CListItem * t)
{
  if (m_last == NULL)
    m_last = t ;
  else
  {
    m_first->m_prev = t ;
    t->m_next = m_first;
  }
  m_first = t ;
}

void CItemList::Remove(CListItem * t)
{
  assert(t->m_list == this);
  if (t == m_first)
     m_first = t->m_next;
   if (t == m_last)
     m_last = t->m_prev;
   if (t->m_next != NULL)
     t->m_next->m_prev = t->m_prev;
   if (t->m_prev != NULL)
     t->m_prev->m_next = t->m_next;
   t->Init();
   m_count--;
}




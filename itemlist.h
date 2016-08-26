/*
 * list.h
 *
 *  Created on: 02.11.2013
 *      Author: dorwig
 */

#ifndef ITEMLIST_H_
#define ITEMLIST_H_

#include <stddef.h>

class CListItem
{
public:
  CListItem * m_next ;
  CListItem * m_prev ;

  CListItem()
  {
    m_next = m_prev = NULL;
  }
};

class CItemList
{
public:

  CListItem * m_first;
  CListItem * m_last;
  int m_count;
  CItemList()
  {
    m_first = m_last = NULL;
    m_count = 0;
  }

  void AddTail(CListItem * t)
  {
    if (m_first == NULL)
      m_first = t ;
    else
    {
      m_last->m_next = t ;
      t->m_prev = m_last;
    }
    m_last = t ;
    m_count++;
  }

  /*
   * Add item b before item a
   */
  void AddBefore(CListItem * a,CListItem * b)
  {
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
  }

  void Remove(CListItem * t)
  {
    if (t == m_first)
       m_first = t->m_next;
     if (t == m_last)
       m_last = t->m_prev;
     if (t->m_next != NULL)
       t->m_next->m_prev = t->m_prev;
     if (t->m_prev != NULL)
       t->m_prev->m_next = t->m_next;
     m_count--;
  }

  CListItem * GetHead()
  {
    return m_first;
  }

  bool isEmpty()
  {
    return m_count == 0 ;
  }

  CListItem * GetTail()
  {
    return m_last;
  }

  unsigned Count() { return m_count;}
};

#define mbroffset(typ,mbr) (((long unsigned)(&((typ*)4)->mbr))-4)
#define fromitem(item,typ,list) (typ*)((char*)item-mbroffset(typ,list))

#define listforeach(item,list) for( item = (list).m_first ; item != NULL ; item = item->m_next)



#endif /* ITEMLIST_H_ */

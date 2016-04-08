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
  CListItem()
  {
    m_next = m_prev = this;
  }
  CListItem * m_next ;
  CListItem * m_prev ;

  static void AddTail(CListItem * list,CListItem * item)
  {
    item->m_next = list;
    item->m_prev = list->m_prev;
    list->m_prev->m_next = item;
    list->m_prev = item;
  }

  static void AddHead(CListItem * list,CListItem * item)
  {
    item->m_prev = list ;
    item->m_next = list->m_next;
    list->m_next->m_prev = item;
    list->m_next = item ;
  }

  static void Remove(CListItem * item)
  {
    item->m_prev->m_next = item->m_next;
    item->m_next->m_prev = item->m_prev;
    item->m_prev = item->m_next = item ;
  }
};

class CItemList
{
public:

  CItemList() : m_list()
  {

  }
  void AddTail(CListItem & t)
  {
    m_list.AddTail(&m_list,&t);
  }

  void AddHead(CListItem & t)
  {
    m_list.AddHead(&m_list,&t);
  }

  void Remove(CListItem & t)
  {
    m_list.Remove(&t);
  }

  CListItem * GetHead()
  {
    CListItem * item = m_list.m_next;
    if (item == &m_list)
      item = NULL;
    return item ;
  }

  bool isEmpty()
  {
    return m_list.m_next != &m_list ;
  }

  CListItem * GetNext(CListItem * item)
  {
    item = item->m_next;
    if (item == &m_list)
      item = NULL;
    return item;
  }

  CListItem * GetTail()
  {
    return m_list.m_prev;
  }

  CListItem m_list;

};

#define mbroffset(typ,mbr) (((long unsigned)(&((typ*)4)->mbr))-4)
#define fromitem(item,typ,list) (typ*)((char*)item-mbroffset(typ,list))

#define listforeach(item,list) for( item = list->m_next ; item != list ; item = item->m_next)



#endif /* ITEMLIST_H_ */

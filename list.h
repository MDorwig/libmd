/*
 * list.h
 *
 *  Created on: 02.11.2013
 *      Author: dorwig
 */

#ifndef LIST_H_
#define LIST_H_

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

  static void Remove(CListItem * list,CListItem * item)
  {
    item->m_prev->m_next = item->m_next;
    item->m_next->m_prev = item->m_prev;
    item->m_prev = item->m_next = item ;
  }
};

class CItemList
{
public:
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
    m_list.Remove(&m_list,&t);
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

#define fromitem(item,typ,list) (typ*)((char*)item-offsetof(typ,list))

#define listforeach(item,list) for( item = list->m_next ; item != list ; item = item->m_next)



#endif /* LIST_H_ */
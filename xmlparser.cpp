/*
 * xmlparser.cpp
 *
 *  Created on: 20.08.2013
 *      Author: dorwig
 */

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "xmlparser.h"

xmlscanner::xmlscanner()
{
  m_state = SC_ASCII;
  m_ch = -1;
  m_utf8cnt = 0 ;
}

int xmlscanner::nextchar(const char ** input)
{
  int ch = **input;
  int mask = 0xff ;
  if (m_state == SC_ASCII)
  {
    switch(ch & 0xf8)
    {
      case 0x00:
      case 0x08:
      case 0x10:
      case 0x18:
      case 0x20:
      case 0x28:
      case 0x30:
      case 0x38:
      case 0x40:
      case 0x48:
      case 0x50:
      case 0x58:
      case 0x60:
      case 0x68:
      case 0x70:
      case 0x78:
        if (ch != 0)
          (*input)++;
      return ch ;

      case 0x80:
      case 0x88:
      case 0x90:
      case 0x98:
      case 0xA0:
      case 0xA8:
      case 0xB0:
      case 0xB8:
       /*
        * illegal for utf8
        */
      return -1 ;

      case 0xc0:
      case 0xc8:
      case 0xd0:
      case 0xd8:
        mask = 0x1f ;
        m_utf8cnt = 1 ;
      break ;

      case 0xe0:
      case 0xe8:
        mask = 0x0f ;
        m_utf8cnt = 2 ;
      break ;

      case 0xf0:
        mask = 0x07 ;
        m_utf8cnt = 3 ;
      break ;
    }
    m_state = SC_UTF8;
    m_ch = ch & mask ;
    (*input)++;
    ch = **input;
  }
  if (m_state == SC_UTF8)
  {
    while (m_utf8cnt > 0 && (ch & 0xc0) == 0x80)
    {
      (*input)++;
      m_ch <<= 6 ;
      m_ch |= ch & 0x3f ;
      m_utf8cnt--;
      ch = **input;
    }
    if (m_utf8cnt == 0)
    {
      m_state = SC_ASCII;
      return m_ch ;
    }
  }
  return -1 ;
}

xmlparserException::xmlparserException(const char * fmt,...)
{
  va_list lst ;
  va_start(lst,fmt);
  m_text = NULL;
  vasprintf(&m_text,fmt,lst);
  va_end(lst);
}

xmlparserException::~xmlparserException() throw()
{
  delete m_text ;
}

const char * xmlparserException::what() const throw()
{
  return m_text ;
}

xmlAttr::xmlAttr(const char * aname,const char * aval)
{
  name = (xmlChar*)strdup(aname);
  children = new xmlNode(XML_TEXT_NODE,"");
  children->content = (xmlChar*)strdup(aval);
  children->parent = parent;
  children->doc = doc;
}

xmlAttr::~xmlAttr()
{
  delete children;
  delete name ;
}

xmlNode::xmlNode(xmlElementType et,const char * en)
{
  type = et ;
  name = (xmlChar*)strdup(en);
  _private = NULL;
  children = NULL;
  last = NULL;
  parent = NULL;
  next = NULL;
  prev = NULL;
  doc = NULL;
  content = NULL;
  properties = NULL;
}

xmlNode::~xmlNode()
{
  for (xmlNodePtr c = children ; c != NULL ;)
  {
    xmlNodePtr n = c ;
    c = c->next ;
    delete n ;
  }
  for (xmlAttrPtr a = properties ; a != NULL ;)
  {
    xmlAttrPtr p = a ;
    a = a->next ;
    delete p ;
  }
  if (name != NULL)
    delete name ;
  if (content != NULL)
    delete content;
}

void xmlNode::AddAttr(xmlAttrPtr a)
{
  a->next = properties;
  a->parent = this;
  a->doc  = doc;
  properties = a ;
}

void xmlNode::AddChild(xmlNodePtr c)
{
  if (children == NULL)
    children = c ;
  else
  {
    last->next = c ;
    c->prev = last ;
  }
  last = c ;
  c->parent = this;
}

xmlDoc::xmlDoc(const char * docname)
{
  _private = NULL;
   type = XML_DOCUMENT_NODE;
  name = strdup(docname);
  children = NULL;
  last = NULL;
  parent = NULL;
  next = NULL;
  prev = NULL;
  version = NULL;
  encoding = NULL;
}

void xmlDoc::AddChild(xmlNode * n)
{
  if (children == NULL)
    children = n ;
  else
  {
    last->next = n ;
    n->prev = last ;
  }
  last = n;
}

xmlDoc::~xmlDoc()
{
  xmlNode * c = children ;
  while(c != NULL)
  {
    xmlNode * n = c ;
    c = c->next;
    delete n ;
  }

}

xmlparser::xmlparser(bool trace)
{
  m_trace   = trace;
  m_charval = 0 ;
  m_strdelim = 0 ;
  m_skipws = 0 ;
  for (int i = 0 ; i < (int)(sizeof m_state/sizeof m_state[0]) ; i++)
  {
    m_state[i] = PS_IDLE;
  }
  m_statesp = 0 ;
  m_doc = NULL;
  m_curnode = NULL;
}

xmlparser::~xmlparser()
{

}

const char * xmlparser::StateName(parserstate st)
{
  switch(st)
  {
    case  PS_IDLE:        return "PS_IDLE";
    case  PS_OPENTAG:     return "PS_OPENTAG";
    case  PS_CLOSETAG:    return "PS_CLOSETAG";
    case  PS_IDENT_START: return "PS_IDENT_START";
    case  PS_IDENT:       return "PS_IDENT";
    case  PS_ESCAPE1:     return "PS_ESCAPE1";
    case  PS_ESCAPE2:     return "PS_ESCAPE2";
    case  PS_ESCAPE3:     return "PS_ESCAPE3";
    case  PS_ESCAPE4:     return "PS_ESCAPE4";
    case  PS_QUOT1:				return "PS_QUOT1";
    case  PS_QUOT2:				return "PS_QUOT2";
    case  PS_QUOT3:				return "PS_QUOT3";
    case  PS_QUOT4:				return "PS_QUOT4";
    case  PS_AMPAPOS:			return "PS_AMPAPOS";
    case  PS_AMP1:				return "PS_AMP1";
    case  PS_AMP2:				return "PS_AMP2";
    case  PS_APOS1:				return "PS_APOS1";
    case  PS_APOS2:				return "PS_APOS2";
    case  PS_APOS3:				return "PS_APOS3";
    case  PS_LT:					return "PS_LT";
    case  PS_GT:					return "PS_GT";
    case  PS_STRING:      return "PS_STRING";
    case  PS_XMLDECL:     return "PS_XMLDECL";
    case  PS_CHARDATA:    return "PS_CHARDATA";
    case  PS_ATTR_LIST:   return "PS_ATTR_LIST";
    case  PS_ATTR_NAME:   return "PS_ATTR_NAME";
    case  PS_ATTR_EQ:     return "PS_ATTR_EQ";
    case  PS_ATTR_VALUE:  return "PS_ATTR_VALUE";
    case  PS_START_TAG:   return "PS_START_TAG";
    case  PS_START_TAG1:  return "PS_START_TAG1";
    case  PS_END_TAG:     return "PS_END_TAG";
    case  PS_COMPLETE:    return "PS_COMPLETE";
    case  PS_COMMENT:     return "PS_COMMENT";
    case  PS_COMMENT1:     return "PS_COMMENT1";
    case  PS_COMMENT2:     return "PS_COMMENT2";
    case  PS_COMMENT3:     return "PS_COMMENT3";
    case  PS_COMMENT4:     return "PS_COMMENT4";
  }
  return "PS_UNKNOWN";
}

void xmlparser::onContent(xmlNode * node,const std::string & content)
{
  node->content = (xmlChar*)strdup(content.c_str()) ;
  if (m_trace)
    printf("element %s = %s\n",node->name,node->content);
}

void xmlparser::onElementComplete(xmlNode * node)
{

}

void xmlparser::onDocumentComplete(xmlDocPtr doc)
{
}

void xmlparser::setState(parserstate st)
{
  m_state[m_statesp] = st ;
  if (m_trace)
      printf("Enterstate (%d) %s\n",m_statesp,StateName(st));
}

xmlparser::parserstate xmlparser::getState()
{
  return m_state[m_statesp];
}

void xmlparser::pushState(parserstate st)
{
  if (m_statesp < (int)(sizeof m_state/sizeof m_state[0]))
  {
    m_statesp++;
    setState(st);
  }
  else
  {
    throw xmlparserException("push to stack overflow");
  }
}

xmlparser::parserstate xmlparser::popState()
{
  parserstate st ;
  if (m_statesp > 0)
  {
    m_statesp--;
  }
  else
  {
    throw xmlparserException("FSM: pop from empty stack");
  }
  st = getState();
  if (m_trace)
    printf("Resume (%d) in %s\n",m_statesp,StateName(st));
  return st ;
}

bool xmlparser::isNameStartChar(int ch)
{
  if (ch == ':' || ch == '_') return true ;
  if (ch >= 'A' && ch <= 'Z') return true;
  if (ch >= 'a' && ch <= 'z') return true;
  if (ch >= 0xc0 && ch <= 0xd6) return true ;
  if (ch >= 0xd8 && ch <= 0xf6) return true ;
  if (ch >= 0xF8 && ch <= 0x2FF) return true;
  if (ch >= 0x370 && ch <= 0x37D) return  true ;
  if (ch >= 0x37F && ch <= 0x1FFF) return true ;
  if (ch >= 0x200C && ch <= 0x200D) return true;
  if (ch >= 0x2070 && ch <= 0x218F) return true;
  if (ch >= 0x2C00 && ch <= 0x2FEF) return true;
  if (ch >= 0x3001 && ch <= 0xD7FF) return true;
  if (ch >= 0xF900 && ch <= 0xFDCF) return true;
  if (ch >= 0xFDF0 && ch <= 0xFFFD) return true;
  if (ch >= 0x10000 && ch <= 0xEFFFF) return true ;
  return false ;
}

bool xmlparser::isNameChar(int ch)
{
  if (ch == '-' || ch == '.' || (ch >= '0' && ch <= '9') || ch == 0xB7 || isNameStartChar(ch))
    return true ;
  if (ch >= 0x0300 && ch <= 0x036F)
    return true ;
  return (ch == 0x203F || ch == 0x2040) ;
}

bool xmlparser::isWhiteSpace(int ch)
{
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

void xmlparser::completeElement(bool checkname,const char * input)
{
  xmlNodePtr n = m_curnode;
  if (n->type == XML_PI_NODE)
  {
    for (xmlAttrPtr a = n->properties ; a != NULL ; a = a->next)
    {
      if (strcmp((char*)a->name,"version") == 0)
        m_doc->version = (xmlChar*)strdup((char*)a->children->content);
      else if (strcmp((char*)a->name,"encoding") == 0)
        m_doc->encoding = (xmlChar*)strdup((char*)a->children->content);
    }
    m_curnode = n->parent;
    delete n;
    setState(PS_IDLE);
  }
  else
  {
    if (!checkname || strcmp((char*)n->name,m_ident.c_str()) == 0)
    {
      onElementComplete(n);
      m_curnode = n->parent;
      if (m_statesp > 0)
      {
        popState();
        if (getState() != PS_OPENTAG)
          throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),input);
      }
      if (m_curnode == NULL && n->type != XML_PI_NODE)
      {
        onDocumentComplete(m_doc);
        delete m_doc ;
        m_doc = NULL;
      }
      setState(PS_IDLE);
    }
    else
    {
      throw xmlparserException("%s does not match %s\n",m_ident.c_str(),m_curnode->name);
    }
  }
}

void xmlparser::AddNode(xmlNode * n)
{
  if (m_doc == NULL)
    m_doc = new xmlDoc("doc");
  n->doc = m_doc;
  if (n->type != XML_PI_NODE)
  {
    if (m_curnode == NULL)
      m_doc->AddChild(n);
    else
      m_curnode->AddChild(n);
  }
  else
  {
    n->parent = m_curnode ;
  }
  m_curnode = n ;
}

int xmlparser::parse(const char ** input)
{
  while(1)
  {
    int ch = m_scanner.nextchar(input);
    if (ch <= 0)
      return -1 ;
restart:
    switch(getState())
    {
      case PS_IDLE:
        if (ch == '<')
          setState(PS_OPENTAG);
        else if (!isWhiteSpace(ch))
          return -1 ;
      break ;

      case PS_OPENTAG:
        if (ch == '?')
        {
          /*
           * "<?"
           */
          setState(PS_XMLDECL);
          pushState(PS_IDENT_START);
        }
        else if (ch == '!')
        {
          /*
           * "<!"
           */
          pushState(PS_COMMENT);
        }
        else if (isNameStartChar(ch))
        {
          /*
           * "<"NameStartChar
           */
          m_ident = ch ;
          pushState(PS_START_TAG);
          pushState(PS_IDENT);
        }
        else if (ch == '/')
        {
          /*
           * "</"
           */
          pushState(PS_END_TAG);
          pushState(PS_IDENT_START);
        }
        else
        {
          throw xmlparserException("'?','/' or identifier expected near %s",*input);
        }
      break ;

      case  PS_COMMENT:
        if (ch != '-')
        {
          throw xmlparserException("'-' expected near %s",*input);
        }
        setState(PS_COMMENT1);
      break ;

      case  PS_COMMENT1:
        if (ch != '-')
          throw xmlparserException("'-' expected near %s",*input);
        setState(PS_COMMENT2);
      break ;

      case  PS_COMMENT2:
        if (ch == '-')
          setState(PS_COMMENT3);
      break ;

      case  PS_COMMENT3:
        if (ch == '-')
          setState(PS_COMMENT4);
        else
          setState(PS_COMMENT2);
      break ;

      case  PS_COMMENT4:
        if (ch != '>')
          throw xmlparserException("'> expected near %s",*input);
        popState();
        if (getState() == PS_OPENTAG)
          setState(PS_IDLE);
      break ;

      case  PS_CLOSETAG:
        if (ch == '>')
        {
          switch(popState())
          {
            case PS_XMLDECL:
            {
              completeElement(false,*input);
            }
            break ;
            default:
            break ;
          }
        }
        else
          throw xmlparserException("'>' expected near %s",*input);
      break ;

      case  PS_XMLDECL:
        if (!isWhiteSpace(ch))
        {
          if (ch == '?')
          {
            pushState(PS_CLOSETAG);
          }
          else if (isNameStartChar(ch))
          {
            m_ident = ch ;
            pushState(PS_IDENT);
          }
        }
      break ;

      case  PS_IDENT_START:
        if (!isWhiteSpace(ch))
        {
          if (isNameStartChar(ch))
          {
            m_ident = ch ;
            setState(PS_IDENT);
          }
          else
          {
            throw xmlparserException("identifier expected near %s",*input);
          }
        }
      break ;

      case  PS_IDENT:
        if (isNameChar(ch))
          m_ident += ch ;
        else
        {
           switch(popState())
           {
             case PS_XMLDECL:
              if (m_ident == "xml")
              {
                xmlNodePtr n = new xmlNode(XML_PI_NODE,m_ident.c_str());
                AddNode(n);
                pushState(PS_ATTR_LIST);
                goto restart;
              }
            break ;

            case  PS_ATTR_NAME:
              if (ch == '=')
                setState(PS_ATTR_VALUE);
              else
                setState(PS_ATTR_EQ);
            break ;

            case  PS_START_TAG:
            {
              xmlNode * n = new xmlNode(XML_ELEMENT_NODE,m_ident.c_str());
              AddNode(n);
              pushState(PS_ATTR_LIST);
            }
            goto restart ;

            case PS_END_TAG:
              if (ch == '>')
              {
                completeElement(true,*input);
              }
              else if (!isWhiteSpace(ch))
              {
                throw xmlparserException("'>' expected in after \"%s\"\n",m_curnode->name);
              }
            break ;

            default:
              throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
            break ;
          }
        }
      break ;

      case  PS_END_TAG:
        if (ch == '>')
        {
          completeElement(true,*input);
        }
        else
        {
          throw xmlparserException("'>' expected in after \"%s\"\n",m_curnode->name);
        }
      break ;


      case  PS_START_TAG:
        if (ch == '/')
        {
          setState(PS_START_TAG1);
        }
        else if (ch == '>')
        {
          m_lexval = "" ;
          m_skipws = 1;
          pushState(PS_CHARDATA);
        }
      break ;

      case  PS_START_TAG1:
        if (ch == '>')
        {
        	if (m_statesp > 0)
        	{
        		if (popState() == PS_OPENTAG)
        		{
        			m_ident = (char*)m_curnode->name;
        		}
        		pushState(getState());
        	}
          completeElement(false,*input);
          setState(PS_IDLE);
        }
        else
          throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case  PS_ATTR_LIST:
        if (isNameStartChar(ch))
        {
          m_ident = ch ;
          pushState(PS_ATTR_NAME);
          pushState(PS_IDENT);
        }
        else if (!isWhiteSpace(ch))
        {
          popState();
          goto restart;
        }
      break ;

      case  PS_ATTR_NAME:
        if (isNameStartChar(ch))
        {
          m_ident = ch ;
          pushState(PS_IDENT);
        }
        else if (isWhiteSpace(ch))
        {
          pushState(PS_IDENT_START);
        }
        else if (ch == '=')
        {
          setState(PS_ATTR_EQ);
        }
        else
          throw xmlparserException("attributename expected in state %s near %s\n",StateName(getState()),*input);
      break ;

      case  PS_ATTR_EQ:
        if (!isWhiteSpace(ch))
        {
          if (ch == '=')
          {
            setState(PS_ATTR_VALUE);
          }
          else
            throw xmlparserException("'=' expected in state %s near %s\n",StateName(getState()),*input);
        }
      break ;

      case  PS_ATTR_VALUE:
        if (!isWhiteSpace(ch))
        {
          if (ch == '\'' || ch == '"')
          {
            m_strdelim = ch ;
            m_lexval = "";
            pushState(PS_STRING);
          }
        }
      break ;

      case PS_STRING:
        if (ch == m_strdelim)
        {
          if (popState() == PS_ATTR_VALUE)
          {
            xmlAttrPtr a = new xmlAttr(m_ident.c_str(),m_lexval.c_str());
            m_curnode->AddAttr(a);
            popState();
          }
        }
        else if (ch == '&')
        {
          m_charval = 0 ;
          pushState(PS_ESCAPE1);
        }
        else
        {
          m_lexval += ch;
        }
      break ;

      case  PS_ESCAPE1:
        if (ch == '#')
          setState(PS_ESCAPE2); // "&#"
        switch (ch)
        {
        	case	'q':
        		setState(PS_QUOT1);
        	break ;

        	case  'a':
        		setState(PS_AMPAPOS);	// &a
        	break ;

        	case 	'l':
        		setState(PS_LT); // &l
        	break ;

        	case 	'g':
        		setState(PS_GT);	// &g
        	break ;

        	default:
        		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
        	break ;

        }
      break ;

      case  PS_ESCAPE2:
        if (ch == 'x')
          setState(PS_ESCAPE4);
       else if (ch >= '0' && ch <= '9')
         setState(PS_ESCAPE3); // "&#x"
       else
         return 0 ;
       break ;

      case  PS_ESCAPE3:
        if (isdigit(ch))
        {
          m_charval *= 10 ; // "&#xx"
          m_charval += ch - '0';
        }
        else if (ch == ';')
        {
          m_lexval += (char)m_charval;
          popState();
          switch(getState())
          {
            case PS_STRING:
            case PS_CHARDATA:
              m_lexval += (char)m_charval;
            break;

            default:
              throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
            break ;
          }
        }
        else
        {
          return 0 ;
        }
      break ;

      case  PS_ESCAPE4:
        if (isxdigit(ch))
        {
          m_charval *= 16 ;
          if (ch >= 'a')
            ch -= 'a' - 10;
          else if (ch >= 'A')
            ch -= 'A' - 10;
          else
            ch -= '0';
          m_charval += ch ;
        }
        else if (ch == ';')
        {
          m_lexval += (char)m_charval;
          popState();
          switch(getState())
          {
            case PS_STRING:
            case PS_CHARDATA:
            break;

            default:
              throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
            break ;
          }
        }
      break ;

      case  PS_QUOT1:
      	if (ch == 'u')
      		setState(PS_QUOT2);	// &qu
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case	PS_QUOT2:
      	if (ch == 'o')
      		setState(PS_QUOT3);	// &quo
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case PS_QUOT3:
      	if (ch == 't')
      	{
      		m_charval = '"';
      		setState(PS_ESCAPE4); // &quot
      	}
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case  PS_AMPAPOS:
      	if (ch == 'm')
      		setState(PS_AMP1);		// &am
      	else if (ch == 'p')
      		setState(PS_APOS1);		// &ap
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case PS_AMP1:
      	if (ch == 'p')
      	{
      		m_charval = '&';	// &amp
      		setState(PS_ESCAPE4);
      	}
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case PS_APOS1:
      	if (ch == 'o')
      		setState(PS_APOS2);
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case PS_APOS2:
      	if (ch == 's')
      	{
      		m_charval = '\'';
      		setState(PS_ESCAPE4);
      	}
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case  PS_LT:
      	if (ch == 't')
      	{
      		m_charval = '<';
      		setState(PS_ESCAPE4);
      	}
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case  PS_GT:
      	if (ch == 't')
      	{
      		m_charval = '>';
      		setState(PS_ESCAPE4);
      	}
      	else
      		throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;

      case  PS_CHARDATA:
        if (ch == '<')
        {
          switch(popState())
          {
            case  PS_START_TAG:
            {
              onContent(m_curnode,m_lexval);
              popState();
            }
            break ;
            default:
              throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
            break ;
          }
        }
        else if (ch == '&')
        {
          pushState(PS_ESCAPE1);
        }
        else
        {
        	if (!isWhiteSpace(ch) || !m_skipws)
        	{
        		m_skipws=0;
        		if (ch == '\n')
        			m_skipws = 1 ;
        		m_lexval += ch ;
        	}
        }
      break ;

      case  PS_COMPLETE:
        return 1 ;
      break ;

      default:
        throw xmlparserException("parse error in state %s near %s\n",StateName(getState()),*input);
      break ;
    }
  }
  return -1 ;
}

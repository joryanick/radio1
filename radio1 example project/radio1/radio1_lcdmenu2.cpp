// radio1_lcdmenu2.cpp
//
// GoodPrototyping radio1 rev.C5
// This file is part of a modified menu system released into public domain

//- Autor:   Jomelo
//- ICQ:     78048329
//- License: all Free
//- Edit:    2009.09.11

#include "radio1_lcdmenu2.h"
#include "radio1.h"

  lcdmenu2::lcdmenu2(menu &r, uint8_t ro, uint8_t co)
  {
      rootMenu        = &r;
      curMenu         = rootMenu;
      back            = 0;
      cols            = co;
      rows            = ro;
      cursor_pos      = 0;
      layer           = 0;
      layer_save[0]   = 0;      
  }
  
  void lcdmenu2::setCursor()
  {
  char space[3];

    space[0]=32;
    space[1]=32;
    space[2]=0;  
    
    if(cursor_pos > curloc-scroll)
    {
        TV.print(10, CONST_MENU_INDENT_WIDTH+(cursor_pos*8), space);
    } else
       if(cursor_pos < curloc-scroll)
       {
         TV.print(10, CONST_MENU_INDENT_WIDTH+(curloc-scroll-1)*8, space);
       }
      
    cursor_pos = curloc-scroll;

    TV.printPGM(10, CONST_MENU_INDENT_WIDTH+(curloc-scroll)*8, MENUCONST_MENUCARET);
  }
  
  void lcdmenu2::doScroll()
  {
      if (curloc<0)
      {
          curloc=0;
      }
      else {
              while (curloc>0&&!curMenu->getChild(curloc)) // only allow it to go up to Menu item (one more if back button enabled)
              {
                curloc--;
              }
           }
  
      if (curloc >= (rows+scroll))
      {
          scroll++;
          display();
      } else 
      if (curloc < (scroll))
      {
        scroll--;
        display();
      } else
        {
          setCursor();
        }
  }
    
  void lcdmenu2::goUp()
  {
      curloc-=1;
      doScroll();
  }
  
  void lcdmenu2::goDown()
  {
      curloc+=1;
      doScroll();
  }

  void lcdmenu2::goTop()
  {
      // go topmost in the menu (11.13.11)
      curloc=0;
      doScroll();    
  }
  
  void lcdmenu2::goBack()
  {
      if(layer > 0)
      {
          back = 1;       
          goMenu(*curMenu->getParent());          
      } else { goTop(); } // new 11.13.11
  }
  
  void lcdmenu2::goEnter()
  {
      menu *tmp;
      tmp=curMenu;
      if ((tmp=tmp->getChild(curloc)))
      {  // the child exists
          if (tmp->canEnter)
          {  // canEnter function is set
              if (tmp->canEnter(*tmp))
              {  // it wants us to enter
                  goMenu(*tmp);
              }
          }
          else {
                // canEnter function not set, assume entry allowed
                goMenu(*tmp);                
                curfuncname = tmp->name;
               }
      }
      else { // child did not exist  The only time this should happen is on the back Menu item, so go back
             goBack();
           }
  }

  void lcdmenu2::goSettings(menu &m)
  {
      // go to settings
      curloc=0;
      layer = 0;
      back = 0;
      goMenu(m);
  }
  
  void lcdmenu2::goMenu(menu &m)
  {
  uint8_t diff;
  
      curMenu=&m;  
      if(layer < 8)
      {
          scroll = 0;
          if(back == 0)
          {
              layer_save[layer] = curloc;
              layer++;
              curloc = 0;
          } else
            {
              back = 0;
  
              if(layer > 0)
              {
                  layer--;
                  curloc = layer_save[layer];
                  
                  if(curloc >= rows)
                  {
                    diff = curloc-(rows-1);
                    for(int i=0; i<diff; i++)
                    {
                        doScroll();
                    }
                  }
              }
            }
      }
  
      if(layer >= 0 && layer <5)
      {
        funcname[layer-1] = curMenu->name;
      }
  
      // show menu if there is a further child
      if (curMenu->ChildExists() == true) // 11.30.11
      {
        display();
      }
  }

  void lcdmenu2::display()
  {
  menu * tmp;
  int i = scroll;
  int j = 0;
  int maxi=(rows+scroll);
  char space[3];
  
      TV.clear_screen();

      // draw menu header & line
      TV.printPGM(6, 6, MENUCONST_MENU);
      TV.draw_line(0,14, SCREENWIDTH-1, 14, WHITE); 
  
      if ( (tmp=curMenu->getChild(0)) )
      {
          do
          {
              j++;
          } while ( (tmp=tmp->getSibling(1)) );
      }
      j--;
  
      if ( (tmp=curMenu->getChild(i)) )
      {
          do {
              space[0]=32; space[1]=32; space[2]=0;
              TV.print(10, CONST_MENU_INDENT_WIDTH+(i-scroll)*8, space); // not flashing
  
              TV.printPGM(18, CONST_MENU_INDENT_WIDTH+(i-scroll)*8, (const char *) tmp->name);
              i++;
          } while ((tmp=tmp->getSibling(1))&&i<maxi);
      }
      else { // no children
             goBack();
           }
      setCursor();
  }
  
  char lcdmenu2::getChar(size_t n)
  {
    return pgm_read_byte(((const prog_char*)curfuncname) + n);
  }

////////////////////////////////////////////////////////////
  
  // menu.cpp
   void menu::setParent(menu &p)
  {
    parent=&p;
  }
  
  void menu::addSibling(menu &s,menu &p)
  {
    if (sibling)
    {
     sibling->addSibling(s,p);
    } else {
             sibling=&s;
             sibling->setParent(p);
           }
  }

  menu::menu(const prog_char *n)
  {    
    name = (const __FlashStringHelper*)n;
    canEnter=NULL;
  }
  
  menu::menu(const prog_char *n, boolean (*c)(menu&))
  {
    name = (const __FlashStringHelper*)n;
    canEnter=c;
  }

  void menu::addChild(menu &c)
  {
    if (child)
    {
     child->addSibling(c,*this);
    } else {
             child=&c;
             child->setParent(*this);
           }
  }
  
  boolean menu::ChildExists()
  {
    if (child)
      return true;
    else
      return false;
  }
  
  menu * menu::getChild(int which)
  {
    if (child)
    {
     return child->getSibling(which);
    } else  {
              // this menu item has no children
             return NULL;
            }
  }
  
  menu * menu::getSibling(int howfar)
  {
    if (howfar==0)
    {
     return this;
    } else
    if (sibling)
    {
     return sibling->getSibling(howfar-1);
    } else // asking for a nonexistent sibling
      {
        return NULL;
      }
  }
  
  menu * menu::getParent()
  {
    if (parent)
     return parent;
    else // top menu
       return this;
  }
// end of radio1_lcdmenu2.cpp

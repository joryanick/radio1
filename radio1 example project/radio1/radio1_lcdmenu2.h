// radio1_lcdmenu2.h
//
// GoodPrototyping radio1 rev.C5
// This file is part of a modified menu system released into public domain

//- Autor:   Jomelo
//- ICQ:     78048329
//- License: all Free
//- Edit:    2009.09.11
//- Lastedit:10.09.2009

#ifndef lcdmenu2_h
#define lcdmenu2_h

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "radio1_defs.h"

class menu
{
private:
 menu * parent;  // parent menu, NULL if this is the top
 menu * child;   // first child menu, NULL if no children
 menu * sibling; // next sibling menu, NULL if this is the last sibling

 void setParent(menu &p);          // sets the menu parent to p
 void addSibling(menu &s,menu &p); // adds a sibling s with parent p. if the menu already has a sibling, ask that sibling to add it
 
 public:
 const __FlashStringHelper * name;              // name of this menu
 menu(const prog_char *n);                      // constructs the menu with a name and a NULL use function (be careful calling it)
 menu(const prog_char *n, boolean (*c)(menu&)); // constructs the menu with a specified use function
   
 boolean (*canEnter)(menu&);    // function to be called when this menu is 'used'
 
 void addChild(menu &c);        // adds the child c to the menu. if the menu already has a child, ask the child to add it as a sibling
 boolean ChildExists();         // returns state of a child menu in current item
 menu * getChild(int which);    // returns a pointer to a specific child of this menu
 menu * getSibling(int howfar); // returns a pointer to the sibling howfar siblings away from this menu
 menu * getParent();            // returns this menu's parent menu. if no parent, returns itself
};

//# Lcd Menu 2
//# =======================
class lcdmenu2
{
    private:
        menu * rootMenu;
        menu * curMenu;     

        uint8_t cols;               // columns
        uint8_t rows;               // rows
        
        uint8_t layer_save[8];      // previous cursor position
        uint8_t layer;              // levels
        uint8_t back;               // set back
       
        int curloc;                 // actual cursor position
        int scroll;                 // actual scroll position

        uint8_t cursor_pos;         // cursor position

    public:
        const __FlashStringHelper* curfuncname;
        const __FlashStringHelper* funcname[5]; // store the last function names, up to the 3rd level            
        
        lcdmenu2(menu &r, uint8_t row, uint8_t cols);

        void setCursor();         // set the cursor on the menu
        void doScroll();          // scroll to the new menu item
        void goMenu(menu &m);     // go directly to menu m

        void goTop();             // move to item #1 if menu button is pressed while in the menu
        void goSettings(menu &m); // move to settings menu item #1 with redraw of menu
        void goUp();              // move cursor up
        void goDown();            // move cursor down
        void goBack();            // move to the parent menu
        void goEnter();           // activate the menu under the cursor
        
        void display();           // display the current menu on the LCD         
        
        char getChar(size_t n);   // get character in menu name at position
};
#endif // lcdmenu2_h

// end of radio1_lcdmenu2.h

/*  This file contains routines that create menus and menu items within
 * those menus.
 *
 *                     This code is under the GNU Copyleft.
 *
 *  Dominic Giampaolo
 *  dbg@sgi.com
 */
#include <stdio.h>
#include <stdlib.h>
#include "xstuff.h"
#include <X11/Xaw3dxft/MenuButton.h>
#include <X11/Xaw3dxft/SimpleMenu.h>
#include <X11/Xaw3dxft/SmeBSB.h>
#include "libsx.h"
#include "libsx_private.h"
#include "check_mark.h"

static Widget popped_up = None;
static Widget popped_parent = None;
static int transitional = 0;

static void
PopdownMenusGlobal()
{
    if (popped_up != None) {
        if (XtIsRealized(popped_up))
            XtPopdown(popped_up);
        popped_up = None;
    }
    popped_parent = None;
    transitional = 0;
}

static void
PopdownEscape(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
    transitional = 0;
    if (XtIsRealized(w)) XtPopdown(w);
    if (w != popped_up) PopdownMenusGlobal();
    popped_up = None;
    popped_parent = None;
}

static void
PopdownAll(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
    if (transitional) {
        transitional = 0;
        return;
    }
    if (XtIsRealized(w)) XtPopdown(w);
    if (w != popped_up) PopdownMenusGlobal();
}

static void 
PopdownChild(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
    Position y;

    XtVaGetValues(w, XtNy, &y, NULL);
    y = event->xbutton.y_root-y;

    if (event->type == ButtonRelease && y>0) {
        if (!transitional) {
	    XtPopdown(w);
            if (w == popped_up) popped_up = None;
	}
        transitional = 0;
    }
}

void SetFocusOn(Widget w)
{
  /* No longer useful ? */
    XWindowAttributes win_attributes;
    if (!w) return;
    if (!XtIsRealized(w)) return;
    XGetWindowAttributes(XtDisplay(w), XtWindow(w), &win_attributes);
    if (win_attributes.map_state==IsViewable)
        XSetInputFocus(XtDisplay(w), XtWindow(w), RevertToPointerRoot, CurrentTime);
}

static void 
HighlightChild(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
    Position x, y;
    Dimension wi, he;

    x = y = 0;
    XtVaGetValues(w, XtNx, &x, XtNy, &y, XtNwidth, &wi, XtNheight, &he, NULL);
    x = event->xbutton.x_root-x;
    y = event->xbutton.y_root-y;

    if (event->type == ButtonPress || event->type == MotionNotify) {
        if (x>=0 && x<=wi && y>=0 && y<=he)
            XtCallActionProc(w, "highlight", event, params, 0);
        else
            XtCallActionProc(w, "unhighlight", event, params, 0);
        SetFocusOn(w);
    }
    SetFocusOn(popped_up);
}

static char defaultTranslations[] = "<Key>Escape: popdown()\n <Enter>: highlight-child()\n <Leave>: unhighlight()\n <Motion>: highlight-child()\n <Btn1Up>: notify() unhighlight() popdown-child()\n <Btn2Up>: unhighlight() popdown-all()\n <Btn3Up>: unhighlight() popdown-all()\n <Btn1Down>: highlight-child()\n <Btn2Down>: highlight-child()\n <Btn3Down>: highlight-child()";

static XtActionsRec actionsList[5] = {
        {"popdown-all",  (XtActionProc) PopdownAll},
        {"escape",  (XtActionProc) PopdownEscape},
        {"popdown-child",  (XtActionProc) PopdownChild},
        {"highlight-child",  (XtActionProc) HighlightChild}
};

/*
 * Menu functions.
 */
Widget MakeMenu(char *name)
{
  int    n = 0;
  Arg    wargs[5];		/* Used to set widget resources */
  Widget button, menu=NULL;

  if (lsx_curwin->toplevel == NULL && OpenDisplay(0,NULL) == 0)
    return NULL;
  if (name == NULL)
    return NULL;
  
  n = 0;
  XtSetArg(wargs[n], XtNlabel, name);	 	          n++;  
  XtSetArg(wargs[n], XtNborderWidth, 1); 	          n++;  


  /*
   * We create two widgets, the simpleMenu widget is a child of the menu button
   * widget.  We return the reference to the button widget however so that
   * way the SetXXColor() and SetWidgetFont() functions work as expect
   * (that is they change the menu button).
   *
   * The MakeMenuItem() function is aware of this, and gets the child of
   * the menu button for creation of the actual menu items.
   *
   */
  button = XtCreateManagedWidget("menuButton", menuButtonWidgetClass,
				 lsx_curwin->form_widget,  wargs, n); 

  if (button)
    menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, button, 
			      NULL, 0);
  
  if (menu == NULL)
   {
     XtDestroyWidget(button);
     button = NULL;
   }

  XtAppAddActions(XtWidgetToApplicationContext(menu), actionsList, 4);
  XtOverrideTranslations(menu, XtParseTranslationTable(defaultTranslations));

  return button;
}


Widget MakeMenuItem(Widget parent, char *name, ButtonCB func, void *arg)
{
  int    n = 0;
  Arg    wargs[5];		/* Used to set widget resources */
  Widget item, menu;

  if (lsx_curwin->toplevel == NULL && OpenDisplay(0,NULL) == 0)
    return NULL;
  if (parent == NULL)
    return NULL;

  /*
   * We get the "menu" widget child of the parent widget because the
   * parent is really a reference to the menu button widget, not the
   * popup-shell widget we really want.  See the above comment in
   * MakeMenu().
   */    
  if ((menu = XtNameToWidget(parent, "menu")) == NULL)
    return NULL;

  n = 0;
  XtSetArg(wargs[n], XtNlabel,      name);	          n++;  
  XtSetArg(wargs[n], XtNleftMargin, check_mark_width);    n++;  


  item = XtCreateManagedWidget("menu_item", smeBSBObjectClass, menu, wargs, n);
  if (item == NULL)
    return NULL;

  
  if (func)
    XtAddCallback(item, XtNcallback, (XtCallbackProc)func, arg);

  return item;
}



void SetMenuItemChecked(Widget w, int state)
{
  int n=0;
  Arg wargs[5];
  Pixmap mark;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return;

  if (lsx_curwin->check_mark == None)  /* create the check mark bitmap */
   {
     Display *d=XtDisplay(XtParent(w));

     mark = XCreateBitmapFromData(d, DefaultRootWindow(d),
				  (char *)check_mark_bits,
				  check_mark_width, check_mark_height);
     if (mark == None)
       return;

     lsx_curwin->check_mark = mark;
   }
  else
    mark = lsx_curwin->check_mark;    


  /*
   * Now set the check mark.
   */
  n=0;
  XtSetArg(wargs[n], XtNleftMargin, 16);          n++;
  if (state)
    { XtSetArg(wargs[n], XtNleftBitmap, mark);    n++; }  /* checkmark on */
  else
    { XtSetArg(wargs[n], XtNleftBitmap, None);    n++; }  /* checkmark off */

  XtSetValues(w, wargs, n);
}



int GetMenuItemChecked(Widget w)
{
  int n=0;
  Arg wargs[5];
  Pixmap mark;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return FALSE;
  
  n=0;
  XtSetArg(wargs[n], XtNleftBitmap, &mark);    n++;
  XtGetValues(w, wargs, n);

  if (mark == None)
    return FALSE;
  else
    return TRUE;
}

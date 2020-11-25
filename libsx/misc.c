/*  This file contains miscellaneous routines that apply to all kinds
 * of widgets.  Things like setting and getting the color of a widget,
 * its font, position, etc.
 *
 *                     This code is under the GNU Copyleft.
 *
 *  Dominic Giampaolo
 *  dbg@sgi.com
 */
#include <stdio.h>
#include <stdlib.h>
#include "xstuff.h"
#include "libsx.h"
#include "libsx_private.h"


/*
 * Miscellaneous functions that allow customization of widgets and
 * other sundry things.
 */

void
SetWindowTitle(char *title)
{
  XTextProperty xtxt;

  if (lsx_curwin->toplevel == NULL || title == NULL)
    return;

  XStringListToTextProperty(&title, 1, &xtxt);

  XSetWMName(lsx_curwin->display, XtWindow(lsx_curwin->toplevel), &xtxt);
}


void
SetIconTitle(char *title)
{
  XTextProperty xtxt;

  if (lsx_curwin->toplevel == NULL || title == NULL)
    return;

  XStringListToTextProperty(&title, 1, &xtxt);

  XSetWMIconName(lsx_curwin->display, XtWindow(lsx_curwin->toplevel), &xtxt);
}


void SetFgColor(Widget w, int color)
{
  int    n = 0;
  Arg    wargs[1];		/* Used to set widget resources */
  DrawInfo *di;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return;

  if ((di=libsx_find_draw_info(w)) != NULL)
   {
     Display *d=XtDisplay(w);
     
     di->foreground = color;

     if (di->mask != 0xffffffff)
       XSetPlaneMask(d, di->drawgc,  di->foreground ^ di->background);
     else
       XSetForeground(d, di->drawgc, color);
       
     return;
   }


  n = 0;
  XtSetArg(wargs[n], XtNforeground, color);	     n++;  

  XtSetValues(w, wargs, n);
}



void SetBgColor(Widget w, int color)
{
  int    n = 0;
  Arg    wargs[1];		/* Used to set widget resources */
  Widget tmp;
  DrawInfo *di;

  if (lsx_curwin->toplevel == NULL || w == NULL)
     return;


  if ((di=libsx_find_draw_info(w)) != NULL)
   {
     Display *d=XtDisplay(w);
     
     XSetBackground(d, di->drawgc, color);
     XSetWindowBackground(d, XtWindow(w), color);
     di->background = color;

     if (di->mask != 0xffffffff)
       XSetPlaneMask(d, di->drawgc, di->foreground ^ di->background);

     return;
   }

  tmp = XtParent(w);
  if (tmp != lsx_curwin->form_widget)
   {
     if (XtNameToWidget(tmp, "menu_item"))
       w = tmp;
   }

  n = 0;
  XtSetArg(wargs[n], XtNbackground, color);	     n++;  

  XtSetValues(w, wargs, n);
}



int GetFgColor(Widget w)
{
  Pixel  color;
  int    n = 0;
  Arg    wargs[1];		/* Used to set widget resources */
  DrawInfo *di;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return -1;

  if ((di=libsx_find_draw_info(w)) != NULL)
   {
     return di->foreground;
   }

  n = 0;
  XtSetArg(wargs[n], XtNforeground, &color);	     n++;  

  XtGetValues(w, wargs, n);

  return color;
}



int GetBgColor(Widget w)
{
  Pixel  color;
  int    n = 0;
  Arg    wargs[1];		/* Used to set widget resources */
  Widget tmp;
  DrawInfo *di;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return -1;

  if ((di=libsx_find_draw_info(w)) != NULL)
   {
     return di->background;
   }

  tmp = XtParent(w);
  if (tmp != lsx_curwin->form_widget)
   {
     if (XtNameToWidget(tmp, "menu_item"))
       w = tmp;
   }

  n = 0;
  XtSetArg(wargs[n], XtNbackground, &color);	     n++;  

  XtGetValues(w, wargs, n);

  return color;
}


void SetBorderColor(Widget w, int color)
{
  int    n = 0;
  Arg    wargs[1];		/* Used to set widget resources */

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return;

  n = 0;
  XtSetArg(wargs[n], XtNborder, color);	 	     n++;  

  XtSetValues(w, wargs, n);
}





void SetWidgetState(Widget w, int state)
{
  int    n = 0;
  Arg    wargs[1];		/* Used to set widget resources */

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return;

  n = 0;
  XtSetArg(wargs[n], XtNsensitive, state);	     n++;  

  XtSetValues(w, wargs, n);
}



int GetWidgetState(Widget w)
{
  int    n = 0, state;
  Arg    wargs[1];		/* Used to set widget resources */

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return 0;

  n = 0;
  XtSetArg(wargs[n], XtNsensitive, &state);	     n++;  

  XtGetValues(w, wargs, n);

  return state;
}



void SetWidgetBitmap(Widget w, char *data, int width, int height)
{
  Pixmap pm;
  Display *d;
  Arg wargs[3];
  int  n=0;

  if (lsx_curwin->display == NULL || w == NULL)
    return;

  d = XtDisplay(w);
  
  pm = XCreateBitmapFromData(d, DefaultRootWindow(d), data, width, height);
  if (pm == None)  
    return;

  n=0;
  XtSetArg(wargs[n], XtNbitmap, pm);    n++;
  XtSetValues(w, wargs, n);  
}

#ifdef  XPM_SUPPORT
void SetWidgetPixmap(Widget w, char **xpmdata)
{
  Pixmap xpm;
  XpmAttributes xpma;
  Arg wargs[4];       /* Used to set widget resources */
  int n = 0;

  if (lsx_curwin->toplevel == NULL && OpenDisplay(0, NULL) == 0)
    return;

  xpma.colormap = DefaultColormap(lsx_curwin->display,
                             DefaultScreen(lsx_curwin->display));
  xpma.valuemask = XpmColormap;
  XpmCreatePixmapFromData(lsx_curwin->display, 
            DefaultRootWindow(lsx_curwin->display), 
            xpmdata, &xpm, NULL, &xpma);

  n = 0;
  XtSetArg(wargs[n], XtNwidth, xpma.width);
  n++;
  XtSetArg(wargs[n], XtNheight, xpma.height);
  n++;
  XtSetArg(wargs[n], XtNbackgroundPixmap, xpm); 
  n++;
  XtSetValues(w, wargs, n);  
}
#endif

void AttachEdge(Widget w, int edge, int attach_to)
{
  char *edge_name;
  static char *edges[]    = { XtNleft,     XtNright,     XtNtop,
			      XtNbottom };
  static int   attached[] = { XtChainLeft, XtChainRight, XtChainTop,
			      XtChainBottom };
  int   a;
  Arg wargs[5];
  int n=0;


  if (w == NULL || edge < 0 || edge > BOTTOM_EDGE
      || attach_to < 0 || attach_to > ATTACH_BOTTOM)
    return;
  
  edge_name = edges[edge];
  a         = attached[attach_to];
  
  n=0;
  XtSetArg(wargs[n], edge_name, a);     n++;

  XtSetValues(w, wargs, n);
}




void SetWidgetPos(Widget w, int where1, Widget from1, int where2, Widget from2)
{
  int  n = 0;
  Arg  wargs[5];		/* Used to set widget resources */
  char *name;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return;

  /*
   * Don't want to do this for menu item widgets
   */
  if ((name = XtName(w)) && strcmp(name, "menu_item") == 0)
    return;

  /*
   * In the case that the widget we were passed is a list widget, we
   * have to play a few games.  We can't set the position of a list
   * widget directly because it is a composite widget.  We want to
   * set the position of the parent of the list widget (a viewport
   * widget) instead.  Normally the parent of a widget will be the
   * current lsx_curwin->form_widget, so if that isn't the case,
   * we have a composite list widget and really want its parent.
   *
   * The extra check for the name of the widget not being "form" is to
   * allow proper setting of multiple form widgets.  In the case that
   * we are setting the position of multiple form widgets, the parent of 
   * the widget we are setting will not be lsx_curwin->form_widget.  When
   * this is the case, we just want to set the widget itself, not the
   * parent.  Basically this is an exception to the previous paragraph
   * (and it should be the only one, I think).
   *
   * Kind of hackish.  Oh well...
   */
  if (XtParent(w) != lsx_curwin->form_widget && strcmp(XtName(w), "form") != 0)
    w = XtParent(w);


  /*
   * If we're placing this new widget (w) relative to a list widget,
   * and the parent of the list widget is not the form_widget, then
   * we should grab the parent of the list widget and use that for
   * doing relative placement (because the list widget is composite).
   */
  if (where1 != NO_CARE)
   {
     name = XtName(from1);
     if (strcmp(name, "list") == 0 &&
	 XtParent(from1) != lsx_curwin->form_widget)
       from1 = XtParent(from1);
   }

  if (where2 != NO_CARE)
   {
     name = XtName(from2);
     if (strcmp(name, "list") == 0 &&
	 XtParent(from2) != lsx_curwin->form_widget)
       from2 = XtParent(from2);
   }


  /*
   * Here we do the first half of the positioning.
   */
  if (where1 == PLACE_RIGHT && from1)
   { 
     XtSetArg(wargs[n], XtNfromHoriz, from1);              n++; 
   }
  else if (where1 == PLACE_UNDER && from1)
   { 
     XtSetArg(wargs[n], XtNfromVert,  from1);              n++; 
   }


  /*
   * Now do the second half of the positioning
   */
  if (where2 == PLACE_RIGHT && from2)
   { 
     XtSetArg(wargs[n], XtNfromHoriz, from2);              n++; 
   }
  else if (where2 == PLACE_UNDER && from2)
   { 
     XtSetArg(wargs[n], XtNfromVert,  from2);              n++; 
   }


  if (n)                      /* if any values were actually set */
    XtSetValues(w, wargs, n);
}


void
SetWidgetSize(Widget w, int width, int height)
{
  int n = 0;
  Arg wargs[2];

  if (width > 0)
   {
     /* printf("setting widget width: %d\n", width); */
     XtSetArg(wargs[n], XtNwidth, width);
     n++;
   }

  if (height > 0)
   {
     /* printf("setting widget height: %d\n", height); */
     XtSetArg(wargs[n], XtNheight, height);
     n++;
   }
  
  if (n > 0 && w != NULL)
    XtSetValues(w, wargs, n);
}

void
GetWidgetSize (Widget w, int *width, int *height)
{
  int n;
  Arg wargs[2];
  Dimension nwidth, nheight;
  Widget tmp;
  
  if (w == NULL) return;
  if (width == NULL && height == NULL) return;

  tmp = XtParent (w);
  if (tmp != lsx_curwin->form_widget)
    {
      if (XtNameToWidget (tmp, "menu_item"))
	w = tmp;
    }

  n = 0;
  XtSetArg (wargs[n], XtNwidth, &nwidth);
  n++;
  XtSetArg (wargs[n], XtNheight, &nheight);
  n++;

  XtGetValues (w, wargs, n);

  if (width)  *width = nwidth;
  if (height) *height = nheight;
}


/*
void
SetIconBitmap(unsigned char *bits, int width, int height)
{
  Pixmap pix;
  XWMHints *xwmh;
  
#if 0
  if (lsx_curwin == NULL || lsx_curwin->display == NULL)
    return;
  
  pix = XCreatePixmapFromBitmapData(lsx_curwin->display,
				    lsx_curwin->window,
				    bits, width, height,
				    cur_di->foreground,
				    cur_di->background,
				    DefaultDepth(display,lsx_curwin->screen));
  if (pix == NULL)
    return;
#endif
}


void
SetIconImage(unsigned char *bitmap, int width, int height)
{
}

*/


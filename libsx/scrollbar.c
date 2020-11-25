/* This file contains routines to manipulate scrollbar widgets (either
 * horizontal or vertical ones).
 *
 *                     This code is under the GNU Copyleft.
 *
 *  Dominic Giampaolo
 *  dbg@sgi.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "xstuff.h"

#ifdef sgi 
/*
 * little fixes for the botched up SGI Xaw3dxft/Scrollbar.h header file...
 */
#define NeedFunctionPrototypes 1    /* Make DAMN sure we pick up prototypes */
#undef NeedWidePrototypes 
#endif

#ifdef __linux__
#define NeedFunctionPrototypes 1    /* Make DAMN sure we pick up prototypes */
#undef NeedWidePrototypes 
#endif

#include <X11/Xaw3dxft/Scrollbar.h>
#include "libsx.h"
#include "libsx_private.h"


/*
 * this structure maintains some internal state information about each
 * scrollbar.
 */
typedef struct ScrollInfo
{
  Widget scroll_widget;
  void  (*func)(Widget widg, float val, void *data);
  float max;
  float where_is;
  float size_shown;
  float val;
  float fraction;
  void  *user_data;

  struct ScrollInfo *next;
}ScrollInfo;

static ScrollInfo *scroll_bars = NULL;

/*
 * This is called when the user interactively uses the middle mouse
 * button to move the slider.
 */
static void
my_jump_proc(Widget scroll_widget, XtPointer client_data, XtPointer percent)
{
  ScrollInfo *si = (ScrollInfo *)client_data;
  float old_val = si->val;
  
  /* We want the scrollbar to be at 100% when the right edge of the slider
   * hits the end of the scrollbar, not the left edge.  When the right edge
   * is at 1.0, the left edge is at 1.0 - size_shown.  Normalize
   * accordingly.
   */
   
  si->val = (*(float *) percent) * si->max;

  if ((si->val + si->size_shown > si->max)
      && fabs((double)(si->size_shown - si->max)) > 0.01)
   {
     si->val = si->max - si->size_shown;
     XawScrollbarSetThumb(scroll_widget, si->val/si->max, 
                                       si->size_shown / si->max); 
   }
  else if (si->val <= 0)
    si->val = 0;

  si->where_is = si->val;
  
  if (si->func && old_val != si->val)
    (*si->func)(si->scroll_widget, si->val, si->user_data);
}

/*
 * This is called whenever the user uses the left or right mouse buttons
 */
static void
my_scroll_proc(Widget scroll_widget, XtPointer client_data, XtPointer position)
{
  int   pos;
  ScrollInfo *si = (ScrollInfo *)client_data;
  float old_val = si->val;
  
  pos = (int)(long int)position;
  
  if (pos < 0)   /* button 3 pressed, go up */
   {
     si->val += (si->fraction * si->max);   /* go up ten percent at a time */
   }
  else           /* button 2 pressed, go down */
   {
     si->val -= (si->fraction * si->max);   /* go down ten percent at a time */
   }

  
  if (si->size_shown != si->max && (si->val + si->size_shown) >= si->max)
    si->val = si->max - si->size_shown;
  else if (si->val >= si->max)
    si->val = si->max;
  else if (si->val <= 0.0)
    si->val = 0.0;
  
  XawScrollbarSetThumb(scroll_widget,si->val/si->max, 
                                           si->size_shown/si->max);

  si->where_is = si->val;
  if (si->func && old_val != si->val)
    (*si->func)(si->scroll_widget, si->val, si->user_data);
}

/*
static void destroy_scroll_info(Widget w, void *data, void *junk)
{
  ScrollInfo *si = (ScrollInfo *)data;
  ScrollInfo *curr;

  if (si == scroll_bars)
    scroll_bars = si->next;
  else
   {
     for(curr=scroll_bars; curr && curr->next != si; curr=curr->next)
  
     if (curr == NULL)
       return;

     curr->next = si->next;
   }

  free(si);
}
*/

void SetScrollbar(Widget w, float where, float max, float size_shown)
{
  ScrollInfo *si;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return;


  /*
   * Here we have to search for the correct ScrollInfo structure.
   * This is kind of hackish, but is the easiest way to make the
   * interfaces to all this easy and consistent with the other
   * routines.
   */
  for(si=scroll_bars; si; si=si->next)
   {
     if (si->scroll_widget == w && XtDisplay(si->scroll_widget) == XtDisplay(w))
       break;
   }

  if (si == NULL)
    return;

  si->where_is = where;
  if (max > -0.0001 && max < 0.0001)
    max = 0.0001;

  si->max = max;
  if (fabs((double)max - size_shown) > 0.01)
    si->max += size_shown;
  si->size_shown = size_shown;
  si->val = si->where_is;
  
  XawScrollbarSetThumb(w, si->where_is/si->max, si->size_shown/si->max);
}

/*
 * Scrollbar Creation/Manipulation routines.
 *
 */
static Widget MakeScrollbar(int length, ScrollCB scroll_func, void *data,
			    int orientation)
{
  int    n = 0;
  Arg    wargs[7];		/* Used to set widget resources */
  Widget scroll_widget;
  ScrollInfo *si;

  if (lsx_curwin->toplevel == NULL && OpenDisplay(0, NULL) == 0)
    return NULL;
    
  si = (ScrollInfo *)calloc(sizeof(ScrollInfo),1);
  if (si == NULL)
    return NULL;

  si->func       = scroll_func;
  si->user_data  = data;
  si->size_shown = si->max = 1.0;
  si->val = si->where_is = 0.0;
  si->fraction = 0.1;

  n = 0;
  XtSetArg(wargs[n], XtNorientation, orientation);   n++; 
  XtSetArg(wargs[n], XtNlength,      length);        n++; 
  XtSetArg(wargs[n], (orientation==XtorientHorizontal)?
                     XtNwidth:XtNheight,       length);        n++; 

  scroll_widget = XtCreateManagedWidget("scrollbar", scrollbarWidgetClass,
					lsx_curwin->form_widget, wargs,n);  

  if (scroll_widget == NULL)
   {
     free(si);
     return NULL;
   }
  si->scroll_widget = scroll_widget;
  si->next = scroll_bars;
  scroll_bars = si;

  XtAddCallback(scroll_widget, XtNjumpProc,   my_jump_proc,   si);
  XtAddCallback(scroll_widget, XtNscrollProc, my_scroll_proc, si);
  
  return scroll_widget;
}


Widget MakeVertScrollbar(int height, ScrollCB scroll_func, void *data)
{
  return MakeScrollbar(height, scroll_func, data, XtorientVertical);
}


Widget MakeHorizScrollbar(int length, ScrollCB scroll_func, void *data)
{
  return MakeScrollbar(length, scroll_func, data, XtorientHorizontal);
}

float __dir__ = 1.0;

void SetScrollbarDirection(float dir)
{
  __dir__ = dir;
}

void SetScrollbarStep(Widget w, float fact)
{
  ScrollInfo *si;

  if (lsx_curwin->toplevel == NULL || w == NULL)
    return;

  /*
   * Here we have to search for the correct ScrollInfo structure.
   * This is kind of hackish, but is the easiest way to make the
   * interfaces to all this easy and consistent with the other
   * routines.
   */
  for(si=scroll_bars; si; si=si->next)
   {
     if (si->scroll_widget == w && XtDisplay(si->scroll_widget) == XtDisplay(w))
       break;
   }

  if (si == NULL)
    return;

  si->fraction = fact * __dir__;
}

void SetThumbBitmap(Widget w, char *black_bits, int width, int height)
{

  Display *d;
  Pixmap thumb_pm;
  Arg    wargs[5];		/* Used to set widget resources */

  d = XtDisplay (lsx_curwin->toplevel);
  thumb_pm = XCreateBitmapFromData (d, DefaultRootWindow(d),
  		black_bits, width, height);

  if (thumb_pm) {
     XtSetArg(wargs[0], XtNthumb, thumb_pm);
  } else
     printf ("\nError, can't make thumb pixmap !!\n");

  XtSetValues(w, wargs, 1);
}

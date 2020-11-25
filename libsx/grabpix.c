/* $XConsortium: grabpix.c,v 1.29 94/04/17 20:25:02 rws Exp $  */

#include <stdio.h>
#include "xstuff.h"
#include "libsx.h"
#include "libsx_private.h"

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/Xaw3dxft/Paned.h>
#include <X11/Xaw3dxft/Command.h>
#include <X11/Xaw3dxft/Label.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/Error.h>

typedef struct _RootWindowClassRec*	RootWindowWidgetClass;
typedef struct _RootWindowRec*	RootWindowWidget;
extern WidgetClass rootWindowWidgetClass;

typedef struct {
    int empty;
} RootWindowClassPart;

typedef struct _RootWindowClassRec {
    CoreClassPart	core_class;
    RootWindowClassPart	root_class;
} RootWindowClassRec;

extern RootWindowClassRec rootClassRec;

typedef struct {
    /* resources */
    char* resource;
    /* private state */
} RootWindowPart;

typedef struct _RootWindowRec {
    CorePart	core;
    RootWindowPart	root;
} RootWindowRec;

static void Realize();
/*
static void InitCursors();
static void SetupGC();
static Window FindWindow(int x, int y);
static void CreateRoot();
static void StartRootPtrGrab();
*/

RootWindowClassRec rootWindowClassRec = {
  { /* core fields */
    /* superclass		*/	(WidgetClass) &widgetClassRec,
    /* class_name		*/	"RootWindow",
    /* widget_size		*/	sizeof(RootWindowRec),
    /* class_initialize		*/	NULL,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	NULL,
    /* initialize_hook		*/	NULL,
    /* realize			*/	Realize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	NULL,
    /* num_resources		*/	0,
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	NULL,
    /* resize			*/	NULL,
    /* expose			*/	NULL,
    /* set_values		*/	NULL,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry		*/	XtInheritQueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
  { /* rootWindow fields */
    /* empty			*/	0
  }
};

WidgetClass rootWindowWidgetClass = (WidgetClass)&rootWindowClassRec;

typedef struct { 
  GC        gc;
  XWindowAttributes win_info;   
  XImage     *image;
  Position  homeX, homeY, x, y;
  Dimension width, height;
  } hlStruct, *hlPtr;

/*ARGSUSED*/
static void Realize(w, value_mask, attributes)
    Widget	w;
    XtValueMask *value_mask;
    XSetWindowAttributes *attributes;
{
    w->core.window = RootWindowOfScreen(w->core.screen);
}


static Cursor newcursor;
static Display *dpy;
static int scr;
static GC selectGC; 
static XGCValues selectGCV;
static Widget toplevel, root;

static char lens_bits[] = {
   0x00, 0x00, 0xf8, 0x00, 0x24, 0x01, 0x22, 0x02, 0x02, 0x02, 0x8e, 0x03,
   0x02, 0x02, 0x22, 0x02, 0x24, 0x07, 0xf8, 0x0d, 0x00, 0x13, 0x00, 0x22,
   0x00, 0x44, 0x00, 0x28, 0x00, 0x10, 0x00, 0x00};

static char lensMask_bits[] = {
   0xf8, 0x00, 0xfc, 0x01, 0xfe, 0x03, 0x77, 0x07, 0x8f, 0x07, 0x8f, 0x07,
   0x8f, 0x07, 0x77, 0x07, 0xfe, 0x0f, 0xfc, 0x1f, 0xf8, 0x3f, 0x00, 0x7f,
   0x00, 0xfe, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x10};

static int sec = 2;  /* security border strip */
static unsigned int srcWidth=1, srcHeight=1;
/* static char formatstr[256]="Pixel at (%x,%y) colored [%r,%g,%b]\n\
Window object 0x%w, colormap entry %c, button %m\n"; */
static char output[256];

#define lens_width 16
#define lens_height 16
#define lens_x_hot 5
#define lens_y_hot 5

#define lensMask_width 16
#define lensMask_height 16
#define lensMask_x_hot 5
#define lensMask_y_hot 5

static void InitCursors()
{
Pixmap cursor, mask;
XColor cfor,cbak;

  cbak.red=0xffff;   cbak.green=0xffff;   cbak.blue=0xffff;
  cfor.red=0;   cfor.green=0;   cfor.blue=0;

  cursor = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), 
                         lens_bits,lens_width,lens_height);
  mask = XCreateBitmapFromData(dpy,DefaultRootWindow(dpy), 
                   lensMask_bits,lensMask_width, lensMask_height);
  newcursor = XCreatePixmapCursor(dpy, cursor, mask, &cfor, &cbak,
                   lens_x_hot,lens_y_hot);
}

void SetupGC()
{
    selectGCV.function = GXxor;
    selectGCV.foreground = 1L;
    selectGCV.subwindow_mode = IncludeInferiors;
    selectGC = XtGetGC(toplevel, GCFunction|GCForeground|GCSubwindowMode,
		       &selectGCV);
}  

Window FindWindow(x, y)
     int x, y;	  
{
  XWindowAttributes wa;
  Window findW = DefaultRootWindow(dpy), stopW, childW;

  XTranslateCoordinates(dpy, findW, findW,
			x, y, &x, &y, &stopW);
  while (stopW) {
    XTranslateCoordinates(dpy, findW, stopW, 
			  x, y, &x, &y, &childW);
    findW = stopW;
    if (childW &&
	XGetWindowAttributes(dpy, childW, &wa) &&
	wa.class != InputOutput)
	break;
    stopW = childW;
  }
  return findW;
}

void CreateRoot()
{
  root = XtCreateWidget("root", rootWindowWidgetClass, toplevel, NULL, 0);
  XtRealizeWidget(root);
}

void StartRootPtrGrab()   
{
  Window    rootR, childR, window;
  int       rootX, rootY, winX, winY;
  unsigned  int mask;
  hlPtr data;
  XColor color;
  int x, y, i;
  Pixel pixel;
  static XEvent event;
  char addstr[20];
  char format[30];

  XtGrabPointer
    (root, False,
     PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
     GrabModeAsync, GrabModeAsync, None, newcursor, CurrentTime);
  XQueryPointer(dpy, DefaultRootWindow(dpy), &rootR, &childR, 
		&rootX, &rootY, &winX, &winY, &mask);
  data = (hlPtr)XtMalloc(sizeof(hlStruct));
  data->x          = rootX;
  data->y          = rootY;
  data->gc         = selectGC;
  data->width      = srcWidth;
  data->height     = srcHeight;

restart:
  XNextEvent(dpy, &event);

  if (event.type == ButtonRelease)	
    {
      x = event.xmotion.x_root;
      y = event.xmotion.y_root;
      
      if (x<sec) x=sec;
      if (y<sec) y=sec;
      if (x+srcWidth > DisplayWidth(dpy,scr)-sec) x = DisplayWidth(dpy,scr) - srcWidth-sec;
      if (y+srcHeight> DisplayHeight(dpy,scr)-sec) y = DisplayHeight(dpy,scr)- srcHeight-sec;
      
      data->x = x; data->y = y;

      window = FindWindow(x,y);
      XGetWindowAttributes(dpy, window, &data->win_info);

      data->image = XGetImage (dpy,
			     RootWindow(dpy, scr),
			     x, y,
			     srcWidth, srcHeight,
			     AllPlanes, ZPixmap);

      pixel = XGetPixel(data->image, 0, 0);
      
      color.pixel = pixel;
      XQueryColor(dpy, data->win_info.colormap, &color);

      *output = '\0';

      for(i=0; i<strlen(format); i++)
	{
        if ((format[i]=='%') && (i+1<strlen(format)))
	  {
          i++;
          switch(format[i])
	    {
            case 'x': sprintf(addstr, "%d", data->x); break;
            case 'y': sprintf(addstr, "%d", data->y); break;
            case 'r': sprintf(addstr, "%d", color.red/256); break;
            case 'g': sprintf(addstr, "%d", color.green/256); break;
            case 'b': sprintf(addstr, "%d", color.blue/256); break;            
            case 'c': sprintf(addstr, "%d", (unsigned int)pixel); break;
            case 'm': sprintf(addstr, "%d", event.xbutton.button); break;
            case 'w': sprintf(addstr, "%x", (unsigned int)window); break;
	    }
	  } else sprintf(addstr,"%c", format[i]);
        strcat(output,addstr);
	}
      XtUngrabPointer(root,CurrentTime);
      return ;
    }
  goto restart;
}

char *GrabPixel(char *format)
{
  dpy = lsx_curwin->display;
  scr = DefaultScreen(dpy);
  toplevel = lsx_curwin->toplevel;

  InitCursors();
  SetupGC();
  CreateRoot();
  StartRootPtrGrab(format);
  return(output);
}




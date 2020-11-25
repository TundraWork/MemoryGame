/* DrawingA.c: The DrawingArea Widget Methods */

/* Copyright 1990, David Nedde
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without fee
 * is granted provided that the above copyright notice appears in all copies.
 * It is provided "as is" without express or implied warranty.
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Xaw3dxft/SimpleP.h>
#include <X11/Xmu/Xmu.h>
#include "drawingP.h"

#ifndef caddr_t
typedef char *caddr_t;
#endif

static void	Initialize();
static void     MyRealize();  /* So we can do our own visual */
static void	Destroy();
static void	Redisplay();
static void	input_draw();
static void	motion_draw();
static void	resize_draw();
static void	enter_draw();
static void	leave_draw();

static char defaultTranslations[] = "<BtnDown>: input() \n <BtnUp>: input() \n <KeyDown>: input() \n <KeyUp>: input() \n <Motion>: motion() \n <Configure>: resize() \n <Enter>: enter() \n <Leave>: leave()";
static XtActionsRec actionsList[] = {
  { "input",  (XtActionProc)input_draw },
  { "motion", (XtActionProc)motion_draw },
  { "resize", (XtActionProc)resize_draw },
  { "enter",  (XtActionProc)enter_draw },
  { "leave",  (XtActionProc)leave_draw },
};

/* Default instance record values */
static XtResource resources[] = {
  {XtNvisual, XtCVisual, XtRVisual, sizeof(caddr_t),
     XtOffset(DrawingAreaWidget, drawing_area.vis),
     XtRVisual, NULL },
  {XtNexposeCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset(DrawingAreaWidget, drawing_area.expose_callback), 
     XtRCallback, NULL },
  {XtNinputCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset(DrawingAreaWidget, drawing_area.input_callback), 
     XtRCallback, NULL },
  {XtNmotionCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset(DrawingAreaWidget, drawing_area.motion_callback), 
     XtRCallback, NULL },
  {XtNresizeCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset(DrawingAreaWidget, drawing_area.resize_callback), 
     XtRCallback, NULL },
  {XtNenterCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset(DrawingAreaWidget, drawing_area.enter_callback), 
     XtRCallback, NULL },
  {XtNleaveCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset(DrawingAreaWidget, drawing_area.leave_callback), 
     XtRCallback, NULL },
};


DrawingAreaClassRec drawingAreaClassRec = {
  /* CoreClassPart */
{
  (WidgetClass) &simpleClassRec,	/* superclass		  */	
    "DrawingArea",			/* class_name		  */
    sizeof(DrawingAreaRec),		/* size			  */
    NULL,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    MyRealize,				/* realize		  */
    actionsList,			/* actions		  */
    XtNumber(actionsList),		/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    TRUE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    NULL,				/* resize		  */
    Redisplay,				/* expose		  */
    NULL,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
    XtInheritDisplayAccelerator,	/* display_accelerator	  */
    NULL				/* extension		  */
  },  /* CoreClass fields initialization */
  {
    /* change_sensitive		*/	XtInheritChangeSensitive
  },  /* SimpleClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* DrawingAreaClass fields initialization */
};

  
WidgetClass drawingAreaWidgetClass = (WidgetClass)&drawingAreaClassRec;


static void Initialize( request, new)
DrawingAreaWidget request, new;
{
  if (request->core.width == 0)
    new->core.width = 100;
  if (request->core.height == 0)
    new->core.height = 100;
}


/*      Function Name: ConvertCursor
 *      Description: Converts a name to a new cursor.
 *      Arguments: w - the simple widget.
 *      Returns: none.
 */

static void
ConvertCursor(Widget w)
{
    SimpleWidget simple = (SimpleWidget) w;
    XrmValue from, to;
    Cursor cursor;
   
    if (simple->simple.cursor_name == NULL)
        return;

    from.addr = (XPointer) simple->simple.cursor_name;
    from.size = strlen((char *) from.addr) + 1;

    to.size = sizeof(Cursor);
    to.addr = (XPointer) &cursor;

    if (XtConvertAndStore(w, XtRString, &from, XtRColorCursor, &to)) {
        if ( cursor !=  None) 
            simple->simple.cursor = cursor;
    } 
    else {
        XtAppErrorMsg(XtWidgetToApplicationContext(w),
                      "convertFailed","ConvertCursor","XawError",
                      "Simple: ConvertCursor failed.",
                      (String *)NULL, (Cardinal *)NULL);
    }
}

static void MyRealize(w, valueMask, attributes)
    DrawingAreaWidget w;
    Mask *valueMask;
    XSetWindowAttributes *attributes;
{
    Pixmap border_pixmap = 0;

    if (!XtIsSensitive((Widget)w)) {
        /* change border to gray; have to remember the old one,
         * so XtDestroyWidget deletes the proper one */
        if (((SimpleWidget)w)->simple.insensitive_border == None)
            ((SimpleWidget)w)->simple.insensitive_border =
                XmuCreateStippledPixmap(XtScreen(w),
                                        w->core.border_pixel, 
                                        w->core.background_pixel,
                                        w->core.depth);
        border_pixmap = w->core.border_pixmap;
        attributes->border_pixmap =
          w->core.border_pixmap = ((SimpleWidget)w)->simple.insensitive_border;

        *valueMask |= CWBorderPixmap;
        *valueMask &= ~CWBorderPixel;
    }

    ConvertCursor((Widget)w);

    if ((attributes->cursor = ((SimpleWidget)w)->simple.cursor) != None)
        *valueMask |= CWCursor;

    XtCreateWindow((Widget)w, (unsigned int)InputOutput,
		   (Visual *)w->drawing_area.vis,
		   *valueMask, attributes );

    if (!XtIsSensitive((Widget)w))
        w->core.border_pixmap = border_pixmap;
}




static void Destroy( w)
DrawingAreaWidget w;
{
  XtRemoveAllCallbacks((Widget)w, XtNexposeCallback);
  XtRemoveAllCallbacks((Widget)w, XtNinputCallback);
  XtRemoveAllCallbacks((Widget)w, XtNmotionCallback);
  XtRemoveAllCallbacks((Widget)w, XtNresizeCallback);
  XtRemoveAllCallbacks((Widget)w, XtNenterCallback);
  XtRemoveAllCallbacks((Widget)w, XtNleaveCallback);
}


/* Invoke expose callbacks */
static void Redisplay(w, event, region)
DrawingAreaWidget w;
XEvent		 *event;
Region		  region;
{
  XawDrawingAreaCallbackStruct cb;

  cb.reason = XawCR_EXPOSE;
  cb.event  = event;
  cb.window = XtWindow(w);
  XtCallCallbacks((Widget)w, XtNexposeCallback, (char *)&cb);
}

/* Invoke resize callbacks */
static void resize_draw(w, event, args, n_args)
DrawingAreaWidget w;
XEvent		 *event;
char		 *args[];
int		  n_args;
{
  XawDrawingAreaCallbackStruct cb;

  cb.reason = XawCR_RESIZE;
  cb.event  = event;
  cb.window = XtWindow(w);
  XtCallCallbacks((Widget)w, XtNresizeCallback, (char *)&cb);
}

/* Invoke input callbacks */
static void input_draw(w, event, args, n_args)
DrawingAreaWidget w;
XEvent		 *event;
char		 *args[];
int		  n_args;
{
  XawDrawingAreaCallbackStruct cb;

  cb.reason = XawCR_INPUT;
  cb.event  = event;
  cb.window = XtWindow(w);
  XtCallCallbacks((Widget)w, XtNinputCallback, (char *)&cb);
}

/* Invoke motion callbacks */
static void motion_draw(w, event, args, n_args)
DrawingAreaWidget w;
XEvent		 *event;
char		 *args[];
int		  n_args;
{
  XawDrawingAreaCallbackStruct cb;

  cb.reason = XawCR_MOTION;
  cb.event  = event;
  cb.window = XtWindow(w);
  XtCallCallbacks((Widget)w, XtNmotionCallback, (char *)&cb);
}

/* Invoke enter callbacks */
static void enter_draw(w, event, args, n_args)
DrawingAreaWidget w;
XEvent		 *event;
char		 *args[];
int		  n_args;
{
  XawDrawingAreaCallbackStruct cb;

  cb.reason = XawCR_ENTER;
  cb.event  = event;
  cb.window = XtWindow(w);
  XtCallCallbacks((Widget)w, XtNenterCallback, (char *)&cb);
}

/* Invoke leave callbacks */
static void leave_draw(w, event, args, n_args)
DrawingAreaWidget w;
XEvent		 *event;
char		 *args[];
int		  n_args;
{
  XawDrawingAreaCallbackStruct cb;

  cb.reason = XawCR_LEAVE;
  cb.event  = event;
  cb.window = XtWindow(w);
  XtCallCallbacks((Widget)w, XtNleaveCallback, (char *)&cb);
}

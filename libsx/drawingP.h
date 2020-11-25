/* DrawingArea Private header file */

/* Copyright 1990, David Nedde
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without fee
 * is granted provided that the above copyright notice appears in all copies.
 * It is provided "as is" without express or implied warranty.
 */

#ifndef _XawDrawingAreaP_h
#define _XawDrawingAreaP_h

#include "drawing.h"
#ifdef X11_R3
#include <X11/SimpleP.h>
#else
#include <X11/Xaw3dxft/SimpleP.h>
#endif

/* The drawing area's contribution to the class record */
typedef struct _DrawingAreaClassPart {
  int ignore;
} DrawingAreaClassPart;

/* Drawing area's full class record */
typedef struct _DrawingAreaClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    DrawingAreaClassPart drawing_area;
} DrawingAreaClassRec;

extern DrawingAreaClassRec drawingAreaClassRec;

#ifndef XtCVisual
#define XtCVisual "Visual"
#endif
#ifndef XtRVisual
#define XtRVisual "Visual"
#endif

/* Resources added and status of drawing area widget */
typedef struct _XsDrawingAreaPart {
  /* Resources */
  Visual                *vis;
  XtCallbackList	expose_callback;
  XtCallbackList	input_callback;
  XtCallbackList	motion_callback;
  XtCallbackList	resize_callback;
  XtCallbackList	enter_callback;
  XtCallbackList	leave_callback;
} DrawingAreaPart;


/* Drawing area's instance record */
typedef struct _DrawingAreaRec {
    CorePart         core;
    SimplePart	     simple;
    DrawingAreaPart  drawing_area;
} DrawingAreaRec;

#endif /* _XawDrawingAreaP_h */

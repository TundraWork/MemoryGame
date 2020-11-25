/* 
 *  [ This file is blatantly borrowed from the pixmap distribution. ]
 *  [ It was written by the fellows below, and they disclaim all    ]
 *  [ warranties, expressed or implied, in this software.           ]
 *  [ As if anyone cares about that...                              ]
 *
 * Copyright 1991 Lionel Mallet
 * 
 * Author:  Davor Matic, MIT X Consortium
 */


#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <X11/Xaw3dxft/Dialog.h>
#include <X11/Xaw3dxft/Command.h>
#include <X11/Xaw3dxft/AsciiText.h>
    
#include "libsx.h"
#include "dialog.h"

#define min(x, y)                     (((x) < (y)) ? (x) : (y))
#define max(x, y)                     (((x) > (y)) ? (x) : (y))

#ifndef strdup  
extern char *strdup(const char *str);
#endif

extern int HILIGHT;
extern int BUTTONBG;
extern int INPUTBG;

extern void SetBgColor(Widget w, int color);
extern void SetFgColor(Widget w, int color);

static void libsx_set_text_focus(Widget w, XEvent *xev, String *parms,
                           Cardinal *num_parms);

static int selected;

static int button_flags[] = { Yes, Abort, No, Okay, Cancel, Retry};


static void SetSelected(Widget w, XtPointer client_data, XtPointer call_data)
{
  selected = (long)client_data;
}


/*
 * Can't make this static because we need to be able to access the
 * name over in display.c to properly set the resources we want.
 */
void SetOkay(Widget w, XEvent *xev, String *parms, Cardinal *numparams)
{
  SetSelected(w, (XtPointer)(Okay | Yes), NULL);
}



/*
 * This action table sets up an action so that the return key can
 * be used to pop-down a popup.  The translation is setup in libsx.c
 * We do it there instead of here because the resources need to be
 * setup at XtAppInitialize time (well strictly that's not entirely
 * true, but we'll pretend it is).
 */
static XtActionsRec popup_actions_table[] =
{
  { "set_text_focus", libsx_set_text_focus },
  { "set-okay", SetOkay }
};
static int added_popup_actions = 0;


Dialog CreateDialog(Widget top_widget, char *name, int options)
{
  int i;
  Dialog popup;
  Widget button;
  
  if ((popup = (Dialog) XtMalloc(sizeof(_Dialog))) == NULL)
    return NULL;


  if (added_popup_actions == 0)
   {
     extern XtAppContext lsx_app_con;
     
     added_popup_actions = 1;
     XtAppAddActions(lsx_app_con, popup_actions_table,
		     XtNumber(popup_actions_table));
   }

  popup->top_widget = top_widget;
  popup->shell_widget = XtCreatePopupShell(name, 
					   transientShellWidgetClass, 
					   top_widget, NULL, 0);
  popup->dialog_widget = XtCreateManagedWidget("dialog", 
					       dialogWidgetClass,
					       popup->shell_widget, 
					       NULL, 0);
  
  for (i = 0; i < XtNumber(button_flags); i++)
    if (options & button_flags[i]) {
      XawDialogAddButton(popup->dialog_widget, 
			 SX_Dialog[i], 
			 SetSelected, (XtPointer)(long int)button_flags[i]);
      button = XtNameToWidget(popup->dialog_widget, SX_Dialog[i]);
      if (BUTTONBG>=0) SetBgColor(button, BUTTONBG);
    }
  
  popup->options = options;
  return popup;
}


void FreeDialog(Dialog dialog)
{
  if (dialog == NULL)
    return;

  XtDestroyWidget(dialog->shell_widget);
  XtFree((char *)dialog);
}



void PositionPopup(Widget shell_widget)
{
  int n;
  Arg wargs[10];
  Position popup_x, popup_y, top_x, top_y;
  Dimension popup_width, popup_height, top_width, top_height, border_width;

  n = 0;
  XtSetArg(wargs[n], XtNx, &top_x); n++;
  XtSetArg(wargs[n], XtNy, &top_y); n++;
  XtSetArg(wargs[n], XtNwidth, &top_width); n++;
  XtSetArg(wargs[n], XtNheight, &top_height); n++;
  XtGetValues(XtParent(shell_widget), wargs, n);

  n = 0;
  XtSetArg(wargs[n], XtNwidth, &popup_width); n++;
  XtSetArg(wargs[n], XtNheight, &popup_height); n++;
  XtSetArg(wargs[n], XtNborderWidth, &border_width); n++;
  XtGetValues(shell_widget, wargs, n);

  popup_x = max(0, 
		min(top_x + ((Position)top_width - (Position)popup_width) / 2, 
		    (Position)DisplayWidth(XtDisplay(shell_widget), 
			       DefaultScreen(XtDisplay(shell_widget))) -
		    (Position)popup_width - 2 * (Position)border_width));
  popup_y = max(0, 
		min(top_y+((Position)top_height - (Position)popup_height) / 2,
		    (Position)DisplayHeight(XtDisplay(shell_widget), 
			       DefaultScreen(XtDisplay(shell_widget))) -
		    (Position)popup_height - 2 * (Position)border_width));
  n = 0;
  XtSetArg(wargs[n], XtNx, popup_x); n++;
  XtSetArg(wargs[n], XtNy, popup_y); n++;
  XtSetValues(shell_widget, wargs, n);
}

void libsx_set_text_focus(Widget w, XEvent *xev, String *parms,
                           Cardinal *num_parms)
{
   if(HILIGHT>=0) SetFgColor(w, HILIGHT);
   XSetInputFocus(XtDisplay(w),XtWindow(w),RevertToNone,CurrentTime);
}


int PopupDialog(XtAppContext app_con, Dialog popup, char *message,
		            char *suggestion, char **answer, XtGrabKind grab)
{
  int n, height = 35;
  Arg wargs[10];
  Widget value;
  static int already_done = FALSE;
  static XtTranslations trans;

  if (already_done == FALSE)
    {
     already_done = TRUE;
     trans = XtParseTranslationTable("#override\n"
            "   <ButtonPress>: set_text_focus()\n");
    }

  n = 0;
  XtSetArg(wargs[n], XtNlabel, message); n++;
  if (suggestion)
   {
     XtSetArg(wargs[n], XtNvalue, suggestion); n++;
   }
  XtSetValues(popup->dialog_widget, wargs, n);

  
  /*
   * Here we get ahold of the ascii text widget for the dialog box (if it
   * exists) and we set some useful resources in it.
   */
  value = XtNameToWidget(popup->dialog_widget, "value");

  n = 0;
  XtSetArg(wargs[n], XtNeditType,         "edit"); n++;
  XtSetArg(wargs[n], XtNresizable,        True);                n++; 
  XtSetArg(wargs[n], XtNheight,           height);              n++; 
  XtSetArg(wargs[n], XtNwidth,            350);                 n++; 
  XtSetArg(wargs[n], XtNresize,           XawtextResizeHeight); n++;
  XtSetArg(wargs[n], XtNscrollHorizontal, XawtextScrollWhenNeeded); n++;
  if (suggestion)
    {
    XtSetArg(wargs[n], XtNinsertPosition,   strlen(suggestion));n++;
    }
  XtSetArg(wargs[n], XtNtranslations, trans); n++;
  if (value)
    XtSetValues(value, wargs, n);


  XtRealizeWidget(popup->shell_widget);

  if(INPUTBG>=0) SetBgColor(value, INPUTBG);
  
  PositionPopup(popup->shell_widget);

  selected = Empty;

  XtPopup(popup->shell_widget, grab);


  while ((selected & popup->options) == Empty)
   {
     XEvent event;

     XtAppNextEvent(app_con, &event);
     XtDispatchEvent(&event);
   }
  
  PopdownDialog(popup, answer);
  
  return (selected & popup->options);
}




void PopdownDialog(Dialog popup, char **answer)
{
  char *tmp;
    if (answer)
     {
       tmp = XawDialogGetValueString(popup->dialog_widget);
       *answer = tmp ? strdup(tmp) : NULL;
     }
    
    XtPopdown(popup->shell_widget);
}

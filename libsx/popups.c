/*   This file contains routines that handle popping up dialog boxes.
 * They use the routines in Dialog.c to do most of the work.
 *
 *                     This code is under the GNU Copyleft.
 * 
 *  Dominic Giampaolo
 *  dbg@sgi.com
 */
#include <stdio.h>
#include <stdlib.h>
#include "xstuff.h"
#include "dialog.h"
#include "libsx.h"
#include "libsx_private.h"


static Widget w1 = None;

static int do_popup(char *blurb, int buttons);

extern XtAppContext  lsx_app_con;


/*
 * User input routines that take place through Dialog boxes (getting
 * a string and a simple yes/no answer).
 */
  
char *GetString(char *blurb, char *default_string)
{
  char *string = default_string;
  Dialog dialog = NULL;

  if (blurb == NULL || (lsx_curwin->toplevel==NULL && OpenDisplay(0,NULL)==0))
    return NULL;

  
  dialog = CreateDialog(lsx_curwin->toplevel, "InputString", 
                        Okay | Cancel);
  
  if (dialog == NULL)   /* then there's an error */
    return NULL;

  if (string == NULL)
    string = "";

  switch(PopupDialog(lsx_app_con, dialog, blurb, string, &string,
		     XtGrabExclusive))
   {
     case Yes:
     case Okay: /* don't have to do anything, string is already set */
                break;

     case Cancel:  string = NULL;
                   break;

     default: string = NULL;    /* shouldn't happen, but just in case */
              break;
   }  /* end of switch */

  FreeDialog(dialog);
  return string;
}

void GetTextCancel(Widget ww, void* data)
{
  char **string = (char **) data;
  *string = NULL;
  SetCurrentWindow(ww);
  CloseWindow();
  w1 = None;
}

void GetTextOkay(Widget ww, void* data)
{
  char *ptr = NULL;
  char **string = (char **) data;

  if (w1) ptr = GetStringEntry(w1);
  if (ptr!=NULL)
    {
      *string = (char *)malloc((strlen(ptr)+1)*sizeof(char));
      strcpy(*string, ptr);
    }
  SetCurrentWindow(ww);
  CloseWindow();
}

char *GetText(char *blurb, char *default_string, int width, int height)
{
Widget wid_gettext[6];
char *string;
int i;

   MakeWindow("GetText",SAME_DISPLAY, EXCLUSIVE_WINDOW);
   
   string = NULL;
   wid_gettext[0] = MakeLabel(blurb);
   if(height>0)
     wid_gettext[1] = MakeTextWidget(default_string,0,1, width, height);
   else
     wid_gettext[1] = MakeStringEntry(default_string,width, NULL, NULL);
   w1 = wid_gettext[1];
   SetWidgetPos(wid_gettext[1],PLACE_UNDER,wid_gettext[0],NO_CARE,NULL);
   wid_gettext[2] = MakeButton(SX_Dialog[POPUP_DIAL],GetTextOkay,&string);
   wid_gettext[3] = MakeButton(SX_Dialog[POPUP_DIAL+1],GetTextCancel,&string);
   for (i=2; i<=3 ; i++)
     SetWidgetPos(wid_gettext[i],PLACE_UNDER,wid_gettext[1],NO_CARE,NULL);
   SetWidgetPos(wid_gettext[3],PLACE_RIGHT,wid_gettext[2],NO_CARE,NULL);

   ShowDisplay();
   if(INPUTBG>=0)
      SetBgColor(wid_gettext[1],INPUTBG);
   if(BUTTONBG>=0) for (i=2; i<=3 ; i++)
      SetBgColor(wid_gettext[i],BUTTONBG);
   MainLoop();
   return string;
}

char *GetLongString(char *blurb, char *default_string, int width)
{
   return GetText(blurb, default_string, width, 0);
}


int GetYesNo(char *blurb)
{
  return do_popup(blurb, Yes|No);
}


int GetOkay(char *blurb)
{
  return do_popup(blurb, Okay);
}


int GetTriState(char *blurb)
{
  return do_popup(blurb, Yes|Abort|No);
}





static int do_popup(char *blurb, int buttons)
{
  Dialog dialog = NULL;
  int ret;

  if (blurb == NULL || (lsx_curwin->toplevel==NULL && OpenDisplay(0,NULL)==0))
    return FALSE;
  

  dialog = CreateDialog(lsx_curwin->toplevel,SX_Dialog[POPUP_DIAL+2],buttons);
  
  if (dialog == NULL)   /* then there's an error */
    return FALSE;

  switch(PopupDialog(lsx_app_con, dialog, blurb, NULL, NULL, XtGrabExclusive))
   {
     case Okay:
     case Yes: ret = TRUE;
               break;

     case No: ret = FALSE;
              break;

     case Abort:  ret = -1;
                  break;

     default: ret = FALSE;   /* unknown return from dialog, return an err */
              break;
   }  /* end of switch */

  FreeDialog(dialog);

  return ret;
}


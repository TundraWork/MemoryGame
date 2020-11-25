/*  This file contains the top level routines that manipulate whole
 * windows and what not.  It's the main initialization stuff, etc.
 *
 *
 *                     This code is under the GNU Copyleft.
 *
 *  Dominic Giampaolo
 *  dbg@sgi.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xstuff.h"
#include "libsx.h"
#include "libsx_private.h"

/*
 *  Libsx Dialogs
 */
char *SX_Dialog[NUM_DIAL];

/*
 * Predefined Args;
 */
static char *_PredefArgs[] = { NULL } ;
char **PredefArgs = _PredefArgs ;

/*
 * External prototypes.
 */
extern void PositionPopup(Widget w);                  /* from Dialog.c */
extern void libsx_set_focus(Widget w, XEvent *xev, String *parms,
                                 Cardinal *num_parms);/* from string_entry.c */
extern void libsx_set_text_focus(Widget w, XEvent *xev, String *parms,
                                 Cardinal *num_parms);
extern void libsx_done_with_text(Widget w, XEvent *xev, String *parms,
                                 Cardinal *num_parms);/* from string_entry.c */


/* internal protos */
static Bool  is_expose_event(Display *d, XEvent *xev, char *blorg);

/*
 * Private static variables.
 */
static WindowState  *lsx_windows  = NULL,
                     empty_window = { 0, 0, 0, NULL, NULL, NULL, NULL, 0,},
                    *orig_window  = NULL;

static int   window_opened=0;
static char *app_name="";
static Display *base_display=NULL;
char _FreqFilter[80] = "*";

/*
 * Global Variables for all of libsx (but not for user applications).
 *
 * We initialize lsx_curwin to point to the static empty_window so that
 * if other functions get called before OpenDisplay(), they won't core
 * dump, and will fail gracefully instead.
 */
volatile WindowState  *lsx_curwin = &empty_window;
XtAppContext  lsx_app_con;

Widget w_prev  = 0, win_prev = 0;
int color_prev = -1;

static String fallback_resources[] =
{
  "*font: 7x13",
  "*beNiceToColormap: False",
  "*Thumb*background: red",
  "*vertical*borderWidth: 0", 
  "*horizontal*borderWidth: 0",
  "*vertical*shadowWidth: 1",
  "*horizontal*shadowWidth: 1",
  "*valueBar*shadowWidth: 1", 
  "*scrollbar*shadowWidth: 1", 
  "*Text*shadowWidth: 1",
  "*SimpleMenu.shadowWidth: 1",
  "*SimpleMenu*SmeBSB*shadowWidth: 0",
  "*Dialog*label.resizable: True",
  "*Dialog*Text.resizable: True",
  "*Dialog.Text.translations: #override <Key>Return: set-okay()\\n\
                              <Key>Linefeed: set-okay()",
  NULL
};

/*
 *   Read dialogs as needed by environment variable LANG
 */
void ReadLocale(char *language)
{
  char locale_file[128];
  char line[128];
  char *ptr;
  FILE *fd;
  int i, j;

  if (language==NULL) language=getenv("LANG");
  if (language==NULL) language="en";

  sprintf(locale_file, SX_SHAREDIR"/dialogs.%c%c", 
       language[0], language[1]);
  fd = fopen(locale_file, "r");
  if (fd==NULL)
     strcpy(locale_file, SX_SHAREDIR"/dialogs.en"), 
  fd = fopen(locale_file, "r");
  if (fd==NULL)
    fprintf(stderr, "Cannot open dialogs %s !!", locale_file);
  else
    {
    i=0;
    while((ptr=fgets(line,120,fd))!=NULL)
      {
      j=strlen(line)-1;
      if (line[j]=='\n') line[j]='\0';
      if (i<NUM_DIAL && *line!='#') 
	{
	SX_Dialog[i] = realloc(SX_Dialog[i], (j+1)*sizeof(char));
        strcpy(SX_Dialog[i],line);
        i++;
	}
      }
    }
}

/*
 *
 * This is the function that gets everything started.
 *
 */
int _OpenDisplay(int argc, char **argv, Display *dpy, Arg *wargs, int arg_cnt)
{
  static char *dummy_argv[] = { "Untitled", NULL };
  char **new_argv;
  static int already_called = FALSE;
  int i;

  if (already_called)   /* must have been called a second time */
    return FALSE;
  already_called = TRUE;

  memset(SX_Dialog, 0, NUM_DIAL*sizeof(char*));
  ReadLocale(NULL);

  if (argv == NULL && *PredefArgs == NULL)
    {
      argv = dummy_argv;
      argc = 1;
    }
  
  /* Parse additional predefined args */
  if (*PredefArgs)
    {
    int j;

    i = 0;
    while (PredefArgs[i]!=NULL) ++i;
  
    new_argv = (char**) malloc((argc+i+1)*sizeof(char *));
  
    new_argv[0] = argv[0];
    i = 0;
    while (PredefArgs[i]!=NULL) 
      {
      new_argv[i+1] = PredefArgs[i];
      ++i;
      }

    for (j=1; j<=argc; j++) new_argv[i+j] = argv[j];
    argc += i;
    }
  else
    new_argv = argv;        


  /*
   * First create the window state.
   */
  lsx_curwin = (WindowState *)calloc(sizeof(WindowState), 1);
  if (lsx_curwin == NULL)
    return FALSE;

  /* open the display stuff */
  if (dpy == NULL)
    {
    lsx_curwin->toplevel = XtAppInitialize(&lsx_app_con, argv[0], NULL, 0,
					   &argc, new_argv, fallback_resources,
					   wargs, arg_cnt);
    if (*PredefArgs) 
       for (i=1; i<=argc; i++) argv[i] = new_argv[i];
    }
  else
    lsx_curwin->toplevel = XtAppCreateShell (argv[0], argv[0],
					     applicationShellWidgetClass,
					     dpy, wargs, arg_cnt);
  

  if (lsx_curwin->toplevel == NULL)
   {
     free((void *)lsx_curwin);
     return FALSE;
   }

  app_name  = argv[0];   /* save this for later */

  lsx_curwin->form_widget = XtCreateManagedWidget("form", formWidgetClass,
						  lsx_curwin->toplevel,NULL,0);

  if (lsx_curwin->form_widget == NULL)
   {
     XtDestroyWidget(lsx_curwin->toplevel);
     free((void *)lsx_curwin);
     lsx_curwin = &empty_window;
     return FALSE;
   }
  lsx_curwin->toplevel_form = lsx_curwin->form_widget;
  

  lsx_curwin->next = lsx_windows;    /* link it in to the list */
  lsx_windows = (WindowState *)lsx_curwin;

  /* save these values for later */
  lsx_curwin->display = (Display *)XtDisplay(lsx_curwin->toplevel); 
  lsx_curwin->screen  = DefaultScreen(lsx_curwin->display);

  orig_window   = (WindowState *)lsx_curwin;
  base_display  = lsx_curwin->display;
  window_opened = 1;

  return argc;
} /* end of _OpenDisplay() */



int OpenDisplay(int argc, char **argv)
{
  return _OpenDisplay(argc, argv, NULL, NULL, 0);
}

#ifdef OPENGL_SUPPORT

#ifndef strdup
extern char *strdup(const char *str);
#endif

int OpenGLDisplay(int argc, char **argv, int *attributes)
{
  int xargc, i, retval, count, tmp_depth;
  char **new_argv, **xargv;
  Display *dpy;
  XVisualInfo *xvi;
  GLXContext cx;
  XVisualInfo   vinfo;
  XVisualInfo	*vinfo_list;	/* returned list of visuals */
  Colormap	colormap;	/* created colormap */
  Widget        top;

  /* Parse additional predefined args */
  if (PredefArgs)
    {
    int i = 0, j;
    while (PredefArgs[i]!=NULL) ++i;
  
    new_argv = (char**) malloc((argc+i+1)*sizeof(char *));
  
    new_argv[0] = argv[0];
    i = 0;
    while (PredefArgs[i]!=NULL) 
      {
      new_argv[i+1] = PredefArgs[i];
      ++i;
      }

    for (j=1; j<=argc; j++) new_argv[i+j] = argv[j];
    argc += i;
    argv = new_argv;
    }

  /*
   * First we copy the command line arguments
   */
  xargc = argc;
  xargv = (char **)malloc(argc * sizeof (char *));
  for(i=0; i < xargc; i++)
    xargv[i] = strdup(argv[i]);

  
  /*
   * The following creates a _dummy_ toplevel widget so we can
   * retrieve the appropriate visual resource.
   */
  top = XtAppInitialize(&lsx_app_con, xargv[0], NULL, 0, &xargc, xargv,
			(String *)NULL, NULL, 0);
  if (top == NULL)
    return 0;
  
  dpy = XtDisplay(top);

  /*
   * Check if the server supports GLX.  If not, crap out.
   */
  if (glXQueryExtension(dpy, NULL, NULL) == GL_FALSE)
   {
     XtDestroyWidget(top);
     return FALSE;
   }


  xvi = glXChooseVisual(dpy, DefaultScreen(dpy), attributes);
  if (xvi == NULL)
   {
     XtDestroyWidget(top);
     return 0;
   }

  cx  = glXCreateContext(dpy, xvi, 0, GL_TRUE);
  if (cx == NULL)
   {
     XtDestroyWidget(top);
     return 0;
   }
  

  /*
   * Now we create an appropriate colormap.  We could
   * use a default colormap based on the class of the
   * visual; we could examine some property on the
   * rootwindow to find the right colormap; we could
   * do all sorts of things...
   */
  colormap = XCreateColormap (dpy,
			      RootWindowOfScreen(XtScreen (top)),
			      xvi->visual,
			      AllocNone);

  /*
   * Now find some information about the visual.
   */
  vinfo.visualid = XVisualIDFromVisual(xvi->visual);
  vinfo_list = XGetVisualInfo(dpy, VisualIDMask, &vinfo, &count);
  if (vinfo_list && count > 0)
   {
     tmp_depth = vinfo_list[0].depth;

     XFree((XPointer) vinfo_list);
   }
  
  XtDestroyWidget(top);
  
  /*
   * Free up the copied version of the command line arguments
   */
  for(i=0; i < xargc; i++)
    if (xargv[i])
      free(xargv[i]);
  free(xargv);


  retval = _OpenDisplay(argc, argv, dpy, NULL, 0);

  if (retval > 0)
   {
     lsx_curwin->xvi             = xvi;
     lsx_curwin->gl_context      = cx;
     lsx_curwin->draw_area_cmap  = colormap;
     lsx_curwin->draw_area_depth = tmp_depth;
   }

  return retval;
}

#endif   /* OPENGL_SUPPORT */

void CloseProcedure(Widget w)
{
volatile WindowState *worig;

  SetCurrentWindow(ORIGINAL_WINDOW);
  worig = lsx_curwin;
  SetCurrentWindow(w);
  if (lsx_curwin->window == worig->window && 
      lsx_curwin->display == worig->display) exit(0);
  CloseWindow();
}

static XtActionsRec close_action[] = {
  /* action_name    routine */
  { "Quit",  (XtActionProc)CloseProcedure },    /* quit application */
};

void ShowDisplay(void)
{
  XEvent xev;
  Display *display;
  static Atom  wm_delete_window;
  XWMHints      *hints;

  if (lsx_curwin->toplevel == NULL || lsx_curwin->window_shown == TRUE)
    return;

  XtRealizeWidget(lsx_curwin->toplevel);
  display = XtDisplay(lsx_curwin->toplevel);

  XtAppAddActions(lsx_app_con, close_action, XtNumber(close_action));
  XtOverrideTranslations(lsx_curwin->toplevel,
              XtParseTranslationTable("<Message>WM_PROTOCOLS:Quit()") );
  wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
  (void) XSetWMProtocols(display, XtWindow(lsx_curwin->toplevel), 
                             &wm_delete_window, 1);
  hints = XGetWMHints(display, XtWindow(lsx_curwin->toplevel));
  hints->flags=AllHints;
  hints->input=True;
  XSetWMHints(display, XtWindow(lsx_curwin->toplevel), hints);

  if (XtIsTransientShell(lsx_curwin->toplevel))  /* do popups differently */
   {
     PositionPopup(lsx_curwin->toplevel);
     XtPopup(lsx_curwin->toplevel, XtGrabExclusive);
     
     lsx_curwin->window  = (Window   )XtWindow(lsx_curwin->toplevel);
     lsx_curwin->window_shown = TRUE;

     return;
   }

  /*
   * wait until the window is _really_ on screen
   */
  while(!XtIsRealized(lsx_curwin->toplevel))
    ;

  /*
   * Now make sure it is really on the screen.
   */
  XPeekIfEvent(XtDisplay(lsx_curwin->toplevel), &xev, is_expose_event, NULL);


  SetDrawArea(lsx_curwin->last_draw_widget);

  lsx_curwin->window  = (Window   )XtWindow(lsx_curwin->toplevel);
  lsx_curwin->window_shown = TRUE;
}   /* end of ShowDisplay() */



static Bool is_expose_event(Display *d, XEvent *xev, char *blorg)
{
  if (xev->type == Expose)
    return TRUE;
  else
    return FALSE;
}



static int libsx_exit_main_loop=0;

void ExitMainLoop(void)
{
  libsx_exit_main_loop = 1;
}


void MainLoop(void)
{
  int transient;
  WindowState *curwin;
  
  if (lsx_curwin->toplevel == NULL)
    return;

  /* in case the user forgot to map the display, do it for them */
  if (lsx_curwin->window_shown == FALSE) 
   {
     ShowDisplay();
     GetStandardColors();
   }

  curwin    = (WindowState *)lsx_curwin;
  transient = XtIsTransientShell(lsx_curwin->toplevel);
  while (lsx_curwin != &empty_window && libsx_exit_main_loop == 0)
   {
     XEvent event;
     
     XtAppNextEvent(lsx_app_con, &event);
     XtDispatchEvent(&event);

     if (curwin != lsx_curwin) /* hmmm, something changed... */
      {
	if (transient)         /* just break out for transient windows */
	  break;

	curwin = (WindowState *)lsx_curwin;
      }
   }

  if (libsx_exit_main_loop)     /* if this was set, reset it back */
    libsx_exit_main_loop = 0;

  return;
}



void
CheckForEvent(void)
{
  XEvent event;

  if (XtAppPending(lsx_app_con) == 0)
    return;

  XtAppNextEvent(lsx_app_con, &event);
  XtDispatchEvent(&event);
}


void SyncDisplay(void)
{
  if (lsx_curwin->display)
    XSync(lsx_curwin->display, FALSE);
}





void AddTimeOut(unsigned long interval, void (*func)(), void *data)
{
  if (lsx_curwin->toplevel && func)
    XtAppAddTimeOut(lsx_app_con, interval, (XtTimerCallbackProc)func, data);
}


void  AddReadCallback(int fd,  IOCallback func, void *data)
{
  XtInputMask mask = XtInputReadMask;
  
  XtAppAddInput(lsx_app_con, fd, (XtPointer)mask,
		(XtInputCallbackProc)func, data);
}


void  AddWriteCallback(int fd, IOCallback func, void *data)
{
  XtInputMask mask = XtInputWriteMask;

  XtAppAddInput(lsx_app_con, fd, (XtPointer)mask,
		(XtInputCallbackProc)func, data);
}



Widget MakeWindow(char *window_name, char *display_name, int exclusive)
{
  WindowState *win=NULL;
  Display *d=NULL;
  Arg wargs[20];
  int n=0;
  Visual *vis;
  Colormap cmap;
  char *argv[5];
  int   argc;

  if (window_opened == 0)   /* must call OpenDisplay() for first window */
    return NULL;

  win = (WindowState *)calloc(sizeof(WindowState), 1);
  if (win == NULL)
    return NULL;
  

  /*
   * Setup a phony argv/argc to appease XtOpenDisplay().
   */
  if (window_name)
    argv[0] = window_name;
  else
    argv[0] = app_name;
  argv[1] = NULL;
  argc = 1;

  if (display_name != NULL)
    d = XtOpenDisplay(lsx_app_con, display_name, app_name, app_name, NULL, 0,
		      &argc, argv);
  else
    d = base_display;
  if (d == NULL)
   {
     free(win);
     return NULL;
   }

  win->display  = d;
  

  cmap = DefaultColormap(d, DefaultScreen(d));
  vis  = DefaultVisual(d, DefaultScreen(d));
  

  n=0;
  XtSetArg(wargs[n], XtNtitle,    window_name);   n++;
  XtSetArg(wargs[n], XtNiconName, window_name);   n++;
  XtSetArg(wargs[n], XtNcolormap, cmap);          n++; 
  XtSetArg(wargs[n], XtNvisual,   vis);           n++; 
  
  if (HILIGHT>=0 && w_prev != 0 && lsx_curwin->toplevel == win_prev )
   {
    SetFgColor(w_prev, color_prev);
    w_prev=0;
   }

  if (exclusive == FALSE)
   {
     win->toplevel = XtAppCreateShell(argv[0], app_name,
				      topLevelShellWidgetClass, d, wargs, n);
   }
  else
   {
     win->toplevel = XtCreatePopupShell(argv[0], transientShellWidgetClass,
					lsx_curwin->toplevel, NULL, 0);
   }

  if (win->toplevel == NULL)
   {
     if (d != base_display)
       XtCloseDisplay(d);
     free(win);
     return NULL;
   }


  win->form_widget = XtCreateManagedWidget("form", formWidgetClass,
					   win->toplevel, NULL, 0);
  if (win->form_widget == NULL)
   {
     XtDestroyWidget(win->toplevel);
     if (d != base_display)
       XtCloseDisplay(d);
     free(win);
     return NULL;
   }
  win->toplevel_form = win->form_widget;
  

  win->screen = DefaultScreen(win->display);

  /*
   * Now link in the new window into the window list and make it
   * the current window.
   */
  win->next   = lsx_windows;
  lsx_windows = win;
  lsx_curwin  = win;

  return win->toplevel;    /* return a handle to the user */
}

Widget GetTopWidget(Widget w)
{
  return XtParent(XtParent(w));
}

Widget MakeForm(Widget parent)
{
  int n;
  Arg wargs[5];
  Widget form;

  if (lsx_curwin->toplevel == NULL)
    return NULL;
  
  if (parent == TOP_LEVEL_FORM)
    parent = lsx_curwin->toplevel_form;
  else if (strcmp(XtName(parent), "form") != 0) /* parent not a form widget */
    return NULL;


  n=0;
  XtSetArg(wargs[n], XtNwidth,     100);   n++;
  XtSetArg(wargs[n], XtNheight,    100);   n++;
  XtSetArg(wargs[n], XtNresizable, 1);     n++; 

  form = XtCreateManagedWidget("form", formWidgetClass,
			       parent, wargs, n);
  if (form == NULL)
    return NULL;

  lsx_curwin->form_widget = form;

  return form;
}


void SetForm(Widget form)
{
  if (lsx_curwin->toplevel == NULL)
    return;
  
  if (form == TOP_LEVEL_FORM)
    lsx_curwin->form_widget = lsx_curwin->toplevel_form;
  else
    lsx_curwin->form_widget = form;
}


Widget GetForm(void)
{
  if (lsx_curwin->toplevel == NULL)
    return NULL;

  return lsx_curwin->form_widget;
}


void SetCurrentWindow(Widget w)
{
  WindowState *win;

  if (w == ORIGINAL_WINDOW)
   {
     if (orig_window)
       lsx_curwin = orig_window;
     else if (lsx_windows)            /* hmm, don't have orig_window */
       lsx_curwin = lsx_windows;
     else
       lsx_curwin = &empty_window;    /* hmm, don't have anything left */

     SetDrawArea(lsx_curwin->last_draw_widget);
     return;
   }

  for(win=lsx_windows; win; win=win->next)
    if (win->toplevel == w && win->display == XtDisplay(w))
      break;

  if (win == NULL)
    return;

  lsx_curwin = win;    
  SetDrawArea(lsx_curwin->last_draw_widget);
}


void CloseWindow(void)
{
  WindowState *tmp;
  
  if (lsx_curwin->toplevel == NULL)
    return;

  XtDestroyWidget(lsx_curwin->toplevel);

  if (lsx_curwin->display != base_display)
   {
     FreeFont(lsx_curwin->font);
     XtCloseDisplay(lsx_curwin->display);
   }

  
  /*
   * if the window to delete is not at the head of the list, find
   * it in the list of available windows and delete it.
   */
  if (lsx_curwin == lsx_windows)
    lsx_windows = lsx_curwin->next;
  else
   {
     for(tmp=lsx_windows; tmp && tmp->next != lsx_curwin; tmp=tmp->next)
       /* nothing */;

     if (tmp == NULL)  /* didn't find it. must be bogus. just return */
       return;
       
     tmp->next = lsx_curwin->next;
   }
  
  if (lsx_curwin == orig_window)
    orig_window = NULL;
  
  free((void *)lsx_curwin);
  
  if (lsx_windows == NULL)
    lsx_windows = &empty_window;

  lsx_curwin = (volatile WindowState *)lsx_windows;
}



void Beep(void)
{
  XBell(lsx_curwin->display, 100);
}

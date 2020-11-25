/* This file contains all routines for creating and managing a file
 * requestor.  The programmer's only interface to the file requestor
 * is the function GetFile().  
 *
 * Originally written by Allen Martin (amartin@cs.wpi.edu).
 *
 * Significant modifications by me (Dominic Giampaolo, dbg@sgi.com)
 * to clean up some bugs, do double-clicks correctly, and make
 * relative path browsing work better.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <limits.h>
#include <sys/times.h>
#include <sys/stat.h>

#include "libsx.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef CLK_TCK
#define  CLK_TCK  sysconf(_SC_CLK_TCK)    /* seems to work right */
#endif  /* CLK_TCK */

extern char _FreqFilter[84];

int view_dir = 0;
int view_pt = 0;

/*
 * Some ugly hacks to make this work better under ultrix.
 */
#ifdef ultrix

/*
 * Can you say "Let's be pinheaded and follow the letter of the spec without
 * regard for what might make sense"?  I knew you could.  And so can the
 * boys and girls at DEC.
 *
 * Which is why they don't provide strdup() in their libc.  Gay or what?
 */
char *strdup(const char *str)
{
  char *new;

  new = malloc(strlen(str)+1);
  if (new)
    strcpy(new, str);

  return new;
}

#endif /* ultrix */


/*
 * No one seems to have a prototype for strdup().  What a pain
 * in the butt.  Why on earth isn't strdup() in the POSIX standard
 * but something completely useless like mbstowcs() is?
 */
#ifndef strdup  
extern char *strdup(const char *str);
#endif

/*
 * Here's where the real code begins.
 */

typedef struct {
  Widget freq_window;
  Widget file_path;
  Widget file_filter;
  Widget file_name;
  Widget file_list;

  char **dirlist;            /* used by the list widget */
  char fpath[MAXPATHLEN];
  char fname[MAXPATHLEN];

  FreqCB list_CB;
  void  *data;
  
  clock_t last_click;        /* time of last click */

} FReqData;

static void load_cancel(Widget w, FReqData *fdata);
static void load_ok(Widget w, FReqData *fdata);
static void load_list(Widget w, char *string, int index, FReqData *fdata);
static void load_dir(Widget w, char *string, FReqData *fdata);
static void load_filter(Widget w, char *string, FReqData *fdata);
static void load_name(Widget w, char *string, FReqData *fdata);
static void home_dir(Widget w, FReqData *fdata);
static void current_dir(Widget w, FReqData *fdata);
static void root_dir(Widget w, FReqData *fdata);
static void refresh_dir(Widget w, FReqData *fdata);
static void viewornot_dir(Widget w, FReqData *fdata);
static void viewornot_pt(Widget w, FReqData *fdata);
static int mystrcmp(const void *a, const void *b);

char **get_dir_list(char *pname, int *num);
void free_dirlist(char **table);

void SetFreqFilter(char *filter)
{
   if (filter) 
      strncpy(_FreqFilter, filter, 80);
   else
      strcpy(_FreqFilter, "*");
}

/*
 * GetFile() - This is the entry point to the file requestor.  A single
 *             argument is passed - the path name for the initial list.
 *             If this path name is passed as NULL, the current directory
 *             is used instead.  The function returns a character string
 *             that is the name of the selected file, path included.  If
 *             an error occurs, or the user selects CANCEL, NULL is returned.
 */
char *GetFile
(char *_label, char *_path, FreqCB _func, void *_data)
{
  FReqData fdata;
  Widget w[19];
  int num_dir, i;
  char path[MAXPATHLEN];
  char filename[MAXPATHLEN]="";
  char *ptr;
  struct stat buf;
  int fields[] = {2,4,5,8};
  int separ[] = {13,14,17};

  if(!_path || !*_path || strcmp(_path, ".") == 0 || strcmp(_path, "./") == 0)
    getcwd(path, MAXPATHLEN);
  else
    strcpy(path, _path);
  
  stat(path, &buf);
  if(S_ISDIR(buf.st_mode))
    {
    if(path[strlen(path)-1] != '/')
      strcat(path, "/");
    }
  else
    {
    ptr = path+strlen(path)-1;
    while (ptr > path && *ptr != '/') --ptr;
    if (*ptr=='/')
      {
      ++ptr;
      strcpy(filename,ptr);
      *ptr='\0';
      }
    }
  stat(path, &buf);
  if(!S_ISDIR(buf.st_mode))
    getcwd(path, MAXPATHLEN);

  if(!(fdata.dirlist = get_dir_list(path, &num_dir)))
    return(NULL);
  
  qsort(fdata.dirlist, num_dir, sizeof(char *), mystrcmp);

  fdata.freq_window = MakeWindow("FileRequestor", SAME_DISPLAY, 
				 EXCLUSIVE_WINDOW);

  w[0]  = MakeLabel(_label);
  w[1]  = MakeLabel(SX_Dialog[FREQ_DIAL]);
  w[2]  = MakeStringEntry(path, 406, (void *)load_dir,    &fdata);
  w[3]  = MakeLabel(SX_Dialog[FREQ_DIAL+1]);
  w[4]  = MakeStringEntry(_FreqFilter, 406, (void *)load_filter,   &fdata);
  w[5]  = MakeScrollList(fdata.dirlist, 468, 300, (void *)load_list, &fdata);
  w[6]  = MakeLabel(SX_Dialog[FREQ_DIAL+2]);
  w[7]  = MakeLabel(SX_Dialog[FREQ_DIAL+3]);
  w[8]  = MakeStringEntry(filename, 406, (void *)load_name, &fdata);
  w[9]  = MakeLabel(SX_Dialog[FREQ_DIAL+4]);
  w[10] = MakeButton(SX_Dialog[FREQ_DIAL+5], (void *)home_dir, &fdata);
  w[11] = MakeButton(SX_Dialog[FREQ_DIAL+6], (void *)current_dir, &fdata);
  w[12] = MakeButton("/", (void *)root_dir, &fdata);
  w[13] = MakeButton(SX_Dialog[FREQ_DIAL+7], (void *)refresh_dir, &fdata);
  w[14] = MakeLabel(SX_Dialog[FREQ_DIAL+8]);
  w[15] = MakeToggle("/", view_dir, NULL, (void *)viewornot_dir, &fdata);
  w[16] = MakeToggle(".", view_pt, NULL, (void *)viewornot_pt, &fdata);
  w[17] = MakeButton(SX_Dialog[FREQ_DIAL+9], (void *)load_cancel, &fdata);
  w[18] = MakeButton(SX_Dialog[FREQ_DIAL+10], (void *)load_ok, &fdata);
  
  for(i=1; i<=8; i++) 
     SetWidgetPos(w[i], PLACE_UNDER, w[i-1-(i==2)-(i==4)-(i==8)], 
                        NO_CARE, NULL);
  SetWidgetPos(w[2], PLACE_RIGHT, w[1], NO_CARE,     NULL);
  SetWidgetPos(w[4], PLACE_RIGHT, w[3], NO_CARE,     NULL);
  SetWidgetPos(w[8], PLACE_RIGHT, w[7], NO_CARE,     NULL);
  for(i=9; i<=18; i++) 
    {
    SetWidgetPos(w[i], PLACE_UNDER, w[8], NO_CARE,     NULL);
    if(i>9)
      SetWidgetPos(w[i], PLACE_RIGHT, w[i-1], NO_CARE, NULL);
    }

  for(i=0; i<=2; i++) 
     XtVaSetValues(w[separ[i]], "horizDistance", 31+(i==2), NULL);

  if (BUTTONBG >= 0) 
    for (i=10; i<=18; i++) if (i!=14) SetBgColor(w[i], BUTTONBG);
  if (INPUTBG >= 0)
    for (i=0; i<=3; i++) SetBgColor(w[fields[i]],INPUTBG);

  /* save the file name & file list widgets, so we can use them later */
  fdata.file_path = w[2];
  fdata.file_filter = w[4];
  fdata.file_list = w[5];
  fdata.file_name = w[8];
  fdata.fname[0] = '\0';

  fdata.last_click = 0;
  fdata.list_CB = _func;
  fdata.data = _data;

  /* set up the file path */
  strcpy(fdata.fpath, path);
  
  ShowDisplay();
  MainLoop();
  
  /* free the directory list */
  if (fdata.dirlist)
    free_dirlist(fdata.dirlist);
  
  SetCurrentWindow(ORIGINAL_WINDOW);

  if(fdata.fname[0] == '\0')
    return(NULL);
  else
    return(strdup(fdata.fname));
}

/*
 * load_cancel() - Callback routine for CANCEL button
 */
static void load_cancel(Widget w, FReqData *fdata)
{
  SetCurrentWindow(fdata->freq_window);
  CloseWindow();
  strcpy(fdata->fname, "");
}

/*
 * load_ok() - Callback routine for OK button
 */
static void load_ok(Widget w, FReqData *fdata)
{
  char *fpath, *fname;
  char fullname[MAXPATHLEN];
  
  fpath = GetStringEntry(fdata->file_path);
  fname = GetStringEntry(fdata->file_name);
  strncpy(_FreqFilter, GetStringEntry(fdata->file_filter), 80);

  sprintf(fullname, "%s%s", fpath, fname);

  /* right here we should check the validity of the file name */
  /* and abort if invalid */

  strcpy(fdata->fname, fullname);

  SetCurrentWindow(fdata->freq_window);
  CloseWindow();
}

/*
 * load_list() - Callback routine for scrollable list widget
 */
static void load_list(Widget w, char *string, int index, FReqData *fdata)
{
  char newpath[MAXPATHLEN], *cptr, *fpath, fullname[MAXPATHLEN];
  char **old_dirlist=NULL;
  static char oldfile[MAXPATHLEN] = { '\0', };
  clock_t cur_click;
  struct tms junk_tms;  /* not used, but passed to times() */
  int num_dir;
  float tdiff;    /* difference in time between two clicks as % of a second */
  

  /*
   * First we check for a double click.
   *
   * If the time between the last click and this click is greater than
   * 0.5 seconds or the last filename and the current file name
   * are different, then it's not a double click, so we just return.
   */
  cur_click = times(&junk_tms);
  tdiff = ((float)(cur_click - fdata->last_click) / CLK_TCK);

  if(tdiff > 0.50 || strcmp(oldfile, string) != 0)
   {
     fdata->last_click = cur_click;
     strcpy(oldfile, string);
     SetStringEntry(fdata->file_name, string);
     return;
   }
  
  /* check if a directory was selected */
  if(string[strlen(string)-1] != '/')   /* a regular file double click */
   {
     fpath = GetStringEntry(fdata->file_path);
     
     sprintf(fullname, "%s%s", fpath, string);
     
     /* right here we should check the validity of the file name */
     /* and abort if invalid */
     
     strcpy(fdata->fname, fullname);
     
     if (fdata->list_CB)
       fdata->list_CB(fdata->file_list, fdata->fpath,
                      fdata->fname, fdata->data);
     else
       {
       SetCurrentWindow(fdata->freq_window);
       CloseWindow();
       }
     return;
   }

  /*
   * Else, we've got a directory name and should deal with it
   * as approrpriate.
   */

  /* check for special cases "./" and "../" */
  if(strcmp(string, "./") == 0)
   {
     if (fdata->fpath)
       strcpy(newpath, fdata->fpath);
     else
       strcpy(newpath, "./");
   }
  else if(strcmp(string, "../") == 0)
   {
     strcpy(newpath, fdata->fpath);
     
     if (strcmp(newpath, "./") == 0)
       strcpy(newpath, string);
     else 
      {
	/*
	 * chop off the last path component and look at what it 
	 * is to determine what to do with the `..' we just got.
	 */
	cptr = strrchr(newpath, '/');
	if (cptr)
	  *cptr = '\0';
	cptr = strrchr(newpath, '/');
	if (cptr)
	  *cptr = '\0';

	if (  (cptr != NULL && strcmp(cptr+1,  "..") == 0)
	    ||(cptr == NULL && strcmp(newpath, "..") == 0))
	 {
	   if (cptr)
	     *cptr = '/';

	   strcat(newpath, "/");      /* put back the / we took out */
	   strcat(newpath, "../");    /* and append the new ../ */
	 }
	else
	 {
	   if(cptr == NULL && strcmp(fdata->fpath, "/") == 0)
	     strcpy(newpath, "/");
	   else if (cptr == NULL)
	     strcpy(newpath, "./");

	   if (newpath[strlen(newpath)-1] != '/')
	     strcat(newpath, "/");
	 }
      }
   }
  else /* not a `./' or `../', so it's just a regular old directory name */
   {
     if (fdata->fpath[strlen(fdata->fpath)-1] == '/')
       sprintf(newpath, "%s%s", fdata->fpath, string);
     else
       sprintf(newpath, "%s/%s", fdata->fpath, string);
   }

  old_dirlist = fdata->dirlist;
  if(!(fdata->dirlist = get_dir_list(newpath, &num_dir)))
    /* should beep the display or something here */
    return;
  
  qsort(fdata->dirlist, num_dir, sizeof(char *), mystrcmp);
  
  strcpy(fdata->fpath, newpath);
  SetStringEntry(fdata->file_path, fdata->fpath);
  SetStringEntry(fdata->file_name, "");
  strcpy(fdata->fpath, newpath);
  ChangeScrollList(fdata->file_list, fdata->dirlist);
  
  /* free the directory list */
  if (old_dirlist)
    free_dirlist(old_dirlist);
  
  
  fdata->last_click = 0;  /* re-init double-click time */
  
  return;
}

/*
 * load_dir() - Callback routine for pathname string entry widget
 */
static void load_dir(Widget w, char *string, FReqData *fdata)
{
  char **old_dirlist, temp[MAXPATHLEN];
  int num_dir;

  strncpy(_FreqFilter, GetStringEntry(fdata->file_filter), 80);

  /* make sure the name has a '/' at the end */
  strcpy(temp, string);
  if(temp[strlen(temp)-1] != '/')
    strcat(temp, "/");
  
  old_dirlist = fdata->dirlist;
  
  if(!(fdata->dirlist = get_dir_list(temp, &num_dir)))
    {
      /* bad path - reset the file path and return */
      SetStringEntry(fdata->file_path, fdata->fpath);
      return;
    }
  
  qsort(fdata->dirlist, num_dir, sizeof(char *), mystrcmp);

  strcpy(fdata->fpath, temp);
  SetStringEntry(fdata->file_path, temp);
  ChangeScrollList(fdata->file_list, fdata->dirlist);

  /* free the directory list */
  if (old_dirlist)
    free_dirlist(old_dirlist);
}

static void load_filter(Widget w, char *string, FReqData *fdata)
{
  load_dir(w, (char *)GetStringEntry(fdata->file_path), (FReqData *)fdata);
}

static void refresh_dir(Widget w, FReqData *fdata)
{
  load_dir(w, (char *)GetStringEntry(fdata->file_path), (FReqData *)fdata);
}

static void viewornot_dir(Widget w, FReqData *fdata)
{
  view_dir = 1 - view_dir;
  load_dir(w, (char *)GetStringEntry(fdata->file_path), (FReqData *)fdata);
}

static void viewornot_pt(Widget w, FReqData *fdata)
{
  view_pt = 1 - view_pt;
  load_dir(w, (char *)GetStringEntry(fdata->file_path), (FReqData *)fdata);
}

static void home_dir(Widget w, FReqData *fdata)
{
  load_dir(w, getenv("HOME"), (FReqData *)fdata);
}

static void current_dir(Widget w, FReqData *fdata)
{
char *ptr;

  load_dir(w, ptr=getcwd(NULL,1024), (FReqData *)fdata);
  free(ptr);
}

static void root_dir(Widget w, FReqData *fdata)
{
  load_dir(w, "/", (FReqData *)fdata);
}


/*
 * load_name() - Callback routine for file name string entry widget
 */
static void load_name(Widget w, char *string, FReqData *fdata)
{
  char *fpath, fullname[MAXPATHLEN];
  
  fpath = GetStringEntry(fdata->file_path);
  
  sprintf(fullname, "%s%s", fpath, string);
  
  /* right here we should check the validity of the file name */
  /* and abort if invalid */
  
  strcpy(fdata->fname, fullname);

  SetCurrentWindow(fdata->freq_window);
  CloseWindow();
  return;
}


/*
 * This function is just a wrapper for mystrcmp(), and is called by qsort()
 * (if used) down below.
 */
static int mystrcmp(const void *a, const void *b)
{
  return strcmp(*(char **)a, *(char **)b);
}


/* 
 * Color selector widget.  The function SelectColor() pops up a modal window
 * that lets a user interactively play with a color in either RGB or
 * HSV or CMYK space and then returns the chosen value to your program.
 *
 * Written by Allen Martin (amartin@cs.wpi.edu)
 * Improved by J.-P. Demailly (demailly@ujf-grenoble.fr)
 * SelectColor() now accesses the RGB data file and has a Grab color button
 *
 */

char *SX_ColorSelector_Label[] = {
};

#define RGBTXT     "/etc/X11/rgb.txt"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <X11/StringDefs.h>
#include "libsx.h"
#include "libsx_private.h"


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define BOX_WIDTH  120
#define BOX_HEIGHT 120

extern int strcasecmp __P ((__const char *__s1, __const char *__s2));
extern int lstat __P ((__const char *__file, struct stat *__buf));
extern float __dir__;
static int whichdir;

static int CYAN=0, MAGENTA=0;
static int w_mode, w_r, w_g, w_b, w_h, w_s, w_v, w_c, w_m, w_y, w_k;
static int mydepth;

/* Static (private) procedures
static void color_cancel(Widget w, CSelData *cdata);
static void grab_color(Widget w, CSelData *cdata);
static void color_ok(Widget w, CSelData *cdata);
static void color_byname(Widget w, char *string, CSelData *cdata);
static void color_string(Widget w, char *string, CSelData *cdata);
static void color_scroll(Widget w, float val, CSelData *cdata);
static void color_redisplay(Widget w, int width, int height, CSelData *cdata);
static void rgb_hsv_cmyk(Widget w, CSelData *cdata);
static void rgb2hsv(CSelData *cdata);
static void hsv2rgb(CSelData *cdata);
static void rgb2cmyk(CSelData *cdata);
static void cmyk2rgb(CSelData *cdata);
static void set_rgb_data(int r, int g, int b, CSelData *cdata);
static void set_widgets(int reset, CSelData *cdata);
static void load_list(char *filename, CSelData *cdata);
static void list_click(Widget w, char *string, int index, CSelData *fdata);
static void reset_list(Widget w, CSelData *cdata);
static void best_match(int mode, CSelData *cdata);
static void show_best_match(Widget w, CSelData *cdata);
*/

static int my_trunc(float v)
{
  return (int)(0.5+v);
}

static float my_sqrt(float v)
{
int i;
float x = 50;

  if (v<=0) return 0.0;
  for (i=0; i<=20; i++)
    x = 0.5*(x+v/x);
  return x;
}

static void rgb2cmyk(CSelData *cdata)
{
  float r = 100.0-(cdata->r/2.55) ;
  float g = 100.0-(cdata->g/2.55) ;
  float b = 100.0-(cdata->b/2.55) ;

  cdata->k = MIN(r, MIN(g, b));
  if ( cdata->k<0 ) cdata->k = 0;

  cdata->c = r - cdata->k;
  cdata->m = g - cdata->k;
  cdata->y = b - cdata->k;
}

static void cmyk2rgb(CSelData *cdata)
{
  cdata->r = 255.0 - (cdata->k+cdata->c)*2.55;
  if (cdata->r<0) cdata->r=0;
  cdata->g = 255.0 - (cdata->k+cdata->m)*2.55;
  if (cdata->g<0) cdata->g=0;
  cdata->b = 255.0 - (cdata->k+cdata->y)*2.55;
  if (cdata->b<0) cdata->b=0;
}

static void rgb2hsv(CSelData *cdata)
{
  float   max = MAX(cdata->r,MAX(cdata->g,cdata->b));
  float   min = MIN(cdata->r,MIN(cdata->g,cdata->b));
  float   delta;
  
  cdata->v = max;
  if (max != 0.0)
    cdata->s = (max - min) / max;
  else
    cdata->s = 0.0;
  
  if (cdata->s == 0)
     cdata->h = 0;
  else
   {
     delta = max - min;
     if (cdata->r == max) 
       cdata->h = (cdata->g - cdata->b) / delta;
     else if (cdata->g == max)
       cdata->h = 2 + (cdata->b - cdata->r) / delta;
     else if (cdata->b == max)
       cdata->h = 4 + (cdata->r - cdata->g) / delta;

     cdata->h *= 60;
     if (cdata->h < 0.0)
       cdata->h += 360;
   }

  cdata->s *= 100.0;
}

static void hsv2rgb(CSelData *cdata)
{
  int     i;
  float   f,h,p,q,t;
  float   s=cdata->s;

  s /= 100.0;
  
  if (s == 0 && cdata->h == 0)
   {
     cdata->r = cdata->g = cdata->b = cdata->v;
   }
  else
   {
     if (cdata->h > 360.499)
       cdata->h = 0.0;
     h = cdata->h / 60.0;
     
     i = h;
     f = h - i;
     p = cdata->v * (1 - s);
     q = cdata->v * (1 - (s * f));
     t = cdata->v * (1 - (s * (1 - f)));
     switch (i)
      {
	case 0: cdata->r = cdata->v; cdata->g = t; cdata->b = p; break;
	case 1: cdata->r = q; cdata->g = cdata->v; cdata->b = p; break;
	case 2: cdata->r = p; cdata->g = cdata->v; cdata->b = t; break;
	case 3: cdata->r = p; cdata->g = q; cdata->b = cdata->v; break;
	case 4: cdata->r = t; cdata->g = p; cdata->b = cdata->v; break;
	case 5: cdata->r = cdata->v; cdata->g = p; cdata->b = q; break;
      }
   }
}

/*
 * Update color widgets: labels, strings, scrolls and draw_area
 */
static void set_widgets(int reset, CSelData *cdata)
{
  char cvalue[80];

  if(cdata->mode == 0)
    {
      if (!reset)
	{
        rgb2hsv(cdata);
        rgb2cmyk(cdata);
	}

      if (w_mode != cdata->mode)
	{
        SetLabel(cdata->lmode,       "RGB ");
        SetLabel(cdata->red_label,   "R");
        SetLabel(cdata->green_label, "G");
        SetLabel(cdata->blue_label,  "B");
        SetBgColor((whichdir)?cdata->red_scroll:cdata->red_string, RED);
        SetBgColor((whichdir)?cdata->green_scroll:cdata->green_string, GREEN); 
        SetBgColor((whichdir)?cdata->blue_scroll:cdata->blue_string, BLUE);  
        XtVaSetValues(cdata->black_label,  XtNmappedWhenManaged, False, NULL);
        XtVaSetValues(cdata->black_string, XtNmappedWhenManaged, False, NULL);
        XtVaSetValues(cdata->black_scroll, XtNmappedWhenManaged, False, NULL);
        w_mode = cdata->mode;
        reset = True;
	}

      if (reset || my_trunc(cdata->r) != w_r)
	{
        w_r = my_trunc(cdata->r);
        sprintf(cvalue, "%d", w_r);
        SetScrollbar(cdata->red_scroll, (float)cdata->r, 255.0, 1.0);
        SetStringEntry(cdata->red_string, cvalue);
	}

      if (reset || my_trunc(cdata->g) != w_g)
	{
        w_g = my_trunc(cdata->g);
        sprintf(cvalue, "%d", w_g);
        SetScrollbar(cdata->green_scroll, (float)cdata->g, 255.0, 1.0);
        SetStringEntry(cdata->green_string, cvalue);
	}
      
      if (reset || my_trunc(cdata->b) != w_b)
	{
        w_b = my_trunc(cdata->b);
        sprintf(cvalue, "%d", w_b);
        SetScrollbar(cdata->blue_scroll, (float)cdata->b, 255.0, 1.0);
        SetStringEntry(cdata->blue_string, cvalue);
	}
    }

  if(cdata->mode == 1)
    {
      if (!reset)
	{
        hsv2rgb(cdata);
        rgb2cmyk(cdata);
	}

      if (w_mode != cdata->mode)
	{
        SetLabel(cdata->lmode,       "HSV ");
        SetLabel(cdata->red_label,   "H");
        SetLabel(cdata->green_label, "S");
        SetLabel(cdata->blue_label,  "V");
        SetBgColor((whichdir)?cdata->red_scroll:cdata->red_string, WHITE);
        SetBgColor((whichdir)?cdata->green_scroll:cdata->green_string, WHITE); 
        SetBgColor((whichdir)?cdata->blue_scroll:cdata->blue_string, WHITE);  
        XtVaSetValues(cdata->black_label,  XtNmappedWhenManaged, False, NULL);
        XtVaSetValues(cdata->black_string, XtNmappedWhenManaged, False, NULL);
        XtVaSetValues(cdata->black_scroll, XtNmappedWhenManaged, False, NULL);
        w_mode = cdata->mode;
        reset = True;
	}

      if (reset || my_trunc(cdata->h) != w_h)
	{
        w_h = my_trunc(cdata->h);
        sprintf(cvalue, "%d", w_h);
        SetScrollbar(cdata->red_scroll, (float)cdata->h, 360.0, 1.0);
        SetStringEntry(cdata->red_string, cvalue);
	}
      
      if (reset || my_trunc(cdata->s) != w_s)
	{
        w_s = my_trunc(cdata->s);
        sprintf(cvalue, "%d", w_s);
        SetScrollbar(cdata->green_scroll, (float)cdata->s, 100.0, 1.0);
        SetStringEntry(cdata->green_string, cvalue);
	}
      
      if (reset || my_trunc(cdata->v) != w_v)
	{
        w_v = my_trunc(cdata->v);
        sprintf(cvalue, "%d", w_v);
        SetScrollbar(cdata->blue_scroll, (float)cdata->v, 255.0, 1.0);
        SetStringEntry(cdata->blue_string, cvalue);
	}
    }

  if(cdata->mode == 2)
    {
      if (!reset)
	{
        cmyk2rgb(cdata);
        rgb2hsv(cdata);
	}

      if (w_mode != cdata->mode)
	{
        SetLabel(cdata->lmode,       "CMYK");
        SetLabel(cdata->red_label,   "C");
        SetLabel(cdata->green_label, "M");
        SetLabel(cdata->blue_label,  "Y");
        SetLabel(cdata->black_label, "K");
        SetBgColor((whichdir)?cdata->red_scroll:cdata->red_string, CYAN);  
        SetBgColor((whichdir)?cdata->green_scroll:cdata->green_string,MAGENTA);
        SetBgColor((whichdir)?cdata->blue_scroll:cdata->blue_string, YELLOW);
        XtVaSetValues(cdata->black_label,  XtNmappedWhenManaged, True, NULL);
        XtVaSetValues(cdata->black_string, XtNmappedWhenManaged, True, NULL);
        XtVaSetValues(cdata->black_scroll, XtNmappedWhenManaged, True, NULL);
        w_mode = cdata->mode;
        reset = True;
	}

      if (reset || my_trunc(cdata->c) != w_c)
	{
        w_c = my_trunc(cdata->c);
        sprintf(cvalue, "%d", w_c);
        SetScrollbar(cdata->red_scroll, (float)cdata->c, 100.0, 1.0);
        SetStringEntry(cdata->red_string, cvalue);
	}
      
      if (reset || my_trunc(cdata->m) != w_m)
	{
        w_m = my_trunc(cdata->m);
        sprintf(cvalue, "%d", w_m);
        SetScrollbar(cdata->green_scroll, (float)cdata->m, 100.0, 1.0);
        SetStringEntry(cdata->green_string, cvalue);
	}
      
      if (reset || my_trunc(cdata->y) != w_y)
	{
        w_y = my_trunc(cdata->y);
        sprintf(cvalue, "%d", w_y);
        SetScrollbar(cdata->blue_scroll, (float)cdata->y, 100.0, 1.0);
        SetStringEntry(cdata->blue_string, cvalue);
	}

      if (reset || my_trunc(cdata->k) != w_k)
	{
        w_k = my_trunc(cdata->k);
        sprintf(cvalue, "%d", w_k);
        SetScrollbar(cdata->black_scroll, (float)cdata->k, 100.0, 1.0);
        SetStringEntry(cdata->black_string, cvalue);
	}
  }

  if (mydepth<16)
    SetPrivateColor(cdata->color, (int)cdata->r, (int)cdata->g, (int)cdata->b);
  else 
  {
    cdata->color = GetRGBColor((int)cdata->r, (int)cdata->g, (int)cdata->b);
    --lsx_curwin->color_index;
    SetDrawArea(cdata->draw);
    SetFgColor(cdata->draw, cdata->color);
    SetBgColor(cdata->draw, cdata->color);
    ClearDrawArea();
  }

}

static void rgb_hsv_cmyk(Widget w, CSelData *cdata)
{
  cdata->mode = (1+cdata->mode) % 3;
  set_widgets(True, cdata);
}

void set_rgb_data(int r, int g, int b, CSelData *cdata)
{
    cdata->r = r;
    cdata->g = g;
    cdata->b = b;
    rgb2hsv(cdata);
    rgb2cmyk(cdata);
    set_widgets(False, cdata);
}
/*
 * grab_color() - Callback for color grab button
 */
static void grab_color(Widget w, CSelData *cdata)
{
char col_str[80];
int  r, g, b;

    strcpy(col_str, GrabPixel("%r %g %b"));
    sscanf(col_str, "%d %d %d", &r, &g, &b);
    set_rgb_data(r, g, b, cdata);
}

static void best_match(int mode, CSelData *cdata)
{
int num=0, i, j, k;
int r, g, b;
int   rank[1000];
float fr=cdata->r, fg=cdata->g, fb=cdata->b;
float d[1000];
float perc;
char  format[30];
  
  while(cdata->rgb_ptr[num])
    {
    sscanf(cdata->rgb_ptr[num], "%d %d %d", &r, &g, &b);
    d[num] = (fr-r)*(fr-r)+(fg-g)*(fg-g)+(fb-b)*(fb-b);
    num++;
    }

  rank[0] = 0;
  for(i=1; i<num; i++)
    {
    j = 0;
    while ((j<i) && (d[i]>=d[rank[j]])) j++;
    if (j==i) 
      rank[i] = i;
    else
      {
      for(k=i; k>j; k--) rank[k] = rank[k-1];
      rank[j] = i;
      }
    }

  sprintf(cdata->match_list[0], "  %3d %3d %3d      %s",
          my_trunc(fr), my_trunc(fg), my_trunc(fb), 
          SX_Dialog[COLSEL_DIAL]);
    cdata->match_ptr[0] = cdata->match_list[0];

  if (mode)
    {
    for(i=1; i<=num; i++)
      {
      sprintf(format, "%%s%%%ds %%s%%2.2f %%%%", 
              (int)(50-strlen(cdata->rgb_ptr[rank[i-1]])));
      perc = my_sqrt((d[rank[i-1]]/19.5075));
      sprintf(cdata->match_list[i], format, 
            cdata->rgb_ptr[rank[i-1]], "", (perc==0)?"":" ",100-perc);
      cdata->match_ptr[i] = cdata->match_list[i];
      }
    cdata->match_ptr[num] = NULL;
    }
  else
    cdata->match_ptr[0]=cdata->rgb_ptr[rank[0]]; 
}

static void show_best_match(Widget w, CSelData *cdata)
{
  best_match(1, cdata);
  ChangeScrollList(cdata->scroll_list, cdata->match_ptr);
}

/*
 * color_ok() - Callback for color requestor OK button
 */
static void color_ok(Widget w, CSelData *cdata)
{
int r, g, b;

  cdata->cancelled = 0;
  if(cdata->output==0)
    {
    numerical:
    sprintf(cdata->save, "#%02X%02X%02X",      
          my_trunc(cdata->r), my_trunc(cdata->g), my_trunc(cdata->b));
    }
  else
    {
    best_match(0, cdata);
    sscanf(cdata->match_ptr[0], "%d %d %d %s", &r, &g, &b, cdata->save);
    if (cdata->output==1)
      {
      if (r!=my_trunc(cdata->r)
             || g!=my_trunc(cdata->g)
             || b!=my_trunc(cdata->b)) goto numerical;
      }
    }
  if (mydepth<16)
     FreePrivateColor(cdata->color);
  SetCurrentWindow(cdata->csel_window);
  CloseWindow();
}

/*
 * color_cancel() - Callback for color requestor CANCEL button
 */
static void color_cancel(Widget w, CSelData *cdata)
{
  cdata->cancelled = 1;
  SetCurrentWindow(cdata->csel_window);
  if (mydepth<16)
     FreePrivateColor(cdata->color);
  CloseWindow();
}

static void set_color_values(int i, float val, CSelData *cdata)
{
  val = MAX(val, 0);

  if(cdata->mode == 0)
    {
      /* range the value */
      val = MIN(255, val);

      if(i==0)
	  cdata->r = val;
      else if(i==1)
	  cdata->g = val;
      else if(i==2)
	  cdata->b = val;
    }
  
  if(cdata->mode == 1)
    {
      if(i==0)
	{
	  val = MIN(360, val);
	  cdata->h = val;
	}
      else if(i==1)
	{
	  val = MIN(100, val);
	  cdata->s = val;
	}
      else if(i==2)
	{
	  val = MIN(255, val);
	  cdata->v = val;
	}
    }      

  if(cdata->mode == 2)
    {
      if(i==0)
	{
	  val = MIN(100, val);
	  cdata->c = val;
	}
      else if(i==1)
	{
	  val = MIN(100, val);
	  cdata->m = val;
	}
      else if(i==2)
	{
	  val = MIN(100, val);
	  cdata->y = val;
	}
      else if(i==3)
	{
	  val = MIN(100, val);
	  cdata->k = val;
	}
    }
  set_widgets(False, cdata);
}

static void color_byname(Widget w, char *string, CSelData *cdata)
{
int r, g, b, i;
char name[40], col_str[40];

  if (string[0]>='0' && string[0]<='9')
    {
    if (sscanf(string, "%d,%d,%d", &r, &g, &b)<3)
        sscanf(string, "%d %d %d", &r, &g, &b);
    }
  else
    if (string[0]=='#')
        sscanf(string, "#%02X%02X%02X", &r, &g, &b);
  else
    if (isalpha(string[0]))
      {
      sscanf(string, "%s", name);
      i = 0;
    iterate:
      if (cdata->rgb_ptr[i] == NULL) return;
      sscanf(cdata->rgb_ptr[i], "%d %d %d %s", &r, &g, &b, col_str);
      ++i;
      if (strcasecmp(name,col_str)) goto iterate;        
      }
  else
    return;

  set_rgb_data(r, g, b, cdata);  
}

static void color_string(Widget w, char *string, CSelData *cdata)
{
  float val;
  int i = 0;

  sscanf(string, "%f", &val);

  if (w==cdata->red_string) 
     { i = 0; w_r = w_h = w_c = -1; } else
  if (w==cdata->green_string) 
     { i = 1; w_g = w_s = w_m = -1; } else
  if (w==cdata->blue_string) 
     { i = 2; w_b = w_v = w_y = -1; } else
  if (w==cdata->black_string) 
     { i = 3; w_k = -1; } 
  set_color_values(i, val, cdata);
}

static void color_scroll(Widget w, float val, CSelData *cdata)
{
int i = 0;

  if (w==cdata->red_scroll) 
     i = 0; else
  if (w==cdata->green_scroll) 
     i = 1; else
  if (w==cdata->blue_scroll) 
     i = 2; else
  if (w==cdata->black_scroll) 
     i = 3; 
  set_color_values(i, val, cdata);
}
  

/*
 * color_redisplay() - Redisplay callback for the color draw area
 */
static void color_redisplay(Widget w, int width, int height, CSelData *cdata)
{
  DrawFilledBox(0, 0, width, height);
}

static void load_list(char *filename, CSelData *cdata)
{
struct stat st;
char *pline;
char rgb_line[256];
FILE *fd;
int  i;
int r, g, b;
char name[40], comp1[20], comp2[20], comp3[20];

  pline = (char *)1;
  if(lstat(RGBTXT,&st)!=0)
    {
    fprintf(stderr, "The RGB colors data file %s does not exist !!\n", RGBTXT);
    return;
    }

  fd=fopen(RGBTXT, "r");

  i = 0;
  while(pline!=NULL)
    {
    pline=fgets(rgb_line,76,fd);
    if((pline!=NULL) && (rgb_line[0]!='!')) 
      {
      *comp1 = '\0';
      sscanf(pline, "%d %d %d %s %s %s %s", &r, &g, &b, name, 
                comp1, comp2, comp3);

      if (!*comp1)
        {
        name[0] = toupper(name[0]);
        sprintf(cdata->rgb_list[i], "  %3d %3d %3d      %s", r, g, b, name);
        cdata->rgb_ptr[i] = cdata->rgb_list[i];
        ++i;
	}
      }
    }
  strcpy(cdata->rgb_list[i], "");
  cdata->rgb_ptr[i] = NULL;

  fclose(fd);
  ChangeScrollList(cdata->scroll_list, cdata->rgb_ptr);
}

static void reset_list(Widget w, CSelData *cdata)
{
  ChangeScrollList(cdata->scroll_list, cdata->rgb_ptr);
}

static void list_click(Widget w, char *string, int index, CSelData *cdata)
{
int r, g, b;
char name[40], color_line[80];

  sscanf(string, "%d %d %d %s\n", &r, &g, &b, name);
  set_rgb_data(r, g, b, cdata);
  sprintf(color_line, "%s  =  #%02X%02X%02X", name, r, g, b);
  SetStringEntry(cdata->lcolor, color_line);
}


/*
 * SelectColor()
 */
char *
SelectColor(char *inicolor, int output, char *txt, CSelCB func, void *data)
{
  CSelData cdata;
  Widget w[30];
  int width, i;
  static char black_bits[] = { 0xff };
  static char col_str[80];
  
  /* initialize the color values */
  cdata.mode = 0;
  cdata.output = output;
  cdata.data = data;
  data = 0;

  w_mode=-1, 
  w_r=-1, w_g=-1, w_b=-1,
  w_h=-1, w_s=-1, w_v=-1,
  w_c=-1, w_m=-1, w_y=-1, w_k=-1;

  cdata.csel_window = MakeWindow("ColorSelector", SAME_DISPLAY,
				 EXCLUSIVE_WINDOW);

  mydepth = DefaultDepth(lsx_curwin->display,lsx_curwin->screen);
  whichdir = (__dir__>0);

  w[0]  = MakeLabel(txt);
  w[1]  = MakeLabel(SX_Dialog[COLSEL_DIAL+1]);
  w[2]  = MakeStringEntry(NULL, 284, (void *)color_byname, &cdata);

  w[3]  = MakeLabel(SX_Dialog[COLSEL_DIAL+2]);
  w[4]  = MakeButton("CMYK", (void *)rgb_hsv_cmyk, &cdata);

  /* determine the width to make the string widgets with */
  width = TextWidth(GetWidgetFont(w[0]),  "8888888");
  if (width <= 0) width = 48;
  
  w[5]  = MakeLabel("R");
  w[6]  = MakeStringEntry(NULL, width, (void *)color_string, &cdata);
  w[7]  = MakeHorizScrollbar(284, (void *)color_scroll, &cdata);
  SetScrollbarStep(w[7], 0.03);

  w[8]  = MakeLabel("G");
  w[9]  = MakeStringEntry(NULL, width, (void *)color_string, &cdata);
  w[10]  = MakeHorizScrollbar(284, (void *)color_scroll, &cdata);
  SetScrollbarStep(w[10], 0.03);

  w[11]  = MakeLabel("B");
  w[12]  = MakeStringEntry(NULL, width, (void *)color_string, &cdata);
  w[13]  = MakeHorizScrollbar(284, (void *)color_scroll, &cdata);
  SetScrollbarStep(w[13], 0.03);

  w[14]  = MakeLabel("K");
  w[15]  = MakeStringEntry(NULL, width, (void *)color_string, &cdata);
  w[16]  = MakeHorizScrollbar(284, (void *)color_scroll, &cdata);
  SetScrollbarStep(w[16], 0.03);

  w[17] = MakeDrawArea(BOX_WIDTH, BOX_HEIGHT, (void *)color_redisplay, &cdata);

  w[18] = MakeButton(SX_Dialog[COLSEL_DIAL+3], (void *)reset_list, &cdata);
  cdata.rgb_ptr[0] = NULL;
  w[19] = MakeScrollList(cdata.rgb_ptr, 
               564, 500, (void *)list_click,&cdata);
  
  w[20] = MakeButton(SX_Dialog[COLSEL_DIAL+4],(void *)grab_color, &cdata);
  w[21] = MakeButton(SX_Dialog[COLSEL_DIAL+5],(void *)show_best_match,&cdata);
  w[22] = MakeButton(SX_Dialog[COLSEL_DIAL+6],(void *)color_ok, &cdata);
  w[23] = MakeButton(SX_Dialog[COLSEL_DIAL+7],(void *)color_cancel, &cdata);
  if (func) 
    {
    w[24] = MakeButton(txt, (ButtonCB)func, &cdata);
    XtVaSetValues(w[24], "horizDistance", 126, NULL);
    SetWidgetPos(w[24], PLACE_UNDER, w[19], PLACE_RIGHT, w[22]);
    if (BUTTONBG>=0)
       SetBgColor(w[24], BUTTONBG);
    }
  
  SetWidgetPos(w[1],  PLACE_UNDER, w[0], NO_CARE,     NULL);
  SetWidgetPos(w[2],  PLACE_RIGHT, w[1], PLACE_UNDER, w[0]);
  SetWidgetPos(w[3],  PLACE_RIGHT, w[2], PLACE_UNDER, w[0]);
  SetWidgetPos(w[4],  PLACE_RIGHT, w[3], PLACE_UNDER, w[0]);
  SetWidgetPos(w[5],  PLACE_UNDER, w[2],  NO_CARE,     NULL);
  SetWidgetPos(w[6],  PLACE_UNDER, w[2],  PLACE_RIGHT, w[5]);
  SetWidgetPos(w[7],  PLACE_UNDER, w[2],  PLACE_RIGHT, w[6]);
  SetWidgetPos(w[8],  PLACE_UNDER, w[5],  NO_CARE,     NULL);
  SetWidgetPos(w[9],  PLACE_UNDER, w[5],  PLACE_RIGHT, w[8]);
  SetWidgetPos(w[10], PLACE_UNDER, w[5],  PLACE_RIGHT, w[9]);
  SetWidgetPos(w[11], PLACE_UNDER, w[8],  NO_CARE,     NULL);
  SetWidgetPos(w[12], PLACE_UNDER, w[8],  PLACE_RIGHT, w[11]);
  SetWidgetPos(w[13], PLACE_UNDER, w[8],  PLACE_RIGHT, w[12]);
  SetWidgetPos(w[14], PLACE_UNDER, w[11], NO_CARE,     NULL);
  SetWidgetPos(w[15], PLACE_UNDER, w[11], PLACE_RIGHT, w[14]);
  SetWidgetPos(w[16], PLACE_UNDER, w[11], PLACE_RIGHT, w[15]);
  SetWidgetPos(w[17], PLACE_UNDER, w[2],  PLACE_RIGHT, w[7]);
  SetWidgetPos(w[18], PLACE_UNDER, w[17], NO_CARE,     NULL);
  SetWidgetPos(w[19], PLACE_UNDER, w[18], NO_CARE,     NULL);
  SetWidgetPos(w[20], PLACE_UNDER, w[19], NO_CARE,     NULL);
  SetWidgetPos(w[21], PLACE_UNDER, w[19], PLACE_RIGHT, w[20]);
  SetWidgetPos(w[22], PLACE_UNDER, w[19], PLACE_RIGHT, w[21]);
  SetWidgetPos(w[23], PLACE_UNDER, w[19], PLACE_RIGHT, w[22]);
  XtVaSetValues(w[18], "vertDistance", 8, NULL);

  /* save important widgets */

  cdata.lcolor       = w[2];
  cdata.lmode        = w[4];

  cdata.red_label    = w[5];
  cdata.red_string   = w[6];
  cdata.red_scroll   = w[7];

  cdata.green_label  = w[8];
  cdata.green_string = w[9];
  cdata.green_scroll = w[10];

  cdata.blue_label   = w[11];
  cdata.blue_string  = w[12];
  cdata.blue_scroll  = w[13];

  cdata.black_label  = w[14];
  cdata.black_string = w[15];
  cdata.black_scroll = w[16];

  cdata.draw         = w[17];
  cdata.scroll_list  = w[19];

  XtVaSetValues(cdata.red_label, "vertDistance", 12, NULL);
  XtVaSetValues(cdata.red_string, "vertDistance", 12, NULL);
  XtVaSetValues(cdata.red_scroll, "vertDistance", 12, NULL);
  XtVaSetValues(cdata.draw, "horizDistance", 19, "vertDistance", 11, NULL);
  XtVaSetValues(cdata.red_scroll, "foreground", "red", NULL);

  ShowDisplay();
  load_list(RGBTXT, &cdata);

  /* allocate the custom color */

  SetDrawArea(cdata.draw);
  if (mydepth<16)
    {
    cdata.color = GetPrivateColor();
    SetColor(cdata.color);
    }

  SetThumbBitmap(cdata.red_scroll, black_bits, 8, 1);
  SetThumbBitmap(cdata.green_scroll, black_bits, 8, 1);
  SetThumbBitmap(cdata.blue_scroll, black_bits, 8, 1);
  SetThumbBitmap(cdata.black_scroll, black_bits, 8, 1);

  if (INPUTBG) 
    {
    SetBgColor(w[2], INPUTBG);
    for(i=5; i<=15; i++) 
        if (i%3 != 2) SetBgColor(w[i], INPUTBG);
    SetBgColor(w[19], INPUTBG);
    }

  if (BUTTONBG) 
    {
    SetBgColor(w[4], BUTTONBG);
    SetBgColor(w[18], BUTTONBG);
    for(i=20; i<=23; i++) SetBgColor(w[i], BUTTONBG);
    }

  if (CYAN==0)
     CYAN = GetNamedColor("cyan");
  if (MAGENTA==0)
     MAGENTA = GetNamedColor("magenta");
  
  /* SetBgColor(cdata.black_scroll, INPUTBG); */
  
  if (inicolor) 
    color_byname(w[2], inicolor, &cdata);
  else
    {
    cdata.r = 0;
    cdata.g = 0;
    cdata.b = 0;
    set_widgets(False, &cdata);
    }

    i = GetRGBColor(0xdc, 0xda, 0xd5);
    SetBgColor(cdata.red_scroll, i);
    SetBgColor(cdata.green_scroll, i);
    SetBgColor(cdata.blue_scroll, i);


  MainLoop();

  data = cdata.data;
  if (cdata.cancelled)
    return NULL;
  else
    {
    strcpy(col_str,cdata.save);
    return col_str;
    }
}

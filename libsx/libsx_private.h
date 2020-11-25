/*
 * Structures and variables private to libsx.
 */

typedef struct WindowState
{
  int      screen;
  int      window_shown;
  Window   window;
  Display *display;
  Widget   toplevel, toplevel_form, form_widget, last_draw_widget;
  int      has_standard_colors;
  int      named_colors[256];
  int      color_index;
  Colormap     cmap;
  Pixmap       check_mark;
  XFontStruct *font;

#ifdef    OPENGL_SUPPORT

  XVisualInfo *xvi;
  GLXContext   gl_context;
  Colormap     draw_area_cmap;
  int          draw_area_depth;
  
#endif /* OPENGL_SUPPORT */
  
  struct WindowState *next;
}WindowState;


/* global list of open windows. defined in libsx.c, used everywhere */
extern volatile WindowState *lsx_curwin;  



typedef struct DrawInfo
{
  RedisplayCB   redisplay;
  MouseButtonCB button_down;
  MouseButtonCB button_up;
  KeyCB         keypress;
  MotionCB      motion;
  EnterCB	enter;
  LeaveCB	leave;

  GC            drawgc;       /* Graphic Context for drawing in this widget */

  int           foreground;   /* need to save these so we know what they are */
  int           background; 
	int 				  width;
	int 					line_style;
  unsigned long mask;
  XFontStruct  *font;

  void        *user_data;

  Widget widget;
  struct DrawInfo *next;
}DrawInfo;


DrawInfo *libsx_find_draw_info(Widget w);  /* private internal function */

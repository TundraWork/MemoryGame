/*For comments on programs and variables, see main.h and utils.h*/
#include <libsx.h>
#include <unistd.h>
#include "loadbmp.h"

SharedData shared;

/*Initialize all data*/
extern void init_data();

/*Delete space from string*/
extern char *trim_whitespace(char *str);

/*Enter the id of the grid, offset_x and offset_y will be 
  assigned the coordinates of the upper left corner of the grid*/
extern void get_area_offset(int area, int *offset_x, int *offset_y);

/*Input coordinates x and y, it will return the id of the grid;*/
extern int get_area_from_coordinate(int x, int y);

/*The function GetRGBColor() from libsx will segmentation fault after 
  using it over 256 times,so we wrote a function to implement it*/
extern int get_rgb_color(uint8_t r, uint8_t g, uint8_t b);

/*Callback of MakeDrawArea()*/
extern void create_drawarea(Widget w, int width, int height, void *data);

/*Load bmp to our data structure,using the libriry 
  "libnsbmp.h" from http://www.netsurf-browser.org/projects/libnsbmp/ */
extern void load_bmp_to_shared(char *file);

/*We put all the pictures in one BMP, use the number of lines(of pixels) to 
  distinguish the pictures, "area" is the id of the grid to be 
  drawn, "offset" is the starting line of the picture to be used*/
extern void fill_drawarea_from_shared(Widget w, int offset, int area);

/*Callback of mouse buttondown,used to achieve mouse buttondown effect*/
extern void drawarea_buttondown(Widget w, int button, int x, int y, void *data);

/*Callback of mouse buttonup,include main logic*/
extern void drawarea_buttonup(Widget w, int button, int x, int y, void *data);

/*If two opened cards are different, use this function to flop the cards*/
extern void flop_back();

/*Callback of mouse move,used to achieve mouse over effect*/
extern void drawarea_mousemove(Widget w, int x, int y, void *data);

/*Callback of button Quit!*/
extern void quit(Widget w, void *data);

/*A timer made by AddTimeOut()*/
extern void timing();

/*Write the scoreboard to scoreboard.txt*/
extern void save_score();

/*Read the scoreboard from scoreboard.txt*/
extern void load_score();

/*Callback of button Restart,initialize all data and clead the screen*/
extern void restart(Widget w, void *data);

/*Callback of button ScoreBoard,show scoreboard in a new window*/
extern void scoreboard(Widget w, void *data);

/*Callback of button About*/
extern void about(Widget w, void *data);

/*Callback of button Close in window "About" and "ScoreBoard"*/
extern void closewindow(Widget w, void *data);

/*Compare the score you just got with the the No,i ranking on the 
  scoreboard,return 1 or 0*/
extern int compare_score(int i);

/*When player finish the game,if he is fast enough,
  goto end_a,add him to scoreboard,otherwise,goto end_b*/
extern void choose_end();

/*This function is used to change the scoreboard, write the latest score 
  into the correct position, and move the rest of the ranking down*/
extern void generate_scoreboard();

/*Ask the player's name and write into scoreboard*/
extern void end_a();

/*Tell the player he is not fast enough*/
extern void end_b();

/*Callback of string in end*/
extern void end_a_enter(Widget w, char *string, void *data);

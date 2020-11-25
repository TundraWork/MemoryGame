/*For comments on programs and variables, see main.h and utils.h*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include "loadbmp.h"
#include "main.h"
#include "utils.h"

int main(int argc, char **argv) {
  if (OpenDisplay(argc, argv) == FALSE) {
    fprintf(stderr, "Failed to call OpenDisplay\n");
    exit(-1);
  }

  init_display(argc, argv);
  MainLoop();

  return 0;
}

void init_display(int argc, char **argv) {
  Widget misc[8];
  Widget draw;
  int area;
  char best_score[64];
  int Fgcolor;
  int Bordercolor;

  init_data();
  sprintf(best_score,
          "  [Best] %02d:%02d.%d (%d Steps) by %-12.12s",
          shared.SAVE_TIME_MIN[0],
          shared.SAVE_TIME_S[0],
          shared.SAVE_TIME_MS[0],
          shared.SAVE_STEPS[0],
          shared.SAVE_PLAYER[0]);

  misc[0] = MakeLabel("    [Time] 00:00.0");
  misc[1] = MakeLabel(best_score);
  misc[2] = MakeButton("Restart", restart, NULL);
  misc[3] = MakeButton("ScoreBoard", scoreboard, NULL);
  misc[4] = MakeButton(" About ", about, NULL);
  misc[5] = MakeButton(" Quit! ", quit, NULL);
  misc[6] = MakeLabel(" ");
  misc[7] = MakeLabel(" ");
  draw = MakeDrawArea(GRID_WIDTH * 4, GRID_HEIGHT * 4, create_drawarea, NULL);

  shared.LABEL[0] = misc[0];
  shared.LABEL[1] = misc[1];

  Fgcolor = get_rgb_color(100, 60, 150);
  Bordercolor = get_rgb_color(50, 50, 255);

  SetFgColor(misc[2], Fgcolor);
  SetBorderColor(misc[2], Bordercolor);
  SetFgColor(misc[3], Fgcolor);
  SetBorderColor(misc[3], Bordercolor);
  SetFgColor(misc[4], Fgcolor);
  SetBorderColor(misc[4], Bordercolor);
  SetFgColor(misc[0], Fgcolor);

  Fgcolor = get_rgb_color(255, 0, 0);
  Bordercolor = get_rgb_color(255, 0, 0);

  SetFgColor(misc[5], Fgcolor);
  SetBorderColor(misc[5], Bordercolor);
  SetFgColor(misc[1], Fgcolor);

  Bordercolor = get_rgb_color(255, 255, 255);
  SetBorderColor(draw, Bordercolor);

  SetWidgetPos(misc[1], PLACE_RIGHT, misc[0], NO_CARE, NULL);
  SetWidgetPos(misc[2], PLACE_RIGHT, misc[1], NO_CARE, NULL);
  SetWidgetPos(misc[3], PLACE_RIGHT, misc[2], NO_CARE, NULL);
  SetWidgetPos(misc[4], PLACE_RIGHT, misc[3], NO_CARE, NULL);
  SetWidgetPos(misc[5], PLACE_RIGHT, misc[4], NO_CARE, NULL);
  SetWidgetPos(misc[6], PLACE_UNDER, misc[0], NO_CARE, NULL);
  SetWidgetPos(misc[7], PLACE_RIGHT, draw, PLACE_UNDER, misc[0]);
  SetWidgetPos(draw, PLACE_RIGHT, misc[6], PLACE_UNDER, misc[0]);

  SetButtonDownCB(draw, drawarea_buttondown);
  SetButtonUpCB(draw, drawarea_buttonup);
  SetMouseMotionCB(draw, drawarea_mousemove);

  load_bmp_to_shared("./assets/map.bmp");

  ShowDisplay();

  for (area = 0; area < 16; area++) {
    fill_drawarea_from_shared(draw, 8, area);
  }
}

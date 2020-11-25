#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "main.h"
#include "utils.h"

void init_data() {
  int i;
  int random = 0;

  memset(shared.CARD_ID, 0, 16 * sizeof(uint8_t));
  memset(shared.CARD_STATUS, 0, 16 * sizeof(uint8_t));
  memset(shared.SAVE_TIME_MIN, 0, 10 * sizeof(uint8_t));
  memset(shared.SAVE_TIME_S, 0, 10 * sizeof(uint8_t));
  memset(shared.SAVE_TIME_MS, 0, 10 * sizeof(uint8_t));
  memset(shared.SAVE_STEPS, 0, 10 * sizeof(uint16_t));
  shared.CARD_OPENED = 0;
  shared.CARD_MATCHED = 0;
  shared.CARD_OPENED_1 = 0;
  shared.TIMING = 0;
  shared.TIME_MIN = 0;
  shared.TIME_S = 0;
  shared.TIME_MS = 0;
  shared.STEPS = 0;
  shared.SLEEPING = 0;

  for (i = 0; i < 10; i++) {
    shared.SAVE_PLAYER[i] = "NONE";
  }

  load_score();

  i = 1;
  srand((unsigned)time(NULL));
  do {
    random = rand() % 16;
    if (shared.CARD_ID[random] == 0) {
      shared.CARD_ID[random] = (int)(fabs(8.5 - i) + 0.5);
      i++;
    }
  } while (i < 17);
  for (i = 0; i < 16; i++) {
    shared.CARD_ID[i]--;
  }
}

char *trim_whitespace(char *str) {
  char *end;
  while (isspace((unsigned char)*str))
    str++;
  if (*str == 0)
    return str;
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;
  end[1] = '\0';
  return str;
}

void get_area_offset(int area, int *offset_x, int *offset_y) {
  if (area < 0) {
    fprintf(stderr, "invalid area: %d\n", area);
    exit(-1);
  }
  if (area < 16) {
    *offset_y = (area - area % 4) / 4 * GRID_HEIGHT;
    *offset_x = area % 4 * GRID_WIDTH;
    return;
  } else {
    fprintf(stderr, "invalid area: %d\n", area);
    exit(-1);
  }
}

int get_area_from_coordinate(int x, int y) {
  int area_x, area_y;
  area_x = (x - x % GRID_WIDTH) / GRID_WIDTH;
  if (area_x == 4)
    area_x = 3;
  area_y = (y - y % GRID_HEIGHT) / GRID_HEIGHT;
  if (area_y == 4)
    area_y = 3;
  return area_x + area_y * 4;
}

int get_rgb_color(uint8_t r, uint8_t g, uint8_t b) {
  int c = r;
  c = (c << 8) | g;
  c = (c << 8) | b;
  return c;
}

void create_drawarea(Widget w, int width, int height, void *data) {
  shared.DRAWAREA = w;
}

void load_bmp_to_shared(char *file) {
  bmp_result result;
  bmp_image image;

  result = load_bmp(file, &image);
  if (result != BMP_OK) {
    fprintf(stderr, "Failed to load bitmap file: %s (%d)\n", file, result);
    exit(-1);
  }
  shared.BITMAP_SIZE_X = image.width;
  shared.BITMAP_SIZE_Y = image.height;
  shared.BITMAP_DATA = (uint8_t *)image.bitmap;
}

void fill_drawarea_from_shared(Widget w, int offset, int area) {
  int scan_x, scan_y;
  int draw_offset_x, draw_offset_y;
  size_t read_ptr;
  int color;

  SetDrawArea(w);

  get_area_offset(area, &draw_offset_x, &draw_offset_y);

  for (scan_y = 0; scan_y < GRID_HEIGHT; scan_y++) {
    for (scan_x = 0; scan_x < shared.BITMAP_SIZE_X; scan_x++) {
      read_ptr = ((scan_y + offset * GRID_HEIGHT) * shared.BITMAP_SIZE_X + scan_x) * BYTES_PER_PIXEL;
      color = get_rgb_color(shared.BITMAP_DATA[read_ptr],
                            shared.BITMAP_DATA[read_ptr + 1],
                            shared.BITMAP_DATA[read_ptr + 2]);
      SetColor(color);
      DrawPixel(scan_x + draw_offset_x, scan_y + draw_offset_y);
    }
  }
}

void drawarea_buttondown(Widget w, int button, int x, int y, void *data) {
  int area;
  if (button == 1 && shared.SLEEPING == 0) {
    area = get_area_from_coordinate(x, y);
    if (shared.CARD_STATUS[area] == 0) {
      fill_drawarea_from_shared(w, 10, area);
    }
  }
}

void drawarea_buttonup(Widget w, int button, int x, int y, void *data) {
  int area;
  area = get_area_from_coordinate(x, y);
  if (button == 1 && shared.SLEEPING == 0 && shared.CARD_STATUS[area] == 0) {
    if (shared.STEPS == 0) {
      shared.TIMING = 1;
      timing();
    }
    if (shared.CARD_OPENED == 0) {
      shared.CARD_STATUS[area] = 1;
      fill_drawarea_from_shared(w, shared.CARD_ID[area], area);
      shared.CARD_OPENED_1 = area;
      shared.CARD_OPENED = 1;
    } else {
      if (shared.CARD_ID[area] == shared.CARD_ID[shared.CARD_OPENED_1]) {
        shared.CARD_STATUS[area] = 1;
        fill_drawarea_from_shared(w, shared.CARD_ID[area], area);
        shared.CARD_OPENED = 0;
        shared.CARD_MATCHED += 2;
      } else {
        shared.SLEEPING = 1;
        fill_drawarea_from_shared(w, shared.CARD_ID[area], area);
        shared.CARD_OPENED_2 = area;
        AddTimeOut(FLOP_BACK_TIMEOUT, flop_back, NULL);
      }
    }
    if (shared.CARD_MATCHED == 16) {
      shared.TIMING = 0;
      shared.CARD_MATCHED++;
      choose_end();
    }
    shared.STEPS++;
  }
}

void flop_back() {
  shared.CARD_STATUS[shared.CARD_OPENED_1] = 0;
  fill_drawarea_from_shared(shared.DRAWAREA, 8, shared.CARD_OPENED_1);
  shared.CARD_STATUS[shared.CARD_OPENED_2] = 0;
  fill_drawarea_from_shared(shared.DRAWAREA, 8, shared.CARD_OPENED_2);
  shared.CARD_OPENED = 0;
  shared.SLEEPING = 0;
}

void drawarea_mousemove(Widget w, int x, int y, void *data) {
  int area;
  area = get_area_from_coordinate(x, y);
  if (area == shared.MOUSE_MOVE_AREA) {
    return;
  } else {
    if (shared.CARD_STATUS[shared.MOUSE_MOVE_AREA] == 1) {
      fill_drawarea_from_shared(w, shared.CARD_ID[shared.MOUSE_MOVE_AREA], shared.MOUSE_MOVE_AREA);
    } else {
      if (shared.SLEEPING == 0) {
        fill_drawarea_from_shared(w, 8, shared.MOUSE_MOVE_AREA);
      } else {
        if (shared.MOUSE_MOVE_AREA != shared.CARD_OPENED_2 && shared.MOUSE_MOVE_AREA != shared.CARD_OPENED_1) {
          fill_drawarea_from_shared(w, 8, shared.MOUSE_MOVE_AREA);
        }
      }
    }
    if (shared.CARD_STATUS[area] == 0) {
      if (shared.SLEEPING == 0) {
        fill_drawarea_from_shared(w, 9, area);
      } else {
        if (area != shared.CARD_OPENED_2 && area != shared.CARD_OPENED_1) {
          fill_drawarea_from_shared(w, 9, area);
        }
      }
    }
    shared.MOUSE_MOVE_AREA = area;
  }
}

void quit(Widget w, void *data) { exit(0); }

void timing() {
  if (shared.TIMING != 0) {
    AddTimeOut(100, timing, NULL);
    shared.TIME_MS += 1;
    if (shared.TIME_MS >= 10) {
      shared.TIME_MS -= 10;
      shared.TIME_S++;
    }
    if (shared.TIME_S >= 60) {
      shared.TIME_S -= 60;
      shared.TIME_MIN++;
    }
    char timeis[40];
    sprintf(timeis, "    [Time] %02d:%02d.%d", shared.TIME_MIN, shared.TIME_S, shared.TIME_MS);
    SetLabel(shared.LABEL[0], timeis);
  }
}

void save_score() {
  FILE *write_ptr;
  char data[2048];
  char data_temp[128];
  int i;

  write_ptr = fopen("scoreboard.txt", "w");
  if (!write_ptr) {
    fprintf(stderr, "Failed to write save data to scoreboard.txt, please check directory permissions.");
    exit(-1);
  }
  memset(data, 0, sizeof(data));
  for (i = 0; i < 10; i++) {
    sprintf(data_temp,
            "%02d | Time %02d:%02d.%d | %03d steps | %-12.12s\n",
            i + 1,
            shared.SAVE_TIME_MIN[i],
            shared.SAVE_TIME_S[i],
            shared.SAVE_TIME_MS[i],
            shared.SAVE_STEPS[i],
            shared.SAVE_PLAYER[i]);
    strcat(data, data_temp);
  }
  fprintf(write_ptr, "%s", data);
  fclose(write_ptr);
}

void load_score() {
  int i;
  FILE *read_ptr = fopen("scoreboard.txt", "r");
  if (!read_ptr)
    return;
  fseek(read_ptr, 0, SEEK_END);
  long fsize = ftell(read_ptr);
  fseek(read_ptr, 0, SEEK_SET);
  char Buffer[fsize + 1];
  fread(Buffer, fsize, 1, read_ptr);
  char *buffer = strdup(Buffer);
  fclose(read_ptr);
  buffer[fsize] = 0;
  char *segment;
  for (i = 0; i < 10; i++) {
    segment = strsep(&buffer, "|");
    if (segment == NULL) {
      goto error;
    }
    segment = strsep(&buffer, ":");
    if (segment != NULL) {
      shared.SAVE_TIME_MIN[i] = atoi(segment);
    } else {
      goto error;
    }
    segment = strsep(&buffer, ".");
    if (segment != NULL) {
      shared.SAVE_TIME_S[i] = atoi(segment);
    } else {
      goto error;
    }
    segment = strsep(&buffer, "|");
    if (segment != NULL) {
      shared.SAVE_TIME_MS[i] = atoi(segment);
    } else {
      goto error;
    }
    segment = strsep(&buffer, "|");
    if (segment != NULL) {
      shared.SAVE_STEPS[i] = atoi(segment);
    } else {
      goto error;
    }
    segment = strsep(&buffer, "\n");
    if (segment != NULL) {
      shared.SAVE_PLAYER[i] = trim_whitespace(segment);
    } else {
      goto error;
    }
  }
  return;
error:
  fprintf(stderr, "Failed to load save data from scoreboard.txt, the file may be damaged.\n"
                  "To get rid of this error, delete scoreboard.txt manually.");
  exit(-1);
}

void restart(Widget w, void *data) {
  int area;
  init_data();
  SetLabel(shared.LABEL[0], "    [Time] 00:00.0");
  for (area = 0; area < 16; area++) {
    fill_drawarea_from_shared(shared.DRAWAREA, 8, area);
  }
}

void scoreboard(Widget w, void *data) {
  char title[] = "ScoreBoard";
  char data_temp[64];
  MakeWindow(title, SAME_DISPLAY, EXCLUSIVE_WINDOW);
  Widget q[14];

  q[0] = MakeLabel("\n");
  for (int i = 0; i < 10; i++) {
    sprintf(data_temp,
            "  %02d | Time %02d:%02d.%d | %03d steps | %.12s  \n",
            i + 1,
            shared.SAVE_TIME_MIN[i],
            shared.SAVE_TIME_S[i],
            shared.SAVE_TIME_MS[i],
            shared.SAVE_STEPS[i],
            shared.SAVE_PLAYER[i]);
    q[i + 1] = MakeLabel(data_temp);
  }
  q[11] = MakeLabel("\n");
  q[12] = MakeLabel("              ");
  q[13] = MakeButton(" Close ", closewindow, NULL);
  q[14] = MakeLabel("\n");

  for (int i = 1; i < 13; i++) {
    SetWidgetPos(q[i], PLACE_UNDER, q[i - 1], NO_CARE, NULL);
  }
  SetWidgetPos(q[13], PLACE_RIGHT, q[12], PLACE_UNDER, q[11]);
  SetWidgetPos(q[14], PLACE_UNDER, q[13], NO_CARE, NULL);

  MainLoop();
}

void about(Widget w, void *data) {
  char title[] = "About";
  MakeWindow(title, SAME_DISPLAY, EXCLUSIVE_WINDOW);
  Widget q[6];
  q[0] = MakeLabel("\n\n   Source: TundraWork/MemoryGame   \n");
  q[1] = MakeLabel("\n      Code available on Github      \n");
  q[2] = MakeLabel("\n             MIT License             \n\n");
  q[3] = MakeLabel("            ");
  q[4] = MakeButton(" Close ", closewindow, NULL);
  q[5] = MakeLabel("\n");
  int Fgcolor = get_rgb_color(40, 0, 150);
  SetFgColor(q[1], Fgcolor);
  SetFgColor(q[2], Fgcolor);

  SetWidgetPos(q[1], PLACE_UNDER, q[0], NO_CARE, NULL);
  SetWidgetPos(q[2], PLACE_UNDER, q[1], NO_CARE, NULL);
  SetWidgetPos(q[3], PLACE_UNDER, q[2], NO_CARE, NULL);
  SetWidgetPos(q[4], PLACE_RIGHT, q[3], PLACE_UNDER, q[3]);
  SetWidgetPos(q[5], PLACE_UNDER, q[4], NO_CARE, NULL);
  ShowDisplay();
  MainLoop();
}

void closewindow(Widget w, void *data) { CloseWindow(); }

int compare_score(int i) {
  int time_new, time_save;
  time_new = shared.TIME_MIN * 600 + shared.TIME_S * 10 + shared.TIME_MS;
  time_save = shared.SAVE_TIME_MIN[i] * 600 + shared.SAVE_TIME_S[i] * 10 + shared.SAVE_TIME_MS[i];
  return (time_save == 0 || time_new < time_save);
}

void choose_end() {
  int i;
  for (i = 0; i < 10; i++) {
    if (compare_score(i)) {
      shared.SCOREBOARD_RANK = i;
      end_a();
      return;
    } else {
      if (i == 9) {
        end_b();
      }
    }
  }
}

void generate_scoreboard() {
  int i = shared.SCOREBOARD_RANK, j;
  for (j = 9; j > i; j--) {
    shared.SAVE_TIME_MIN[j] = shared.SAVE_TIME_MIN[j - 1];
    shared.SAVE_TIME_S[j] = shared.SAVE_TIME_S[j - 1];
    shared.SAVE_TIME_MS[j] = shared.SAVE_TIME_MS[j - 1];
    shared.SAVE_STEPS[j] = shared.SAVE_STEPS[j - 1];
    shared.SAVE_PLAYER[j] = shared.SAVE_PLAYER[j - 1];
  }
  shared.SAVE_TIME_MIN[i] = shared.TIME_MIN;
  shared.SAVE_TIME_S[i] = shared.TIME_S;
  shared.SAVE_TIME_MS[i] = shared.TIME_MS;
  shared.SAVE_STEPS[i] = shared.STEPS;
  shared.SAVE_PLAYER[i] = shared.PLAYER;
  save_score();
  exit(0);
}

void end_a() {
  char title[] = "You Win!";
  char timeis[80];
  int Fgcolor;
  MakeWindow(title, SAME_DISPLAY, EXCLUSIVE_WINDOW);
  Widget q[5];
  sprintf(timeis,
          "\n            You Used : %02d:%02d.%d (%d Steps)\n",
          shared.TIME_MIN,
          shared.TIME_S,
          shared.TIME_MS,
          shared.STEPS);
  q[0] = MakeLabel(timeis);
  q[1] = MakeLabel("\n    Enter your name(Up to 12 letters)and press Enter:    \n\n");
  q[2] = MakeLabel("             ");
  q[3] = MakeStringEntry(NULL, 200, end_a_enter, NULL);
  q[4] = MakeLabel("\n");

  Fgcolor = get_rgb_color(40, 0, 150);
  SetFgColor(q[1], Fgcolor);
  Fgcolor = get_rgb_color(255, 0, 0);
  SetFgColor(q[0], Fgcolor);
  SetWidgetPos(q[1], PLACE_UNDER, q[0], NO_CARE, NULL);
  SetWidgetPos(q[2], PLACE_UNDER, q[1], NO_CARE, NULL);
  SetWidgetPos(q[3], PLACE_RIGHT, q[2], PLACE_UNDER, q[1]);
  SetWidgetPos(q[4], PLACE_UNDER, q[3], NO_CARE, NULL);

  ShowDisplay();
  MainLoop();
}

void end_b() {
  char title[] = "Try Again!";
  char time_new[64];
  Widget q[5];

  MakeWindow(title, SAME_DISPLAY, EXCLUSIVE_WINDOW);

  sprintf(time_new,
          "\n        Your Used : %02d:%02d.%d (%d Steps)       \n\n",
          shared.TIME_MIN,
          shared.TIME_S,
          shared.TIME_MS,
          shared.STEPS);
  q[0] = MakeLabel("\n          You are not fast enough!              \n");
  q[1] = MakeLabel(time_new);
  q[2] = MakeLabel("\n");

  int Fgcolor;
  Fgcolor = get_rgb_color(40, 0, 150);
  SetFgColor(q[1], Fgcolor);
  Fgcolor = get_rgb_color(255, 0, 0);
  SetFgColor(q[0], Fgcolor);

  SetWidgetPos(q[1], PLACE_UNDER, q[0], NO_CARE, NULL);
  SetWidgetPos(q[2], PLACE_UNDER, q[1], NO_CARE, NULL);

  ShowDisplay();
  MainLoop();
}

void end_a_enter(Widget w, char *string, void *data) {
  shared.PLAYER = string;
  generate_scoreboard();
  CloseWindow();
}

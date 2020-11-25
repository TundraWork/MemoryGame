#include <stdint.h>
#include <libsx.h>

typedef struct SharedData {
  uint16_t BITMAP_SIZE_X, BITMAP_SIZE_Y;    /*save the size of bmp*/
  uint8_t *BITMAP_DATA;                     /*save the bmp*/
  Widget DRAWAREA;                          
  Widget LABEL[2];                          /*save some part's Widget*/
  uint8_t MOUSE_MOVE_AREA;                  /*save the id of the grid where the mouse is located*/
  uint8_t CARD_OPENED_1;                    /*save the id of the card first opened*/
  uint8_t CARD_OPENED_2;                    /*save the id of the card second opened*/
  uint8_t CARD_ID[16];                      /*connect the id of the card with the id of the grid*/
  uint8_t CARD_STATUS[16];                  /*card status,0 to closed and 1 to opened*/
  uint8_t CARD_OPENED;                      /*save the the number of cards opened*/
  uint8_t CARD_MATCHED;                     /*save the the number of cards matched*/
  uint8_t TIMING;                           /*the switch of timing(),0 to stop and 1 to run*/
  uint8_t SLEEPING;                         /*save whether the program is sleeping*/
  uint8_t SCOREBOARD_RANK;                  /*save where should the new score be in the scoreboard*/

  uint8_t TIME_MIN;                         /*save how many minutes has been used*/
  uint8_t TIME_S;                           /*save how many seconds has been used*/
  uint8_t TIME_MS;                          /*save how many 0.1seconds has been used*/
  uint16_t STEPS;                           /*save how many steps has been used*/
  char *PLAYER;                             /*save the player's id*/

  uint8_t SAVE_TIME_MIN[10];                /*save minutes readed from scorebord.txt,from no.1 to no.10*/
  uint8_t SAVE_TIME_S[10];                  /*save seconds readed from scorebord.txt,from no.1 to no.10*/
  uint8_t SAVE_TIME_MS[10];                 /*save 0.1seconds readed from scorebord.txt,from no.1 to no.10*/
  uint16_t SAVE_STEPS[10];                  /*save steps readed from scorebord.txt,from no.1 to no.10*/
  char *SAVE_PLAYER[10];                    /*save the player's id readed from scorebord.txt,from no.1 to no.10*/
} SharedData;

/* protos */
int main(int argc, char **argv);
void init_display(int argc, char **argv);

#ifndef _MAIN
#define _MAIN
#define GRID_WIDTH 175
#define GRID_HEIGHT 210
#define FLOP_BACK_TIMEOUT 750
#define TRUE 1
#define FALSE 0
#endif

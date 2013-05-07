#ifndef __DA_GAME_UI_H__
#define __DA_GAME_UI_H__
/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <SDL/SDL.h>   /* All SDL apps need this */
#include "../lib/game_client.h"
#include "../lib/types.h"

typedef enum { 
  TEAMA_S=0, TEAMB_S, FLOOR_S, REDWALL_S, GREENWALL_S, LOGO_S, JACKHAMMER_S, REDFLAG_S, GREENFLAG_S, NUM_S 
} SPRITE_INDEX;

struct UI_Struct {
  SDL_Surface *screen;
  int32_t depth;
  int32_t tile_h;
  int32_t tile_w;

  struct Sprite {
    SDL_Surface *img;
  } sprites[NUM_S];

  uint32_t red_c;
  uint32_t green_c;
  uint32_t blue_c;
  uint32_t white_c;
  uint32_t black_c;
  uint32_t yellow_c;
  uint32_t purple_c;
  uint32_t orange_c;
  uint32_t isle_c;
  uint32_t home_red_c;
  uint32_t jail_c;
  uint32_t home_blue_c;
  uint32_t wall_teama_c;
  uint32_t wall_teamb_c;
  uint32_t player_teama_c;
  uint32_t player_teamb_c;
  uint32_t flag_teama_c;
  uint32_t flag_teamb_c;
  uint32_t jackhammer_c;
};

typedef struct UI_Struct UI;

char map [201][201];

sval ui_zoom(UI *ui, Client * my_client, int fac);
sval ui_pan(UI *ui, Client * my_client, sval xdir, sval ydir);
void ui_paint_it_black(UI *ui);
sval ui_move(UI *ui, sval xdir, sval ydir);
sval ui_keypress(UI *ui, SDL_KeyboardEvent *e, Client* my_client);
void ui_update(UI *ui);
void ui_quit(UI *ui);
void ui_main_loop(UI *ui, uval h, uval w, Client* my_client);
void ui_init(UI **ui);


// RPC calls based on key presses by user
int ui_left(Request *request,Client* my_client);
int ui_right(Request *request,Client* my_client); 
int ui_down(Request *request,Client* my_client);
int ui_up(Request *request,Client* my_client);
int ui_normal(Request *request,Client* my_client);
int ui_pickup_flag(Request *request,Client* my_client);
int ui_pickup_shovel(Request *request,Client* my_client);
int ui_drop_flag(Request *request,Client* my_client);
int ui_drop_shovel(Request *request,Client* my_client);
int ui_join(Request *request,Client* my_client);

//Paint the map
sval ui_paintmap(UI *ui,Maze* maze);

extern void paint_old_pixel(UI* ui, Maze* maze,  int x, int y);
extern void paint_new_pixel(UI *ui, Maze* maze, int x, int y, int color);
#endif

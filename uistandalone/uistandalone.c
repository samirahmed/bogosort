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

#include <stdio.h>
#include <stdlib.h> /* for exit() */
#include <pthread.h>
#include <assert.h>
#include "types2.h"
#include "ui_types.h"
#include "uistandalone.h"
#include "../lib/types.h"
#include "../lib/game_commons.h"

/* A lot of this code comes from http://www.libsdl.org/cgi/docwiki.cgi */

/* Forward declaration of some dummy player code */
static void dummyPlayer_init(UI *ui);
static void dummyPlayer_paint(UI *ui, SDL_Rect *t);


#define SPRITE_H 3
#define SPRITE_W 3

#define UI_FLOOR_BMP "floor.bmp"
#define UI_REDWALL_BMP "redwall.bmp"
#define UI_GREENWALL_BMP "greenwall.bmp"
#define UI_TEAMA_BMP "teama.bmp"
#define UI_TEAMB_BMP "teamb.bmp"
#define UI_LOGO_BMP "logo.bmp"
#define UI_REDFLAG_BMP "redflag.bmp"
#define UI_GREENFLAG_BMP "greenflag.bmp"
#define UI_JACKHAMMER_BMP "shovel.bmp"



//move this to wherever the shallow copy is located
//char map [201][201];
Maze * map_ptr;
Maze maze;
Plist red_players;
Plist blue_players;
Player p;


int init_mapload = 0;
typedef enum {UI_SDLEVENT_UPDATE, UI_SDLEVENT_QUIT} UI_SDL_Event;


struct UI_Player_Struct {
  SDL_Surface *img;
  uval base_clip_x;
  SDL_Rect clip;
};
typedef struct UI_Player_Struct UI_Player;

static inline SDL_Surface *
ui_player_img(UI *ui, int team)
{  
  return (team == 0) ? ui->sprites[TEAMA_S].img 
    : ui->sprites[TEAMB_S].img;
}

static inline sval 
pxSpriteOffSet(int team, int state)
{
  if (state == 1)
    return (team==0) ? SPRITE_W*1 : SPRITE_W*2;
  if (state == 2) 
    return (team==0) ? SPRITE_W*2 : SPRITE_W*1;
  if (state == 3) return SPRITE_W*3;
  return 0;
}

static sval
ui_uip_init(UI *ui, UI_Player **p, int id, int team)
{
  UI_Player *ui_p;
  
  ui_p = (UI_Player *)malloc(sizeof(UI_Player));
  if (!ui_p) return 0;

  ui_p->img = ui_player_img(ui, team);
  ui_p->clip.w = SPRITE_W; ui_p->clip.h = SPRITE_H; ui_p->clip.y = 0;
  ui_p->base_clip_x = id * SPRITE_W * 4;

  *p = ui_p;

  return 1;
}

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static uint32_t 
ui_getpixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;
  
  switch (bpp) {
  case 1:
    return *p;
  case 2:
    return *(uint16_t *)p;
  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
      return p[0] << 16 | p[1] << 8 | p[2];
    else
      return p[0] | p[1] << 8 | p[2] << 16;
  case 4:
    return *(uint32_t *)p;
  default:
    return 0;       /* shouldn't happen, but avoids warnings */
  } // switch
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
static void 
ui_putpixel(SDL_Surface *surface, int x, int y, uint32_t pixel)
 {
   int bpp = surface->format->BytesPerPixel;
   /* Here p is the address to the pixel we want to set */
   uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

   switch (bpp) {
   case 1:
	*p = pixel;
	break;
   case 2:
     *(uint16_t *)p = pixel;
     break;     
   case 3:
     if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
       p[0] = (pixel >> 16) & 0xff;
       p[1] = (pixel >> 8) & 0xff;
       p[2] = pixel & 0xff;
     }
     else {
       p[0] = pixel & 0xff;
       p[1] = (pixel >> 8) & 0xff;
       p[2] = (pixel >> 16) & 0xff;
     }
     break;
 
   case 4:
     *(uint32_t *)p = pixel;
     break;
 
   default:
     break;           /* shouldn't happen, but avoids warnings */
   } // switch
 }

static 
sval splash(UI *ui)
{
  SDL_Rect r;
  SDL_Surface *temp;


  temp = SDL_LoadBMP(UI_LOGO_BMP);
  
  if (temp != NULL) {
    ui->sprites[LOGO_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    r.h = ui->sprites[LOGO_S].img->h;
    r.w = ui->sprites[LOGO_S].img->w;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
    //    printf("r.h=%d r.w=%d r.x=%d r.y=%d\n", r.h, r.w, r.x, r.y);
    SDL_BlitSurface(ui->sprites[LOGO_S].img, NULL, ui->screen, &r);
  } else {
    /* Map the color yellow to this display (R=0xff, G=0xFF, B=0x00)
       Note:  If the display is palettized, you must set the palette first.
    */
    r.h = 40;
    r.w = 80;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
 
    /* Lock the screen for direct access to the pixels */
    if ( SDL_MUSTLOCK(ui->screen) ) {
      if ( SDL_LockSurface(ui->screen) < 0 ) {
	fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
	return -1;
      }
    }
    SDL_FillRect(ui->screen, &r, ui->yellow_c);

    if ( SDL_MUSTLOCK(ui->screen) ) {
      SDL_UnlockSurface(ui->screen);
    }
  }
  /* Update just the part of the display that we've changed */
  SDL_UpdateRect(ui->screen, r.x, r.y, r.w, r.h);

  SDL_Delay(1000);
  return 1;
}


static sval
load_sprites(UI *ui) 
{
  SDL_Surface *temp;
  sval colorkey;

  /* setup sprite colorkey and turn on RLE */
  // FIXME:  Don't know why colorkey = purple_c; does not work here???
  colorkey = SDL_MapRGB(ui->screen->format, 255, 0, 255);
  
  temp = SDL_LoadBMP(UI_TEAMA_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading teama.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[TEAMA_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[TEAMA_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_TEAMB_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading teamb.bmp: %s\n", SDL_GetError()); 
    return -1;
  }
  ui->sprites[TEAMB_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);

  SDL_SetColorKey(ui->sprites[TEAMB_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);
  temp = SDL_LoadBMP(UI_FLOOR_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading floor.bmp %s\n", SDL_GetError()); 
    return -1;
  }
  ui->sprites[FLOOR_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[FLOOR_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_REDWALL_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading redwall.bmp: %s\n", SDL_GetError());
    return -1;
  }
  ui->sprites[REDWALL_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[REDWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_GREENWALL_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading greenwall.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[GREENWALL_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[GREENWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_REDFLAG_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[REDFLAG_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[REDFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_GREENFLAG_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[GREENFLAG_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[GREENFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_JACKHAMMER_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading %s: %s", UI_JACKHAMMER_BMP, SDL_GetError()); 
    return -1;
  }
  ui->sprites[JACKHAMMER_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[JACKHAMMER_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);
  
  return 1;
}

inline static void
draw_cell(UI *ui, SPRITE_INDEX si, SDL_Rect *t, SDL_Surface *s)
{
  SDL_Surface *ts=NULL;
  
  uint32_t tc;

  ts = ui->sprites[si].img;

  if ( ts && t->h == SPRITE_H && t->w == SPRITE_W) 
    SDL_BlitSurface(ts, NULL, s, t);
}
/*
static sval 
ui_initmap(){
FILE *fp;
fp = fopen("../daGame.map", "r");
int i,j;
int map_char;
for(i = 0; i < 200; i++){
   for(j = 0; j < 200; j++){
 
    map_char = fgetc(fp);
   printf("loading character %c", map_char);
    map[i][j] = map_char; 		
		}
	}

}
*/



//helper function for ui_put pixel
//paints SPRITE_W x SPRITE_H pixels starting with x,y at the upper left corner
static void ui_putnpixel(SDL_Surface *surface, int x, int y, uint32_t pixel){
	int w,h;
	for(h = y; h < y+SPRITE_H; h++){
		for(w = x; w < x+SPRITE_W; w++){
			
			ui_putpixel(surface, h, w, pixel);
		
		}
	}
} 





static sval
ui_paintmap(UI *ui) 
{
printf("mapload is %d\n", init_mapload);
int w,h;
int map_char;

int type; 
printf("cell types set");

if(!init_mapload){  
	player_init(&p);

	p.client_position.x = 10; //initialize the client position
        p.client_position.y = 10;
        maze_build_from_file(&maze, "../daGame.map");
	plist_init(&red_players, TEAM_RED,2);
        plist_init(&blue_players, TEAM_BLUE, 2);
	red_players.at[0] = p;
	maze.players[0] = red_players;	
	maze.players[1] = blue_players;
       	maze.get[10][10].player = &p;
	maze.get[10][10].cell_state = CELLSTATE_OCCUPIED;	
        int i,j;
	map_ptr = &maze;
	init_mapload = 1;
}

//init player at location 0


Cell cur_cell;
int cell_state;
int x,y;
SDL_Rect t;

  y = 0; 
  x = 0;
  int scale_x, scale_y;
  t.x = 0;
  t.y = 0;

  for (x = 0; x < 200; x++) {
    for (y = 0; y < 200; y++) {
        cur_cell = maze.get[y][x];
        // get the cell
               
        scale_x = x * SPRITE_W;
	scale_y = y * SPRITE_H;
        type = cur_cell.type;	
if(cur_cell.cell_state == CELLSTATE_EMPTY){

        if(type == CELL_FLOOR){
		ui_putnpixel(ui->screen, scale_x, scale_y, ui-> isle_c);
 			}
	if(type == CELL_WALL && y < 100){
		ui_putnpixel(ui->screen, scale_x, scale_y, ui-> wall_teama_c);
 			}
	if(type == CELL_WALL && y >= 100){ 
		ui_putnpixel(ui->screen, scale_x, scale_y, ui-> wall_teamb_c);
			}
	//for now paint home areas white and jail areas yellow
       	if(type == CELL_JAIL){
		ui_putnpixel(ui->screen, scale_x, scale_y, ui->yellow_c);
			}

	if(type == CELL_HOME){
		ui_putnpixel(ui->screen, scale_x, scale_y, ui-> white_c);
			}
	}
	else{
		
		if(cur_cell.cell_state ==  CELLSTATE_OCCUPIED){
			//if((*(cur_cell.player)).team == TEAM_RED){
			  ui_putnpixel(ui->screen, scale_x, scale_y, ui->yellow_c); // paint yellow for now -- will have to change colors	
				}//else{
				//on the blue team
				
				//}
			//}
			else{
			if(cur_cell.cell_state == CELLSTATE_HOLDING){
				//PRINT OBJECT
				}
				else{
				//PRINT PLAYER HOLDING


					}

				}		
			}
		}
	}


  dummyPlayer_paint(ui, &t);

  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);
 
 return 1;
}

static sval
ui_init_sdl(UI *ui, int32_t h, int32_t w, int32_t d)
{

  fprintf(stderr, "UI_init: Initializing SDL.\n");

  /* Initialize defaults, Video and Audio subsystems */
  if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==-1)) { 
    fprintf(stderr, "Could not initialize SDL: %s.\n", SDL_GetError());
    return -1;
  }

  atexit(SDL_Quit);

  fprintf(stderr, "ui_init: h=%d w=%d d=%d\n", h, w, d);

  ui->depth = d;
  ui->screen = SDL_SetVideoMode(w, h, ui->depth, SDL_SWSURFACE);
  if ( ui->screen == NULL ) {
    fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n", w, h, ui->depth, 
	    SDL_GetError());
    return -1;
  }
    
  fprintf(stderr, "UI_init: SDL initialized.\n");


  if (load_sprites(ui)<=0) return -1;

  ui->black_c      = SDL_MapRGB(ui->screen->format, 0x00, 0x00, 0x00);
  ui->white_c      = SDL_MapRGB(ui->screen->format, 0xff, 0xff, 0xff);
  ui->red_c        = SDL_MapRGB(ui->screen->format, 0xff, 0x00, 0x00);
  ui->green_c      = SDL_MapRGB(ui->screen->format, 0x00, 0xff, 0x00);
  ui->yellow_c     = SDL_MapRGB(ui->screen->format, 0xff, 0xff, 0x00);
  ui->purple_c     = SDL_MapRGB(ui->screen->format, 0xff, 0x00, 0xff);
  
  
  ui->isle_c         = ui->black_c;
  ui->wall_teama_c   = ui->red_c;
  ui->wall_teamb_c   = ui->green_c;
  ui->player_teama_c = ui->red_c;
  ui->player_teamb_c = ui->green_c;
  ui->flag_teama_c   = ui->white_c;
  ui->flag_teamb_c   = ui->white_c;
  ui->jackhammer_c   = ui->yellow_c;
  
 
  /* set keyboard repeat */
  SDL_EnableKeyRepeat(70, 70);  

  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

  splash(ui);
  return 1;
}

static void
ui_shutdown_sdl(void)
{
  fprintf(stderr, "UI_shutdown: Quitting SDL.\n");
  SDL_Quit();
}

static sval
ui_userevent(UI *ui, SDL_UserEvent *e) 
{
  if (e->code == UI_SDLEVENT_UPDATE) return 2;
  if (e->code == UI_SDLEVENT_QUIT) return -1;
  return 0;
}

static sval
ui_process(UI *ui)
{
  SDL_Event e;
  sval rc = 1;

  while(SDL_WaitEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT:
      return -1;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      rc = ui_keypress(ui, &(e.key));
      break;
    case SDL_ACTIVEEVENT:
      break;
    case SDL_USEREVENT:
      rc = ui_userevent(ui, &(e.user));
      break;
    default:
      fprintf(stderr, "%s: e.type=%d NOT Handled\n", __func__, e.type);
    }
    if (rc==2) { 
	
	ui_paintmap(ui); }
    if (rc<0) break;
  }
  return rc;
}

extern sval
ui_zoom(UI *ui, sval fac)
{
  
  //zoom the ui by a factor fac
  //SPRITE_H = SPRITE_H * fac;
  //SPRITE_W = SPRITE_W * fac; 
  //init__mapload = 0;
  fprintf(stderr, "%s:\n", __func__);
  return 2;
}

extern sval
ui_pan(UI *ui, sval xdir, sval ydir)
{
  fprintf(stderr, "%s:\n", __func__);
  return 2;
}

extern sval
ui_move(UI *ui, sval xdir, sval ydir)
{
  fprintf(stderr, "%s:\n", __func__);
  return 1;
}


extern void
ui_update(UI *ui)
{
  SDL_Event event;
  
  event.type      = SDL_USEREVENT;
  event.user.code = UI_SDLEVENT_UPDATE;
  SDL_PushEvent(&event);

}


extern void
ui_quit(UI *ui)
{  
  SDL_Event event;
  fprintf(stderr, "ui_quit: stopping ui...\n");
  event.type      = SDL_USEREVENT;
  event.user.code = UI_SDLEVENT_QUIT;
  SDL_PushEvent(&event);
}

extern void
ui_main_loop(UI *ui, uval h, uval w)
{
  sval rc;
  
  assert(ui);

  ui_init_sdl(ui, h, w, 32);

 //dummyPlayer_init(ui);
    ui_paintmap(ui);
   
  
  while (1) {
    if (ui_process(ui)<0) break;
  }

  ui_shutdown_sdl();
}


extern void
ui_init(UI **ui)
{
  *ui = (UI *)malloc(sizeof(UI));
  if (ui==NULL) return;

  bzero(*ui, sizeof(UI));
  
  (*ui)->tile_h = SPRITE_H;
  (*ui)->tile_w = SPRITE_W;

}


// Kludgy dummy player for testing purposes
struct DummyPlayerDesc {
  UI_Player *uip;
  int id;
  int x, y;
  int team;
  int state;
} dummyPlayer;

/*static void 
dummyPlayer_init(UI *ui) 
{
  dummyPlayer.id = 0;
  dummyPlayer.x = 5; dummyPlayer.y = 5; dummyPlayer.team = 0; dummyPlayer.state = 0;
  ui_uip_init(ui, &dummyPlayer.uip, dummyPlayer.id, dummyPlayer.team); 
}*/

static void 
dummyPlayer_paint(UI *ui, SDL_Rect *t)
{
  //t -> y = 1;
  //t -> x = 1;
  //printf("y is %d\n", t->y);
  //printf("x is %d\n", t->x); 
  //printf("dummyplayer y is %d\n", dummyPlayer.y);
  //printf("dummyplayer x is %d\n", dummyPlayer.x);
  

  ui_putnpixel(ui->screen, dummyPlayer.y, dummyPlayer.x, ui-> jackhammer_c);
  //t->y = dummyPlayer.y * t->h; 
  //t->x = dummyPlayer.x * t->w;
  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);
 
  //dummyPlayer.uip->clip.x = dummyPlayer.uip->base_clip_x +
   // pxSpriteOffSet(dummyPlayer.team, dummyPlayer.state);
  //SDL_BlitSurface(dummyPlayer.uip->img, &(dummyPlayer.uip->clip), ui->screen, t);
}

int
ui_dummy_left(UI *ui)
{ 
//NOTE TO SELF:
//IF THE FOLLOWING ISN"T PRINTING ON RUN MAKE SURE YOURE RUNNING XMONAD 
   int x,y, new_x;
   x = map_ptr->players[0].at[0].client_position.x;
   y = map_ptr->players[0].at[0].client_position.y;
   new_x = x-1;
   if(maze.get[new_x][y].type == CELL_WALL){
	printf("Cell wall, cannot move in that direction\n");
	}
   else{
   printf("player after moving at %d, %d\n", new_x,y); 
   map_ptr->players[0].at[0].client_position.x = new_x;
   (maze.get[new_x][y].player) = (maze.get[y][x].player);
   maze.get[x][y].player = NULL;
   maze.get[x][y].cell_state = CELLSTATE_EMPTY;
   printf("x is %d, new_x is %d\n", x, new_x);
   printf("cell state is %d\n", maze.get[new_x][y].cell_state);
   maze.get[new_x][y].cell_state = CELLSTATE_OCCUPIED;
}   
return 2;
}

int
ui_dummy_right(UI *ui)
{
    int x,y, new_x;
   x = map_ptr->players[0].at[0].client_position.x;
   y = map_ptr->players[0].at[0].client_position.y;
   new_x = x+1;
   if(maze.get[new_x][y].type == CELL_WALL){
	printf("Cell wall, cannot move in that direction\n");
	}
   else{
   printf("player after moving at %d, %d\n", new_x,y); 
   map_ptr->players[0].at[0].client_position.x = new_x;
   (maze.get[new_x][y].player) = (maze.get[y][x].player);
   maze.get[x][y].player = NULL;
   maze.get[x][y].cell_state = CELLSTATE_EMPTY;
   printf("x is %d, new_x is %d\n", x, new_x);
   printf("cell state is %d\n", maze.get[new_x][y].cell_state);
   maze.get[new_x][y].cell_state = CELLSTATE_OCCUPIED;
}   


//send pair of ints to server along with move command
 //if we receive a move player event update then update the dummy player location
 
 return 2;

}


int
ui_dummy_down(UI *ui)
{
    int x,y, new_y;
   x = map_ptr->players[0].at[0].client_position.x;
   y = map_ptr->players[0].at[0].client_position.y;
   new_y = y+1;
   if(maze.get[x][new_y].type == CELL_WALL){
	printf("Cell wall, cannot move in that direction\n");
	}
   else{
   printf("player after moving at %d, %d\n", x,new_y); 
   map_ptr->players[0].at[0].client_position.y = new_y;
   (maze.get[x][new_y].player) = (maze.get[y][x].player);
   maze.get[x][y].player = NULL;
   maze.get[x][y].cell_state = CELLSTATE_EMPTY;
   printf("y is %d, new_y is %d\n", y, new_y);
   printf("cell state is %d\n", maze.get[x][new_y].cell_state);
   maze.get[x][new_y].cell_state = CELLSTATE_OCCUPIED;
}   

 return 2;

}






int
ui_dummy_up(UI *ui)
{
   int x,y, new_y;
   x = map_ptr->players[0].at[0].client_position.x;
   y = map_ptr->players[0].at[0].client_position.y;
   new_y = y-1;
   if(maze.get[x][new_y].type == CELL_WALL){
	printf("Cell wall, cannot move in that direction\n");
	}
   else{
   printf("player after moving at %d, %d\n", x,new_y); 
   map_ptr->players[0].at[0].client_position.y = new_y;
   (maze.get[x][new_y].player) = (maze.get[y][x].player);
   maze.get[x][y].player = NULL;
   maze.get[x][y].cell_state = CELLSTATE_EMPTY;
   printf("y is %d, new_y is %d\n", y, new_y);
   printf("cell state is %d\n", maze.get[x][new_y].cell_state);
   maze.get[x][new_y].cell_state = CELLSTATE_OCCUPIED;
}   

 return 2;
}




int
ui_dummy_normal(UI *ui)
{
  dummyPlayer.state = 0;
  return 2;
}

int
ui_dummy_pickup_red(UI *ui)
{
  dummyPlayer.state = 1;
  return 2;
}

int
ui_dummy_pickup_green(UI *ui)
{
  dummyPlayer.state = 2;
  return 2;
}


int
ui_dummy_jail(UI *ui)
{
  dummyPlayer.state = 3;
  return 2;
}

int
ui_dummy_toggle_team(UI *ui)
{
  if (dummyPlayer.uip) free(dummyPlayer.uip);
  dummyPlayer.team = (dummyPlayer.team) ? 0 : 1;
  ui_uip_init(ui, &dummyPlayer.uip, dummyPlayer.id, dummyPlayer.team);
  return 2;
}

int
ui_dummy_inc_id(UI *ui)
{
  if (dummyPlayer.uip) free(dummyPlayer.uip);
  dummyPlayer.id++;
  if (dummyPlayer.id>=100) dummyPlayer.id = 0;
  ui_uip_init(ui, &dummyPlayer.uip, dummyPlayer.id, dummyPlayer.team);
  return 2;
}

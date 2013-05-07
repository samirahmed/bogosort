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
#include <sys/types.h>
#include <assert.h>
#include "uistandalone.h"
#include "SDL/SDL.h"
#include "../lib/game_commons.h"

/* A lot of this code comes from http://www.libsdl.org/cgi/docwiki.cgi */


#define UI_FLOOR_BMP "client/bitmap/floor.bmp"
#define UI_REDWALL_BMP "client/bitmap/redwall.bmp"
#define UI_GREENWALL_BMP "client/bitmap/greenwall.bmp"
#define UI_TEAMA_BMP "client/bitmap/teama.bmp"
#define UI_TEAMB_BMP "client/bitmap/teamb.bmp"
#define UI_LOGO_BMP "client/bitmap/logo.bmp"
#define UI_REDFLAG_BMP "client/bitmap/redflag.bmp"
#define UI_GREENFLAG_BMP "client/bitmap/greenflag.bmp"
#define UI_JACKHAMMER_BMP "client/bitmap/shovel.bmp"
#define SPRITE_H 5
#define SPRITE_W 5

int zoom_level = 1;
int pan_offset_x = 0;
int pan_offset_y = 0;
int map_h;
int map_w;
int init_mapload = 0;
typedef enum {UI_SDLEVENT_UPDATE, UI_SDLEVENT_QUIT} UI_SDL_Event;


struct UI_Player_Struct
{
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

    switch (bpp)
    {
    case 1:
        *p = pixel;
        break;
    case 2:
        *(uint16_t *)p = pixel;
        break;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        }
        else
        {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(uint32_t *)p = pixel;
        break;

    default:
        break;       /* shouldn't happen, but avoids warnings */
    } // switch
}

static
sval splash(UI *ui)
{
    SDL_Rect r;
    SDL_Surface *temp;


    temp = SDL_LoadBMP(UI_LOGO_BMP);

    if (temp != NULL)
    {
        ui->sprites[LOGO_S].img = SDL_DisplayFormat(temp);
        SDL_FreeSurface(temp);
        r.h = ui->sprites[LOGO_S].img->h;
        r.w = ui->sprites[LOGO_S].img->w;
        r.x = ui->screen->w/2 - r.w/2;
        r.y = ui->screen->h/2 - r.h/2;
        //  printf("r.h=%d r.w=%d r.x=%d r.y=%d\n", r.h, r.w, r.x, r.y);
        SDL_BlitSurface(ui->sprites[LOGO_S].img, NULL, ui->screen, &r);
    }
    else
    {
        /* Map the color yellow to this display (R=0xff, G=0xFF, B=0x00)
        Note:  If the display is palettized, you must set the palette first.
        */
        r.h = 40;
        r.w = 80;
        r.x = ui->screen->w/2 - r.w/2;
        r.y = ui->screen->h/2 - r.h/2;

        /* Lock the screen for direct access to the pixels */
        if ( SDL_MUSTLOCK(ui->screen) )
        {
            if ( SDL_LockSurface(ui->screen) < 0 )
            {
                fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
                return -1;
            }
        }
        SDL_FillRect(ui->screen, &r, ui->yellow_c);

        if ( SDL_MUSTLOCK(ui->screen) )
        {
            SDL_UnlockSurface(ui->screen);
        }
    }
    /* Update just the part of the display that we've changed */
    SDL_UpdateRect(ui->screen, r.x, r.y, r.w, r.h);

    SDL_Delay(1000);
    return 1;
}


static sval load_sprites(UI *ui)
{
    SDL_Surface *temp;
    sval colorkey;

    /* setup sprite colorkey and turn on RLE */
    // FIXME:  Don't know why colorkey = purple_c; does not work here???
    colorkey = SDL_MapRGB(ui->screen->format, 255, 0, 255);

    temp = SDL_LoadBMP(UI_TEAMA_BMP);
    if (temp == NULL)
    {
        fprintf(stderr, "ERROR: loading teama.bmp: %s", SDL_GetError());
        return -1;
    }
    ui->sprites[TEAMA_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    SDL_SetColorKey(ui->sprites[TEAMA_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    colorkey);

    temp = SDL_LoadBMP(UI_TEAMB_BMP);
    if (temp == NULL)
    {
        fprintf(stderr, "ERROR: loading teamb.bmp: %s\n", SDL_GetError());
        return -1;
    }
    ui->sprites[TEAMB_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);

    SDL_SetColorKey(ui->sprites[TEAMB_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    colorkey);
    temp = SDL_LoadBMP(UI_FLOOR_BMP);
    if (temp == NULL)
    {
        fprintf(stderr, "ERROR: loading floor.bmp %s\n", SDL_GetError());
        return -1;
    }
    ui->sprites[FLOOR_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    SDL_SetColorKey(ui->sprites[FLOOR_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    colorkey);

    temp = SDL_LoadBMP(UI_REDWALL_BMP);
    if (temp == NULL)
    {
        fprintf(stderr, "ERROR: loading redwall.bmp: %s\n", SDL_GetError());
        return -1;
    }
    ui->sprites[REDWALL_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    SDL_SetColorKey(ui->sprites[REDWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    colorkey);

    temp = SDL_LoadBMP(UI_GREENWALL_BMP);
    if (temp == NULL)
    {
        fprintf(stderr, "ERROR: loading greenwall.bmp: %s", SDL_GetError());
        return -1;
    }
    ui->sprites[GREENWALL_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    SDL_SetColorKey(ui->sprites[GREENWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    colorkey);

    temp = SDL_LoadBMP(UI_REDFLAG_BMP);
    if (temp == NULL)
    {
        fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError());
        return -1;
    }
    ui->sprites[REDFLAG_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    SDL_SetColorKey(ui->sprites[REDFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    colorkey);

    temp = SDL_LoadBMP(UI_GREENFLAG_BMP);
    if (temp == NULL)
    {
        fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError());
        return -1;
    }
    ui->sprites[GREENFLAG_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    SDL_SetColorKey(ui->sprites[GREENFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
                    colorkey);

    temp = SDL_LoadBMP(UI_JACKHAMMER_BMP);
    if (temp == NULL)
    {
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
    ts = ui->sprites[si].img;

    if ( ts && t->h == SPRITE_H && t->w == SPRITE_W)
        SDL_BlitSurface(ts, NULL, s, t);
}


//helper function for ui_put pixel
//paints SPRITE_W x SPRITE_H pixels starting with x,y at the upper left corner
static void ui_putnpixel(SDL_Surface *surface, int x, int y, uint32_t pixel)
{
    int w,h;
    int tw, th; // translated w and h
    for(h = y; h < y+SPRITE_H; h++)
    {
        for(w = x; w < x+SPRITE_W; w++)
        {
		tw = w + pan_offset_x;
		th = h + pan_offset_y;
		if(0 <= th && th < map_h && 0 <= tw && tw < map_w){
			ui_putpixel(surface, th, tw, pixel);
		}
        }
    }
}


extern sval
ui_paintmap(UI *ui,Maze* maze)
{
    if (proto_debug()) fprintf(stderr,"mapload is %d\n", init_mapload);

    int type;
    Team_Types turf;
    if (proto_debug()) fprintf(stderr,"cell types set");

    Cell cur_cell;
    int x,y;

    y = 0;
    x = 0;
    int scale_x, scale_y;

    for (x = 0; x < 200; x++)
    {
        for (y = 0; y < 200; y++)
        {
            cur_cell = maze->get[y][x];

            scale_x = x * zoom_level;
            scale_y = y * zoom_level;
            type = cur_cell.type;
            turf = cur_cell.turf;
            if(cur_cell.cell_state == CELLSTATE_EMPTY)
            {
                if(type == CELL_FLOOR)
                {
                    ui_putnpixel(ui->screen, scale_x, scale_y, ui-> isle_c);
                }
                if(type == CELL_WALL && turf == TEAM_RED)
                {
                    ui_putnpixel(ui->screen, scale_x, scale_y, ui-> wall_teama_c);
                }
                if(type == CELL_WALL && turf == TEAM_BLUE)
                {
                    ui_putnpixel(ui->screen, scale_x, scale_y, ui-> wall_teamb_c);
                }
                //for now paint home areas white and jail areas yellow
                if(type == CELL_JAIL)
                {
                    ui_putnpixel(ui->screen, scale_x, scale_y, ui->jail_c);
                }

                if(type == CELL_HOME && turf == TEAM_RED )
                {
                    ui_putnpixel(ui->screen, scale_x, scale_y, ui->home_red_c);
                }
                else if(type == CELL_HOME && turf == TEAM_BLUE )
                {
                    ui_putnpixel(ui->screen, scale_x, scale_y, ui->home_blue_c);
                }
            }
            else
            {

                if(cur_cell.cell_state ==  CELLSTATE_OCCUPIED)
                {
                    if (maze->client_player &&  maze->client_player == cur_cell.player)
                      ui_putnpixel(ui->screen, scale_x, scale_y, ui->yellow_c);
                    else if((*(cur_cell.player)).team == TEAM_RED)
                      ui_putnpixel(ui->screen, scale_x, scale_y, ui->player_teama_c); 
                    else if((*(cur_cell.player)).team == TEAM_BLUE)
                      ui_putnpixel(ui->screen, scale_x, scale_y, ui->player_teamb_c);
                      
                }
                else
                {
                    if(cur_cell.cell_state == CELLSTATE_HOLDING)
                    {
                        //PRINT OBJECT
                        //orange if jackhammer/shovel
                        //purple if flag
                        if(cur_cell.object->type == OBJECT_SHOVEL)
                        {
                            ui_putnpixel(ui->screen, scale_x, scale_y, ui-> purple_c);
                        }
                        else
                        {
                            ui_putnpixel(ui->screen, scale_x, scale_y, ui->orange_c);
                        }
                    }

                }
            }
        }
    }

    SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);

    return 1;
}

static sval
ui_init_sdl(UI *ui, int32_t h, int32_t w, int32_t d)
{

    map_h = h;
    map_w = w;
    if (proto_debug()) fprintf(stderr, "UI_init: Initializing SDL.\n");

    /* Initialize defaults, Video and Audio subsystems */
    if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==-1))
    {
        fprintf(stderr, "Could not initialize SDL: %s.\n", SDL_GetError());
        return -1;
    }

    atexit(SDL_Quit);

    if (proto_debug()) fprintf(stderr, "ui_init: h=%d w=%d d=%d\n", h, w, d);

    ui->depth = d;
    ui->screen = SDL_SetVideoMode(w, h, ui->depth, SDL_SWSURFACE);
    if ( ui->screen == NULL )
    {
        fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n", w, h, ui->depth,
                SDL_GetError());
        return -1;
    }


    if (load_sprites(ui)<=0) return -1;

    ui->black_c   = SDL_MapRGB(ui->screen->format, 0x27, 0x28, 0x22);
    ui->white_c   = SDL_MapRGB(ui->screen->format, 0xf8, 0xf8, 0xf2);
    ui->red_c     = SDL_MapRGB(ui->screen->format, 0xf9, 0x26, 0x72);
    ui->blue_c    = SDL_MapRGB(ui->screen->format, 0x57, 0xb4, 0xd1);
    ui->yellow_c  = SDL_MapRGB(ui->screen->format, 0xe6, 0xdb, 0x74);
    ui->purple_c  = SDL_MapRGB(ui->screen->format, 0xae, 0x81, 0xff);
    ui->green_c   = SDL_MapRGB(ui->screen->format, 0x7f, 0xe2, 0x2e);
    ui->jail_c    = SDL_MapRGB(ui->screen->format, 0x75, 0x71, 0x5e);
    ui->home_red_c  = SDL_MapRGB(ui->screen->format, 0x50, 0x10, 0x27);
    ui->home_blue_c = SDL_MapRGB(ui->screen->format, 0x25, 0x53, 0x33);
    ui->orange_c  = SDL_MapRGB(ui->screen->format, 0xff, 0xff, 0xff);
    ui->wall_teama_c   = SDL_MapRGB(ui->screen->format, 0x77, 0x08, 0x30);
    ui->wall_teamb_c   = SDL_MapRGB(ui->screen->format, 0x0b, 0x66, 0x83);

    ui->isle_c         = ui->black_c;
    ui->player_teama_c = ui->red_c;
    ui->player_teamb_c = ui->blue_c;
    ui->flag_teama_c   = ui->white_c;
    ui->flag_teamb_c   = ui->white_c;
    ui->jackhammer_c   = ui->yellow_c;

    fprintf(stdout, COLOR_OKGREEN "[%dx%d] UI Initialized" COLOR_END "\n",w,h);
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
ui_process(UI *ui, Client* my_client)
{
    SDL_Event e;
    sval rc = 1;

    while(SDL_WaitEvent(&e))
    {
        switch (e.type)
        {
        case SDL_QUIT:
            return -1;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            rc = ui_keypress(ui, &(e.key),my_client);
            break;
        case SDL_ACTIVEEVENT:
            break;
        case SDL_USEREVENT:
            rc = ui_userevent(ui, &(e.user));
            break;
        default:
            fprintf(stderr, "%s: e.type=%d NOT Handled\n", __func__, e.type);
        }

        if (rc==-1) break;
    }
    return rc;
}




extern sval
ui_zoom(UI *ui, Client *c, int fac)
{
    if(fac == 1){
	    if(zoom_level > 1){

		    ui_paint_it_black(ui);
		    zoom_level--;
	    }
	 else{
	    printf("Cannot zoom in any farther");
	 }
    }
    else{
	    if(zoom_level < SPRITE_H){
		    zoom_level++;
	    }
	    else{
	    printf("Cannot zoom out any farther");
	    }
    }
    fprintf(stderr, "%s:\n", __func__);
    ui_paintmap(ui, &c->maze);
      return 2;

}

//paint the cells black for panning and zooming
void ui_paint_it_black(UI *ui){
	int x,y;
	int scale_x, scale_y;
    for (x = 0; x < 200; x++)
    {
        for (y = 0; y < 200; y++){
            scale_x = x * zoom_level;
            scale_y = y * zoom_level;
            ui_putnpixel(ui->screen, scale_x, scale_y, ui-> black_c);
	}
    }
}

extern sval
ui_pan(UI *ui, Client *c,  sval xdir, sval ydir)
{
	//guaranteed to only have xdir, ydir input as
	// (1,0) (-1,0), (0,-1), (0,1)

       ui_paint_it_black(ui);
	if(xdir == 1){
		pan_offset_x+=3;
	}
	
	if(xdir == -1){
		pan_offset_x-=3;
	}
	
	if(ydir == 1){
		pan_offset_y+=3;
	}
	if(ydir == -1){

		pan_offset_y-=3;
	}

    ui_paintmap(ui, &c->maze);
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

    event.type    = SDL_USEREVENT;
    event.user.code = UI_SDLEVENT_UPDATE;
    SDL_PushEvent(&event);

}


extern void ui_quit(UI *ui)
{
    SDL_Event event;
    fprintf(stderr, "ui_quit: stopping ui...\n");
    event.type    = SDL_USEREVENT;
    event.user.code = UI_SDLEVENT_QUIT;
    SDL_PushEvent(&event);
}

extern void  ui_main_loop(UI *ui, uval h, uval w,Client* my_client)
{
    ui_init_sdl(ui, h, w, 32);
    ui_paintmap(ui, &my_client->maze);

    while (1)
    {
        if (ui_process(ui,my_client)==-1)
            break;
    }

    ui_shutdown_sdl();
}


extern void ui_init(UI **ui)
{
    *ui = (UI *)malloc(sizeof(UI));
    if (ui==NULL) return;

    bzero(*ui, sizeof(UI));

    (*ui)->tile_h = SPRITE_H;
    (*ui)->tile_w = SPRITE_W;

}


int ui_left(Request *request,Client* my_client)
{
    int rc,x,y, new_x;
    x = my_client->my_player->client_position.x;
    y = my_client->my_player->client_position.y;
    new_x = x-1;

    Pos next;
    bzero(&next,sizeof(next));

    if(my_client->maze.get[new_x][y].type == CELL_WALL)
    {
        printf("Cell wall, cannot move in that direction\n");
        rc = -999;//I will change this later
    }
    else
    {
        next.x = new_x;
        next.y = y;
        request_action_init(request,my_client,ACTION_MOVE,
                            &my_client->my_player->client_position,&next);
        rc = doRPCCmd(request);
        process_RPC_message(my_client);
    }
    return rc;
}

int ui_right(Request *request,Client* my_client)
{
    int rc,x,y, new_x;
    x = my_client->my_player->client_position.x;
    y = my_client->my_player->client_position.y;
    new_x = x+1;

    Pos next;
    bzero(&next,sizeof(next));

    if(my_client->maze.get[new_x][y].type == CELL_WALL)
    {
        printf("Cell wall, cannot move in that direction\n");
        rc = -999;//I will change this later
    }
    else
    {
        next.x = new_x;
        next.y = y;
        request_action_init(request,my_client,ACTION_MOVE,
                            &my_client->my_player->client_position,&next);
        rc = doRPCCmd(request);
        process_RPC_message(my_client);
    }

    return rc;

}


int ui_down(Request *request,Client* my_client)
{
    int rc,x,y, new_y;
    x = my_client->my_player->client_position.x;
    y = my_client->my_player->client_position.y;
    new_y = y+1;

    Pos next;
    bzero(&next,sizeof(next));

    if(my_client->maze.get[x][new_y].type == CELL_WALL)
    {
        printf("Cell wall, cannot move in that direction\n");
        rc = -999;
    }
    else
    {
        next.x = x;
        next.y = new_y;
        request_action_init(request,my_client,ACTION_MOVE,
                            &my_client->my_player->client_position,&next);
        rc = doRPCCmd(request);
        process_RPC_message(my_client);
    }

    return rc;

}


int ui_up(Request *request,Client* my_client)
{
    int rc,x,y, new_y;
    x = my_client->my_player->client_position.x;
    y = my_client->my_player->client_position.y;
    new_y = y-1;

    Pos next;
    bzero(&next,sizeof(next));

    if(my_client->maze.get[x][new_y].type == CELL_WALL)
    {
        printf("Cell wall, cannot move in that direction\n");
        rc = -999;
    }
    else
    {
        next.x = x;
        next.y = new_y;
        request_action_init(request,my_client,ACTION_MOVE,
                            &my_client->my_player->client_position,&next);
        rc = doRPCCmd(request);
        process_RPC_message(my_client);

    }

    return rc;
}

int  ui_join(Request *request,Client* my_client)
{
    int rc;
    request_hello_init(request,my_client);
    rc = doRPCCmd(request);
    if(rc==0)
        process_RPC_message(my_client);
    return rc;
}

int  ui_drop_flag(Request *request,Client* my_client)
{
    int rc;
    Pos * pos = &my_client->my_player->client_position;
    request_action_init(request,my_client,ACTION_DROP_FLAG,pos,pos);
    rc = doRPCCmd(request);
    if(rc==0)
        process_RPC_message(my_client);
    return rc;
}

int  ui_drop_shovel(Request *request,Client* my_client)
{
    int rc;
    Pos * pos = &my_client->my_player->client_position;
    request_action_init(request,my_client,ACTION_DROP_SHOVEL,pos,pos);
    rc = doRPCCmd(request);
    if(rc==0)
        process_RPC_message(my_client);
    return rc;
}

int  ui_pickup_flag(Request *request,Client* my_client)
{
    int rc;
    Pos * pos = &my_client->my_player->client_position;
    request_action_init(request,my_client,ACTION_PICKUP_FLAG,pos,pos);
    rc = doRPCCmd(request);
    if(rc==0)
        process_RPC_message(my_client);
    return rc;
}

int  ui_pickup_shovel(Request *request,Client* my_client)
{
    int rc;
    Pos * pos = &my_client->my_player->client_position;
    request_action_init(request,my_client,ACTION_PICKUP_SHOVEL,pos,pos);
    rc = doRPCCmd(request);
    if(rc==0)
        process_RPC_message(my_client);
    return rc;
}

extern sval
ui_keypress(UI *ui, SDL_KeyboardEvent *e,Client* my_client)
{
    SDLKey sym = e->keysym.sym;
    SDLMod mod = e->keysym.mod;
    Request request;
    bzero(&request,sizeof(Request));
    int rc;
    if (e->type == SDL_KEYDOWN)
    {
        if (sym == SDLK_LEFT && mod != KMOD_SHIFT)
        {
            if (proto_debug()) printf("move left\n");
            if (proto_debug() ) fprintf(stderr, "%s: move left\n", __func__);
            rc = ui_left(&request,my_client);
            return rc;
        }
        if (sym == SDLK_RIGHT && mod != KMOD_SHIFT)
        {
            if (proto_debug() ) fprintf(stderr, "%s: move right\n", __func__);
            rc = ui_right(&request,my_client);
            return rc;
        }
        if (sym == SDLK_UP && mod != KMOD_SHIFT)
        {
            if (proto_debug() )fprintf(stderr, "%s: move up\n", __func__);
            rc = ui_up(&request,my_client);
            return rc;
        }
        if (sym == SDLK_DOWN && mod != KMOD_SHIFT)
        {
            if (proto_debug() )fprintf(stderr, "%s: move down\n", __func__);
            rc = ui_down(&request,my_client);
            return rc;
        }
        if (sym == SDLK_r && mod != KMOD_SHIFT)
        {
            if (proto_debug() )fprintf(stderr, "%s: pickup flag\n", __func__);
            if(my_client->my_player->flag==NULL)
                rc = ui_pickup_flag(&request,my_client);
            else
                rc = ui_drop_flag(&request,my_client);
            return rc;
        }
        if (sym == SDLK_g && mod != KMOD_SHIFT)
        {
            if (proto_debug() )fprintf(stderr, "%s: pickup shovel\n", __func__);
            if(my_client->my_player->shovel==NULL)
                rc = ui_pickup_shovel(&request,my_client);
            else
                rc = ui_drop_shovel(&request,my_client);
            return rc;
        }
        if (sym == SDLK_j && mod != KMOD_SHIFT)
        {
            if (proto_debug() )fprintf(stderr, "%s: pickup shovel\n", __func__);
            rc = ui_join(&request,my_client);
            process_RPC_message(my_client);
            return rc;
        }
        if (sym == SDLK_q) return -1;
        if (sym == SDLK_z) return ui_zoom(ui, my_client, 1);
        if (sym == SDLK_x) return ui_zoom(ui, my_client, -1); 
	if (sym == SDLK_w) return ui_pan(ui, my_client, -1, 0);
	if (sym == SDLK_s) return ui_pan(ui, my_client, 1, 0);
	if (sym == SDLK_a) return ui_pan(ui, my_client, 0, -1);
	if (sym == SDLK_d) return ui_pan(ui, my_client, 0, 1);
        else
        {
           if (proto_debug() ) fprintf(stderr, "%s: key pressed: %d\n", __func__, sym);
        }
    }
    else
    { 
          if (proto_debug() )  fprintf(stderr, "%s: key released: %d\n", __func__, sym);
    }
    return 1;
}

#ifndef __DAGAME_PLAYER_OBJECT_H__
#define __DAGAME_PLAYER_OBJECT_H__
/******************************************************************************
* Copyright (C) 2013 by Katsu Kawakami, Will Seltzer, Samir Ahmed, 
* Boston University
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
#define MAX_TEAM_SIZE 175
#define MAX_PLAYERS   350

#include "net.h"

typedef struct{
    struct Player *        player;
    struct Cell *          object;
    Pos             position;
    Object_Types    type;
    Team_Types      team;
    Visible_Types   visiblity;
} Object;

typedef struct{
    struct Cell*        cell;
    Object*             shovel;
    Object*             flag;
    Player_State_Types  state;
    Team_Types          team;
    unsigned short int  id;
    FDType              fd;
} Player;

typedef struct{
    Player              at[MAX_TEAM_SIZE];
    int                 count;
    int                 unique;
    Team_Types          team;
    pthread_mutex_t     plist_wrlock;
} Plist;

extern void player_init(Player * player);
extern void player_drop(Player * player);
extern int player_has_shovel(Player * player);
extern int player_has_flag(Player * player);

extern void plist_init(Plist * plist);
extern void plist_player_count( Plist * plist );
extern void plist_add_player(Plist* plist, Player * player);
extern void plist_drop_player_using_fd(Plist* plist, FDType fd );
extern void plist_drop_player_using_id(Plist* plist, int id );

#endif

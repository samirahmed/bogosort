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
#include "types.h"

#define MAX_TEAM_SIZE 175
#define MAX_PLAYERS   350
#define MAX_COL_MAZE  1000
#define MAX_ROW_MAZE  1000
#define MAX_COLUMNS   200
#define MAX_ROWS      200
#define NUM_TEAMS     2
#define NUM_OBJECTS   4

/* Common Game Types */
typedef struct{
    unsigned short int  x;
    unsigned short int  y;
} Pos;

typedef struct{
    struct Player * player;
    struct Cell *   object;
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
    int                 fd;
} Player;

typedef struct{
    Pos              pos;
    Cell_Types       type;
    Cell_State_Types cell_state;
    Team_Types       turf;
    Mutable_Types    is_mutable;
    Player*          player;
    Object*          object;
    pthread_mutex_t  lock;
} Cell;

typedef struct{
    Player              at[MAX_TEAM_SIZE];
    int                 count;
    int                 next;
    int                 max;
    Team_Types          team;
    pthread_rwlock_t    plist_wrlock;
} Plist;

typedef struct{
    Pos              min;
    Pos              max;
    Team_Types       team;
    pthread_mutex_t  jail_recursive_lock;
} Jail;

typedef struct{
    Pos              min;
    Pos              max;
    pthread_rwlock_t count_wrlock;
    int              count;
    Team_Types       team;
} Home;

typedef struct{
    Pos              min;
    Pos              max;
    Cell**           get;
    int**            wall;
    Object           objects[NUM_OBJECTS];
    Plist            players[NUM_TEAMS];
    Jail             jail[NUM_TEAMS];
    Home             home[NUM_TEAMS];
    pthread_rwlock_t wall_wrlock;
    pthread_rwlock_t object_wrlock;
} Maze;

// JAIL METHODS
extern void jail_init(Jail*jail, Pos min, Pos max, Team_Types team);

// HOME METHODS
extern void home_init(Home * home, Pos min, Pos max, Team_Types team );

// PLIST METHODS
extern void plist_init(Plist * plist, Team_Types team, int max_player_size);

// PLAYER METHODS
extern void player_init(Player * player);
extern int player_has_shovel(Player * player);
extern int player_has_flag(Player * player);

// CELL METHODS
extern void cell_init(Cell* cell,int x, int y, Team_Types turf, Cell_Types type, Mutable_Types is_mutable);
extern int  cell_is_unoccupied(Cell* cell);
extern int  cell_is_walkable_type(Cell * cell);
extern void cell_lock(Cell*);
extern void cell_unlock(Cell*);

// Maze Construction
extern void maze_init(Maze * m,int max_x, int max_y);
extern void maze_dump(Maze*map);
extern void maze_destroy(Maze*map);
extern int  maze_build_from_file(Maze*map, char* filename);

#endif

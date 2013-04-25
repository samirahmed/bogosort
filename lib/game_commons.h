#ifndef __DAGAME_GAME_COMMONS_H__
#define __DAGAME_GAME_COMMONS_H__
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

#define MAX_TEAM_SIZE 200
#define MAX_PLAYERS   400
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

// forward declarations
struct CellStruct;
struct PlayerStruct;
typedef struct CellStruct* cell_t;
typedef struct PlayerStruct* player_t;

typedef struct{
    cell_t          cell;
    player_t        player;
    Object_Types    type;
    Team_Types      team;
    Visible_Types   visiblity;
    
    Pos             client_position;
    int             client_player_id;
    Team_Types      client_player_team;
    int             client_has_player;
    pthread_mutex_t lock;
    unsigned int    thread;
} Object;

typedef struct PlayerStruct{
    cell_t              cell;
    Object*             shovel;
    Object*             flag;
    Player_State_Types  state;
    Team_Types          team;
    unsigned short int  id;
    int                 fd;
    Pos                 client_position;
    pthread_mutex_t     lock;
    unsigned int        thread;
} Player;

typedef struct CellStruct{
    Pos              pos;
    Cell_Types       type;
    Cell_State_Types cell_state;
    Team_Types       turf;
    Mutable_Types    is_mutable;
    Player*          player;
    Object*          object;
    pthread_mutex_t  lock;
    unsigned int     thread;
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
    Game_State_Types current_game_state;
    pthread_mutex_t  state_lock;
    int              last_team;
} Maze;

// UTIL METHODS
extern Team_Types opposite_team(Team_Types team);

// OBJECT METHODS
extern Object* object_get(Maze*m, Object_Types object,Team_Types team);
extern int     object_get_index(Team_Types team , Object_Types object);

// JAIL METHODS
extern void jail_init(Jail*jail, Pos min, Pos max, Team_Types team);

// HOME METHODS
extern void home_init(Home * home, Pos min, Pos max, Team_Types team );
extern int  home_contains(Pos* query, Home*home);

// PLIST METHODS
extern void plist_init(Plist * plist, Team_Types team, int max_player_size);

// MAZE METHODS
extern void maze_set_state(Maze*m,Game_State_Types state);
extern Game_State_Types  maze_get_state(Maze*m);

// PLAYER METHODS
extern void player_init(Player* player);
extern int player_get_position(Player * player, Pos*pos);

// CELL METHODS
extern void cell_init(Cell* cell,int x, int y, Team_Types turf, Cell_Types type, Mutable_Types is_mutable);
extern int  cell_is_unoccupied(Cell* cell);
extern int  cell_is_holding(Cell* cell);
extern int  cell_is_walkable_type(Cell * cell);
extern void cell_lock(Cell*);
extern void cell_unlock(Cell*);
extern int cell_is_near(Cell* current, Cell* next);

// Maze Construction
extern void maze_init(Maze * m,int max_x, int max_y);
extern void maze_dump(Maze*map);
extern void maze_destroy(Maze*map);
extern int  maze_build_from_file(Maze*map, char* filename);

// Marshalling and Compression
extern int decompress_is_ignoreable(int * compressed);

extern void compress_player(Player* player, int* compressed , Player_Update_Types type);
extern void decompress_player(Player* player, int* compressed, Player_Update_Types *type);

extern void compress_object(Object* object, int* compressed );
extern void decompress_object(Object* object, int* compressed );

extern void compress_game_state(Maze* object, int* compressed);
extern int decompress_game_state(Game_State_Types* gstate, int* compressed);
extern void compress_broken_wall(Pos * position, int* compressed);
extern void decompress_broken_wall(Pos * position, int* compressed);

#endif

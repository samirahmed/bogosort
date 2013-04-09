#ifndef __DAGAME_MAZE_H__
#define __DAGAME_MAZE_H__
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
#include "./types.h" 
#include "./game_pieces.h" 

#define MAX_COL_MAZE  200
#define MAX_ROW_MAZE  200
#define NUM_TEAMS     2
#define NUM_OBJECTS   4


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

// Maze Construction
extern void maze_init(Maze * m,int max_x, int max_y);
extern void maze_dump(Maze*map);
extern int  maze_build_from_file(Maze*map, char* filename);

// Locking Methods
extern void maze_lock(Maze*m, Pos current, Pos next);
extern void maze_unlock(Maze*m, Pos current, Pos next);
//extern void jail_lock(Maze*m, Team_Types team);

// Maze Manipulation
extern void maze_update_player_position( Maze*m, Player* player, Cell* NextCell);
extern void maze_update_object_cell( Maze*m, Object* object, Player* player );
extern void maze_update_object_player( Maze*m, Object* object, Player* player );
extern void maze_move_player( Maze*m, Cell* current, Cell* next);
extern void maze_jail_player( Maze*m, Cell* current, Cell* next);
extern void maze_jailbreak( Maze*m, Team_Types team );
extern void maze_jailbreak( Maze*m, Team_Types team );
extern void maze_use_shovel( Maze*m, Cell* current, Cell* next);
extern void maze_object_drop_pickup( Maze*m, Action action, Cell* current, Cell* next);
extern void maze_spawn_player(Maze* m, Player* p);
extern void maze_reset_shovel(Maze* m, Object* object);


//extern void cell_unmarshall_from_header(Cell * cell, Proto_Msg_Hdr *hdr);
//extern void cell_marshall_into_header(Cell * cell, Proto_Msg_Hdr * hdr);

#endif

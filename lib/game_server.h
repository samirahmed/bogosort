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
#include "./game_commons.h" 

// Locking Methods
extern void server_maze_lock(Maze*m, Pos current, Pos next);
extern void server_maze_unlock(Maze*m, Pos current, Pos next);
extern void server_jail_lock(Jail * jail );
extern void server_jail_unlock(Jail * jail);
extern void server_object_unlock(Maze*m);
extern void server_object_read_lock(Maze*m);
extern void server_object_write_lock(Maze*m);

//extern void jail_lock(Maze*m, Team_Types team);

// Maze Manipulation
extern void maze_update_player_position( Maze*m, Player* player, Cell* NextCell);
extern void maze_update_object_cell( Maze*m, Object* object, Player* player );
extern void maze_update_object_player( Maze*m, Object* object, Player* player );
extern void maze_move_player( Maze*m, Cell* current, Cell* next);
extern void maze_use_shovel( Maze*m, Cell* current, Cell* next);
extern void maze_object_drop_pickup( Maze*m, Action_Types action, Cell* current, Cell* next);
extern void maze_spawn_player(Maze* m, Player* p);
extern void maze_reset_shovel(Maze* m, Object* object);

// JAIL METHODS
extern void server_jail_player( Maze*m, Cell* current, Cell* next);

// PLAYER METHODS
extern void player_drop(Player * player);


// HOME METHODS
extern void server_hash_id( Maze* m, int key, Cell** cell, Team_Types team);
extern int  server_find_empty_home_cell_and_lock(Maze*m, Team_Types team, Cell** cell ,int id, int query);
extern int  server_home_count_increment(Home * home);
extern int  server_home_count_decrement(Home * home);
extern int  server_home_count_read(Home * home);

// PLIST METHODS
extern void server_plist_player_count( Plist * plist );
extern void server_plist_add_player(Plist* plist, Player * player);
extern void server_plist_drop_player_using_fd(Plist* plist, int fd );
extern void server_plist_drop_player_using_id(Plist* plist, int id );

// ACTION METHODS

extern int _server_action_drop_flag(Maze*m , Player* player);
extern int _server_action_drop_shovel(Maze*m , Player*player);
extern int _server_action_player_reset_shovel(Maze*m, Player*player);
extern int _server_action_pickup_object(Maze*m, Player* player);
extern int _server_action_update_cell(Maze*m, Object* object, Cell* newcell );
extern int _server_action_update_cell_and_player(Maze*m, Object* object, Cell* newcell, Player* player);
extern int _server_action_update_player(Maze*m, Player*player, Cell*newcell);
extern int _server_action_move_player(Maze*m, Cell* currentcell , Cell* nextcell );
extern int _server_action_jailbreak( Maze*m, Team_Types team );
extern int _server_action_jail_player(Maze*m, Cell* currentcell);

#endif

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

typedef struct{
  Player player_a;
  Player player_b;
  int    compress_player_a;
  int    compress_player_b;
  Pos    broken_wall;
  int    wall_break;
} EventUpdate;

typedef struct{
  Pos          pos; 
  Team_Types   team; 
  int          id; 
  int          fd; 
  Action_Types action;
  int          test_mode;
  EventUpdate  update;
} GameRequest;

// Request Methods
extern int server_request_init(Maze*m,GameRequest*request,int fd,Action_Types action, int pos_x, int pos_y);
extern int  server_fd_to_id_and_team(Maze*m,int fd, int *team_ptr, int*id_ptr);

extern int* server_request_plist(Maze*m, Team_Types team, int* length);
extern void server_request_objects(Maze*m,int*rshovel,int*rflag,int*bshovel,int*bflag);
extern int* server_request_walls(Maze*m, int* length);

// Game Methods
extern int  server_game_add_player(Maze*maze,int fd, Player**player);
extern void server_game_drop_player(Maze*maze,int team, int id);
extern int  server_game_action(Maze*maze , GameRequest* request);
extern int  server_validate_player( Maze*m, Team_Types team, int id , int fd );
extern int _server_game_wall_move(Maze*m,Player*player, Cell*current, Cell*next);
extern int _server_game_floor_move(Maze*m, Player*player, Cell*current, Cell*next);
extern int _server_game_state_update(Maze*m, Player*player, Cell*current, Cell*next);
extern int _server_game_move(Maze*m, Player*player, Cell* current, Cell*next);

// Locking Methods
extern void server_maze_property_unlock(Maze*m);
extern void server_maze_property_lock(Maze*m);
extern void server_maze_lock(Maze*m, Pos current, Pos next);
extern void server_maze_unlock(Maze*m, Pos current, Pos next);
extern int  server_maze_lock_by_player(Maze*m, Player*player , Pos * next );
extern void server_object_unlock(Maze*m);
extern void server_object_read_lock(Maze*m);
extern void server_object_write_lock(Maze*m);
extern void server_plist_read_lock(Plist*plist);
extern void server_plist_write_lock(Plist*plist);
extern void server_plist_unlock(Plist*plist);
extern void _server_root_lock(Maze*m, Cell*c);    // requires cell to be locked before calling
extern void _server_root_unlock(Maze*m, Cell*c);  // requires cell to be locked before calling
extern void server_wall_read_lock(Maze*m);
extern void server_wall_write_lock(Maze*m);
extern void server_wall_unlock(Maze*m);

// Object Methods
extern void object_lock(Object*object);
extern void object_unlock(Object*object);

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
extern void server_jail_hash( Maze* m, int key, Cell** cell, Team_Types team);
extern int  server_find_empty_jail_cell_and_lock(Maze*m, Team_Types team, Cell** cell ,int id, int query);
extern void server_jail_player( Maze*m, Cell* current, Cell* next);
extern void server_jail( Maze* m, int key, Cell** cell, Team_Types team);

// PLAYER METHODS
extern void player_drop(Player * player);
extern void player_init(Player * player);
extern void player_lock(Player*player);
extern void player_unlock(Player*player);
extern int  player_has_shovel(Player * player);
extern int  player_has_flag(Player * player);
extern int  server_player_spawn(Maze*m, Player * player);

// HOME METHODS
extern void server_home_hash( Maze* m, int key, Cell** cell, Team_Types team);
extern int  server_find_empty_home_cell_and_lock(Maze*m, Team_Types team, Cell** cell ,int id, int query);
extern int  server_home_count_increment(Home * home);
extern int  server_home_count_decrement(Home * home);
extern int  server_home_count_read(Home * home);

// PLIST METHODS
extern int  server_plist_player_count( Plist * plist );
extern int  server_plist_player_count_increment( Plist * plist );
extern int  server_plist_player_count_decrement( Plist * plist );
extern int  server_plist_add_player(Plist* plist, int fd);
extern int  server_plist_find_player_by_fd(Plist*plist, int fd);
extern int  server_plist_drop_player_by_fd(Maze*m, Plist*plist, int fd);
extern void server_plist_drop_player_by_id(Maze*m, Plist* plist, int id );

////////////////////
// UNSAFE METHODS //
////////////////////

// ACTION METHODS

extern int _server_action_drop_flag(Maze*m , Player* player);
extern int _server_action_drop_shovel(Maze*m , Player*player);
extern int _server_action_player_reset_shovel(Maze*m, Player*player);
extern int _server_action_pickup_object(Maze*m, Player* player);
extern int _server_action_update_cell(Maze*m, Object* object, Cell* newcell );
extern int _server_action_update_cell_and_player(Maze*m, Object* object, Cell* newcell, Player* player);
extern int _server_action_update_player(Maze*m, Player*player, Cell*newcell);
extern int _server_action_move_player(Maze*m, Cell* currentcell , Cell* nextcell );
extern int _server_action_jailbreak( Maze*m, Team_Types team, Cell*current, Cell*next );
extern int _server_action_jail_player(Maze*m, Cell* currentcell);

// OTHERS 
extern void _server_drop_handler(Maze*m, Player*player);
extern void _server_validate_request(Maze*m, Player*player);

#endif

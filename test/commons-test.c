#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/net.h"
#include "../lib/game_commons.h"
#include "../lib/game_server.h"
#include "../lib/test.h"

Maze maze;  // global maze

int test_compression(TestContext *tc)
{
  int assertion,compressed;
  assertion = 1;
  maze_build_from_file(&maze, "test.map");
  
  // PLAYER COMPRESSION
  Player plr, test_plr; 
  // TODO replace with a player init
  bzero(&plr,sizeof(Player));
  plr.id = 55;
  plr.cell = &(maze.get[16][45]);
  plr.team = TEAM_BLUE;
  plr.state = PLAYER_JAILED;
  compress_player(&plr,&compressed,PLAYER_ADDED);

  assertion = !decompress_is_ignoreable(&compressed);
  should(assertion, "correctly set a player's do not ignore bit", tc);
  if (!assertion) return -1;
 
  Player_Update_Types update_type;
  update_type = PLAYER_UNCHANGED;
  decompress_player(&test_plr,&compressed, &update_type);
  assertion = ( plr.team == test_plr.team &&
                plr.state == test_plr.state && 
                plr.cell->pos.x == test_plr.client_position.x &&
                plr.cell->pos.y == test_plr.client_position.y &&
                plr.id == test_plr.id &&
                update_type == PLAYER_ADDED);
  should(assertion,"compress and decompress player objects correctly",tc);
  if (!assertion) return -1;

  // OBJECT COMPRESSION
  Object obj,test_obj;
  obj = maze.objects[object_get_index(TEAM_RED,OBJECT_FLAG)];
  obj.cell = &(maze.get[16][45]);
  compress_object(&obj,&compressed);
  
  assertion = !decompress_is_ignoreable(&compressed);
  should(assertion, "correctly set an object's do not ignore bit", tc);
  if (!assertion) return -1;

  decompress_object(&test_obj,&compressed);
  assertion = ( obj.team == test_obj.team && 
         obj.cell->pos.x == test_obj.client_position.x &&
         obj.cell->pos.y == test_obj.client_position.y &&
         obj.type == test_obj.type && 
         test_obj.client_has_player == 0 );
  should(assertion,"compress and decompress objects WITHOUT a player correctly",tc);
  if (!assertion) return -1;
  
  // OBJECT WITH PLAYER
  obj.player = &plr;
  compress_object(&obj,&compressed);
  decompress_object(&test_obj,&compressed);
  assertion = (  obj.team == test_obj.team && 
                 obj.cell->pos.x == test_obj.client_position.x &&
                 obj.cell->pos.y == test_obj.client_position.y &&
                 obj.type == test_obj.type && 
                 test_obj.client_has_player == 1 && 
                 plr.id == test_obj.client_player_id && 
                 plr.team == test_obj.client_player_team);
  should(assertion,"compress and decompress objects WITH a player correctly",tc);
  if (!assertion) return -1;

  maze_destroy(&maze);
  
  // GAME STATE COMPRESSION + WALL COMPRESSION
  Pos broken,test_pos;
  broken.x = 66;
  broken.y = 102;
  
  Game_State_Types test_state;
  test_state = GAME_STATE_UNCHANGED;
  maze_set_state(&maze, GAME_STATE_RED_WIN);
  
  compress_game_state(&maze, &compressed);
  compress_broken_wall(&broken, &compressed);
  
  assertion = decompress_game_state(&test_state, &compressed);
  assertion = (assertion >=0 ) && (test_state == maze_get_state(&maze));
  should(assertion,"compress and decompress the game state correctly",tc);
  if (!assertion) return -1;
  
  assertion = !decompress_is_ignoreable(&compressed);
  should(assertion, "correctly set an object's do not ignore bit", tc);
  if (!assertion) return -1;
 
  decompress_broken_wall(&test_pos, &compressed);
  assertion = (test_pos.x = broken.x && test_pos.y == broken.y);
  should(assertion, "compress and decompress broken wall positions correctly", tc);
  if (!assertion) return -1;

  return (assertion ? 0 : -1);
}

int test_cell(TestContext *tc)
{
    int rc;
    rc = 1;
    maze_build_from_file(&maze, "test.map");
    int xx,yy;
    
    for(xx = maze.min.x ; xx < maze.max.x ; xx++ )
    {
      for(yy = maze.min.y ; yy < maze.max.y ; yy++ )
      {
        cell_lock(&(maze.get[xx][yy])); 
        rc = ((pthread_mutex_trylock(&(maze.get[xx][yy].lock)) != 0) && rc );
        cell_unlock(&(maze.get[xx][yy]));
      }
    }
    rc = rc!=0;
    should(rc!=0,"lock properly",tc);

    if(rc) return 0;
    else return -1;
}

int test_maze_load(TestContext *tc)
{
    int rc;
    rc = maze_build_from_file(&maze,"test.map");
    should(rc>0,"build from file without errors",tc);
    if (rc<0) return rc;

    rc = (maze.min.x == 0 && maze.min.y == 0) && (maze.max.x == 200 && maze.max.y == 200);
    should(rc,"build with correct dimensions",tc);
    if (!rc) return -1;

    rc = (maze.home[TEAM_RED].min.x == 2   && maze.home[TEAM_RED].min.y == 90) &&
         (maze.home[TEAM_RED].max.x == 12  && maze.home[TEAM_RED].max.y == 109) &&
         (maze.home[TEAM_BLUE].min.x == 188 && maze.home[TEAM_BLUE].min.y == 90) &&
         (maze.home[TEAM_BLUE].max.x == 198 && maze.home[TEAM_BLUE].max.y == 109);
    should(rc,"know where the homebase min and max positions are",tc);
    if (!rc) return rc;
   
    rc = (maze.jail[TEAM_RED].min.x == 90   && maze.jail[TEAM_RED].min.y == 90) &&
         (maze.jail[TEAM_RED].max.x == 98  && maze.jail[TEAM_RED].max.y == 109) &&
         (maze.jail[TEAM_BLUE].min.x == 102 && maze.jail[TEAM_BLUE].min.y == 90) &&
         (maze.jail[TEAM_BLUE].max.x == 110 && maze.jail[TEAM_BLUE].max.y == 109);
    should(rc,"know where the jail min and max positions are",tc);
    if (!rc) return rc;

    rc = (maze.players[TEAM_RED].count == 0 && maze.players[TEAM_BLUE].count == 0) &&
         (maze.players[TEAM_RED].max < 192 && maze.players[TEAM_BLUE].max < 192);
    should(rc,"know successfully initialize the plists",tc);
    if (!rc) return rc;

    maze_destroy(&maze);

    return 1;
}

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
      
    // ADD TESTS HERE
    run(&test_maze_load,"Maze Load",&tc); 
    run(&test_cell,"Cells",&tc); 
    run(&test_compression,"Compression",&tc); 
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}

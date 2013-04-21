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

void test_find_and_lock(TestContext * tc)
{
    int assertion, key, team , xx,yy;
    Maze maze;
    maze_build_from_file(&maze,"test.map");
    
    Cell * cell;
    Cell * test;
    assertion = 1;
    for ( team =0; team<NUM_TEAMS ; team++)
    {
        for ( key = -1000 ; key<1000000 && assertion ;key++ )
        { 
          server_home_hash(&maze, key, &cell, team); 
          assertion = ( cell->pos.x < maze.max.x && cell->pos.x >= maze.min.x &&
                        cell->pos.y < maze.max.y && cell->pos.y >= maze.min.y && assertion);
        }
    }
    should("hash any integer to a valid home position",assertion,tc);
    
    assertion = 1;
    for ( team =0; team<NUM_TEAMS ; team++)
    {
        for ( key = -1000 ; key<1000000 && assertion ;key++ )
        { 
          server_jail_hash(&maze, key, &cell, team); 
          assertion = ( cell->pos.x < maze.max.x && cell->pos.x >= maze.min.x &&
                        cell->pos.y < maze.max.y && cell->pos.y >= maze.min.y && assertion);
        }
    }
    should("hash any integer to a valid jail position",assertion,tc);
    
    for ( team=0; team<NUM_TEAMS; team++)
    {
      key = 173;
      server_home_hash(&maze,key,&cell,team);
      /*fprintf(stderr,"Target is  %d, %d" , cell->pos.x,cell->pos.y );*/
      Home *home = &(maze.home[team]);
      for ( xx = home->min.x ; xx < home->max.x ; xx++)
      { 
        for ( yy = home->min.y ; yy < home->max.y ;yy++)
        {
           if (!(xx == cell->pos.x && yy==cell->pos.y))
           {
              if ( (xx+yy)%3 == 0 )
              { 
                cell_lock(&(maze.get[xx][yy]));
              }
              else 
              {
                maze.get[xx][yy].player=(Player*)1;
                maze.get[xx][yy].object=(Object*)1;
              }
           }
        }
      }
        // now there should be only one position which works
        // query player type
        
        server_find_empty_home_cell_and_lock(&maze,team, &test, 0, 0);
        assertion = (cell->pos.x == test->pos.x && cell->pos.y == test->pos.y && (pthread_mutex_trylock(&cell->lock)!=0) );
        should("correctly find an empty home cell to place a player in team",assertion,tc);
        cell_unlock(cell); 
        
        // query for object type
        server_find_empty_home_cell_and_lock(&maze,team, &test, 0, 1);
        assertion = (cell->pos.x == test->pos.x && cell->pos.y == test->pos.y && (pthread_mutex_trylock(&cell->lock)!=0) );
        should("correctly find an empty home cell to place an object",assertion,tc);
        cell_unlock(cell); 
    }
    
    for ( team=0; team<NUM_TEAMS; team++)
    {
      key = 99;
      server_jail_hash(&maze,key,&cell,team);
      /*fprintf(stderr,"Target is  %d, %d" , cell->pos.x,cell->pos.y );*/
      Jail *jail = &(maze.jail[team]);
      for ( xx = jail->min.x ; xx < jail->max.x ; xx++)
      { 
        for ( yy = jail->min.y ; yy < jail->max.y ;yy++)
        {
           if (!(xx == cell->pos.x && yy==cell->pos.y))
           {
              if ( (xx+yy)%3 == 0 )
              { 
                cell_lock(&(maze.get[xx][yy]));
              }
              else 
              {
                maze.get[xx][yy].player=(Player*)1;
                maze.get[xx][yy].object=(Object*)1;
              }
           }
        }
      }
        // now there should be only one position which works
        // query player type
        
        server_find_empty_jail_cell_and_lock(&maze,team, &test, 0, 0);
        assertion = (cell->pos.x == test->pos.x && cell->pos.y == test->pos.y && (pthread_mutex_trylock(&cell->lock)!=0) );
        should("correctly find an empty jail cell to place a player in team",assertion,tc);
        cell_unlock(cell); 
        
        // query for object type
        server_find_empty_jail_cell_and_lock(&maze,team, &test, 0, 1);
        assertion = (cell->pos.x == test->pos.x && cell->pos.y == test->pos.y && (pthread_mutex_trylock(&cell->lock)!=0) );
        should("correctly find an empty jail cell to place an object",assertion,tc);
        cell_unlock(cell); 
    }

    maze_destroy(&maze);
}


/*********************/
/* SERVER LOCKS TEST */
/*********************/

void st_increment_home(Task*task)
{
  server_home_count_increment((Home*)(task->arg0)); 
}

void st_decrement_home(Task*task)
{
  server_home_count_decrement((Home*)(task->arg0)); 
}

void st_decrement_plist(Task*task)
{
  Maze*m = (Maze*)task->arg0;
  int* team= ((int*) task->arg1);
  server_plist_player_count_decrement(&m->players[*team]); 
}

void st_increment_plist(Task*task)
{
  Maze*m = (Maze*)task->arg0;
  int* team= ((int*) task->arg1);
  server_plist_player_count_increment(&m->players[*team]); 
}

void st_add_drop_player(void*task_ptr)
{
  Task* task = (Task*) task_ptr;
  Maze* m    = (Maze*) task->arg0;
  int*  team = (int*)  task->arg1;
  int*  fd   = (int*)  task->arg2;
  int rc, ii;
  
  for (ii = 0; ii<10;ii++)
  {
      test_nanosleep();
      rc = server_plist_add_player( &m->players[*team] ,*fd );
      if (test_debug()) fprintf(stderr,"%d add       %d)\n",rc,*fd);
      
      test_nanosleep();
      rc = server_plist_drop_player_by_fd( m , &m->players[*team] , *fd );
      if (test_debug()) fprintf(stderr,"%d drop      %d)\n",rc,*fd);
  }

  test_nanosleep();
  rc = server_plist_add_player( &m->players[*team] ,*fd );
  if (test_debug()) fprintf(stderr,"%d add again %d)\n",rc,*fd);

}

void test_plist(TestContext*tc)
{
    Maze maze;
    maze_build_from_file(&maze,"test.map");
   
    ////////////////////////////////
    //  PLIST ADDING AND DROPPING
    ////////////////////////////////
    
    int start, stop, ii, team, rc, assertion,fd;
    team = randint()%2;
    start = 1000;
    stop = 2000;
    Plist*players = &maze.players[team];

    rc = server_plist_find_player_by_fd(players,123);
    assertion = (rc < 0) && (server_plist_player_count(players) == 0);
    should("not have any players when initialized",rc,tc);

    for (ii=0; ii< players->max; ii++ )
    {
      fd = start + ii;
      rc = server_plist_add_player(players,fd);
      assertion = assertion && (rc >=0 );
    }

    assertion = assertion && (server_plist_player_count(players) == players->max );
    should("successfully add players until filled to the brim",assertion,tc);

    rc = server_plist_add_player(players,123);
    assertion = (rc < 0 );
    should("not allow addition of a new player when full",assertion,tc);
  
    assertion = 1;
    for (ii=0;ii<players->max; ii++)
    {
      fd = ii+start;
      rc = server_plist_find_player_by_fd(players,fd);
      assertion = assertion && (rc>=0);
    }
    should("be able to find any player by valid fd",assertion,tc);

    rc = server_plist_find_player_by_fd(players,123);
    assertion = (rc < 0);
    should("indicate failure when searching for fd that doesn't exist",assertion,tc);
    
    rc = server_plist_drop_player_by_fd(&maze, players,123);
    assertion = (rc < 0);
    should("indicate failure when dropping fd that doesn't exist",assertion,tc);

    assertion = 1;
    for (ii=0; ii<players->max; ii++)
    {
      rc = server_plist_drop_player_by_fd(&maze, players,ii+start);
      assertion = assertion && (rc >=0 );
    }
    for (ii=0; ii<players->max; ii++)
    {
      assertion = assertion && (players->at[ii].fd == -1);
    }
    assertion = assertion && (server_plist_player_count(players) == 0);
    should("successfully drop all the players",assertion,tc);

    maze_destroy(&maze);
    maze_build_from_file(&maze,"test.map");
    
    int num_tasks = players->max;
    Task* tasks = malloc(sizeof(Task)*num_tasks);
    int*  fds = malloc(sizeof(int)*num_tasks);
    bzero(tasks,sizeof(Task)*num_tasks);
    bzero(fds,sizeof(int)*num_tasks);
    for (ii=0; ii< num_tasks; ii++)
    {
      fds[ii] = ii + start;
      test_task_init(&tasks[ii],(Proc)&st_add_drop_player,1,&maze,&players->team,&fds[ii],NULL,NULL,NULL);
    }
    parallelize(tasks,num_tasks,1); // run each task one 1 thread only
   
    assertion = 1;
    for (ii=0; ii<players->max; ii++)
    {
      rc = server_plist_find_player_by_fd(players,fds[ii]);
      assertion = assertion && (rc>=0);
    }
    assertion = assertion && ( server_plist_player_count(players) == players->max);
    should("support concurrent add and drops",assertion,tc);

    maze_destroy(&maze);
}



void st_game_add_drop(void*task_ptr)
{
  Task* task = (Task*) task_ptr;
  Maze* m    = (Maze*) task->arg0;
  int*  fd   = (int*)  task->arg1;
  int rc, ii;
  Player*player;

  for (ii = 0; ii<10;ii++)
  {
      test_nanosleep();
      rc = server_game_add_player( m ,*fd, &player );
      if (test_debug()) fprintf(stderr,"%d add       %d, id=%d ,team=%d \n",rc,*fd,player->id, player->team);
      
      if (rc >=0) 
      {
        test_nanosleep();
        server_game_drop_player( m , player->team , player->id );
        if (test_debug()) fprintf(stderr,"%d drop      %d, id=%d ,team=%d \n",rc,*fd,player->id, player->team);
      }
  }

  test_nanosleep();
  rc =  server_game_add_player( m ,*fd , &player);
  if (test_debug()) fprintf(stderr,"%d finally add  %d, id=%d ,team=%d\n",rc,*fd,player->id,player->team);

}

void test_game_add_drop(TestContext * tc)
{
    Maze maze;
    maze_build_from_file(&maze,"test.map");
    
    int team, assertion, num_tasks, start,ii;
    start = 7000;
    num_tasks = 180;

    Task* tasks = malloc(sizeof(Task)*num_tasks);
    int*  fds = malloc(sizeof(int)*num_tasks);
    bzero(tasks,sizeof(Task)*num_tasks);
    bzero(fds,sizeof(int)*num_tasks);
    
    for (ii=0; ii< num_tasks; ii++)
    {
      fds[ii] = ii + start;
      test_task_init(&tasks[ii],(Proc)&st_game_add_drop,1,&maze,&fds[ii],NULL,NULL,NULL,NULL);
    }
    parallelize(tasks,num_tasks,1); // run each task one 1 thread only
   
    assertion =1;
    // Check referential integrety of cells and player
    for (team=0;team<NUM_TEAMS;team++)
    {
      Plist*players = &maze.players[team];
      for( ii=0; ii<players->max ; ii++ )
      {
        Player*player = &players->at[ii];
        if (player->fd != -1)
        {
           Cell*cell = player->cell;
           assertion = assertion && (cell) &&(cell->player = player);
        }
      }
      should("maintain reference integrity",assertion,tc);
    }
    
    assertion = (server_plist_player_count(&maze.players[TEAM_RED]) + 
                server_plist_player_count(&maze.players[TEAM_BLUE])  == num_tasks );
    should("maintain player count correctly",assertion,tc);
    
    maze_destroy(&maze);
}

void test_server_locks(TestContext * tc)
{
    Maze maze;
    maze_build_from_file(&maze,"test.map");
   
    /////////////////
    //  JAIL LOCKS
    /////////////////
  
    ////////////////
    // HOME COUNTER
    ////////////////
    int team, assertion,thread_per_task;
    thread_per_task = 500;
    team = randint()%NUM_TEAMS;
    // Assign a team at random 

    // create tasks
    Task tasks[2];
    bzero(tasks, sizeof(Task)*2 );
    
    test_task_init(&tasks[0],(Proc)&st_increment_home,50,&maze.home[team],NULL,NULL,NULL,NULL,NULL);
    test_task_init(&tasks[1],(Proc)&st_decrement_home,48,&maze.home[team],NULL,NULL,NULL,NULL,NULL);
    parallelize(tasks,2,thread_per_task);
    
    assertion = server_home_count_read(&maze.home[team]) == ((tasks[0].reps-tasks[1].reps)*thread_per_task);
    should("atomically increment and decrement home counter",assertion,tc);
   
    ////////////////
    // PLIST COUNTER
    ////////////////
    
    thread_per_task = 500;
    team = randint()%NUM_TEAMS;
    // Assign a team at random 

    // create tasks
    bzero(tasks, sizeof(Task)*2 );
    
    test_task_init(&tasks[0],(Proc)&st_increment_plist,50,&maze,&team,NULL,NULL,NULL,NULL);
    test_task_init(&tasks[1],(Proc)&st_decrement_plist,48,&maze,&team,NULL,NULL,NULL,NULL);
    parallelize(tasks,2,thread_per_task);
    
    assertion = server_plist_player_count(&maze.players[team]) == ((tasks[0].reps-tasks[1].reps)*thread_per_task);
    should("atomically increment and decrement player counter",assertion,tc);
   
    maze_destroy(&maze);
}

void test_pickup_drop_logic(TestContext*tc)
{
   Maze maze;
   maze_build_from_file(&maze,"test.map");
   
   int rc, fd_blue, assertion, fd_red;
   GameRequest request;
   Player *blue,*red;
   fd_blue = randint()%1000;
   fd_red = fd_blue+1;

   /////////////////
   // PICKUP SHOVEL
   /////////////////
   Object* blue_shovel = object_get(&maze, OBJECT_SHOVEL , TEAM_BLUE);
   server_game_add_player(&maze,fd_blue,&blue);
   server_request_init(&maze,&request,fd_blue,ACTION_MOVE,blue_shovel->cell->pos.x,blue_shovel->cell->pos.y);
   request.test_mode = 1; //  teleport
   server_game_action(&maze,&request);
   server_request_init(&maze,&request,fd_blue,ACTION_PICKUP_SHOVEL,blue->cell->pos.x,blue->cell->pos.y);
   rc = server_game_action(&maze,&request);
   assertion = (rc >= 0) &&
               (blue_shovel->cell = blue->cell) &&
               (blue_shovel->player = blue)     &&
               (blue->shovel == blue_shovel)    &&
               (blue_shovel->cell->object != blue_shovel) &&
               (player_has_shovel(blue) && !player_has_flag(blue));
   should("be picked up by players correctly",assertion,tc);

   ///////////////
   // MOVE WITH OBJECT
   ///////////////
   Pos old = blue->cell->pos;
   server_request_init(&maze,&request,fd_blue,ACTION_MOVE,blue->cell->pos.x-1,blue->cell->pos.y);
   rc = server_game_action(&maze,&request);
   assertion = (rc >= 0) &&
               (blue_shovel->cell == blue->cell) && 
               (blue_shovel->cell == blue->cell) && 
               (blue->cell->pos.x == old.x-1)    &&
               (blue->cell->pos.y == old.y)      &&
               (blue->cell->object!= blue_shovel)&&
               (player_has_shovel(blue));
   should("correctly move with players",assertion,tc);
   
   ////////////////////////////////
   // TRY TO PICKUP ANOTHER SHOVEL
   ////////////////////////////////
   Object*red_shovel = object_get(&maze,OBJECT_SHOVEL,TEAM_RED); 
   server_request_init(&maze,&request,fd_blue,ACTION_MOVE,red_shovel->cell->pos.x,red_shovel->cell->pos.y);
   request.test_mode =1;
   server_game_action(&maze,&request);
   server_request_init(&maze,&request,fd_blue,ACTION_PICKUP_SHOVEL,blue->cell->pos.x,blue->cell->pos.y);
   rc = server_game_action(&maze,&request);
   
   assertion = (rc == ERR_NO_SHOVEL_SPACE) && 
               (red_shovel->cell->object == red_shovel) &&
               (red_shovel->player != blue) &&
               (blue->shovel == blue_shovel);

   should("not be picked up when a player is holding a similar type",assertion,tc);
  
   /////////////////////////////////
   // PICKUP FLAG AND SHOVEL
   /////////////////////////////////
   Object* blue_flag = object_get(&maze, OBJECT_FLAG , TEAM_BLUE);
   server_request_init(&maze,&request,fd_blue,ACTION_MOVE,blue_flag->cell->pos.x,blue_flag->cell->pos.y);
   request.test_mode = 1; //  teleport
   server_game_action(&maze,&request);

   server_request_init(&maze,&request,fd_blue,ACTION_PICKUP_FLAG,blue->cell->pos.x,blue->cell->pos.y);
   rc = server_game_action(&maze,&request);
   assertion = (rc >= 0) &&
               (blue_flag->cell = blue->cell) &&
               (blue_flag->player = blue)     &&
               (blue->flag == blue_flag)    &&
               (blue_flag->cell->object != blue_flag) &&
               (player_has_shovel(blue) && player_has_flag(blue));
   should("allow picking up both flag and shovel",assertion,tc);
  
   ////////////////////////
   // MOVE WITH BOTH
   /////////////////////// 
   
   old = blue->cell->pos;
   server_request_init(&maze,&request,fd_blue,ACTION_MOVE,blue->cell->pos.x-1,blue->cell->pos.y);
   rc = server_game_action(&maze,&request);
   assertion = (rc >= 0) &&
               (blue_shovel->cell == blue->cell) && 
               (blue_shovel->cell == blue->cell) && 
               (blue_flag->cell = blue->cell)    &&
               (blue_flag->player = blue)        &&
               (blue->cell->pos.x == old.x-1)    &&
               (blue->cell->pos.y == old.y)      &&
               (blue->cell->object!= blue_shovel)&&
               (player_has_shovel(blue) && player_has_flag(blue));
   should("correctly move when a player holds both shovel & flag",assertion,tc);
   
   //////////////////////////////
   // DROP SHOVEL AND DROP FLAG
   //////////////////////////////
    
   server_request_init(&maze,&request,fd_blue,ACTION_DROP_FLAG,blue->cell->pos.x,blue->cell->pos.y);
   rc = server_game_action(&maze,&request);
   assertion = (rc >= 0) &&
               (!player_has_flag(blue))         && 
               (player_has_shovel(blue))        && 
               (blue_flag->cell->object == blue_flag) &&
               (blue_flag->player != blue ) &&
               (blue->cell->player = blue);
   should("drop correctly",assertion,tc);

   ////////////////////////////
   //  GET TAGGED WITH SHOVEL AND FLAG
   ////////////////////////////
   server_request_init(&maze,&request,fd_blue,ACTION_PICKUP_FLAG,blue->cell->pos.x,blue->cell->pos.y);
   rc = server_game_action(&maze,&request);
   
   server_game_add_player(&maze,fd_red,&red);
   server_request_init(&maze,&request,fd_blue,ACTION_MOVE,red->cell->pos.x+1,red->cell->pos.y);
   request.test_mode = 1; //  teleport
   server_game_action(&maze,&request);

   server_request_init(&maze,&request,fd_red,ACTION_MOVE,red->cell->pos.x+1,red->cell->pos.y);
   rc = server_game_action(&maze,&request);
   assertion = ( rc>=0 ) &&
               ( red->state == PLAYER_FREE)           && 
               ( blue->state == PLAYER_JAILED )       &&
               ( !player_has_shovel(blue))            &&
               ( !player_has_flag(blue) )             &&
               ( blue_shovel->cell->type == CELL_HOME)&&
               ( blue_shovel->cell->turf == TEAM_BLUE)&&
               ( blue_flag->cell->player = red  )     &&
               ( blue_flag->cell->object = blue_flag) &&
               ( blue_shovel->cell->object = blue_shovel);
   should("correctly reset and drop when a player is tagged",assertion,tc);
   
   ////////////////////////////
   // USE SHOVEL TEST ...
   ///////////////////////////

   // move red player to the red shovel
   server_request_init(&maze,&request,fd_red,ACTION_MOVE,red_shovel->cell->pos.x,red_shovel->cell->pos.y);
   request.test_mode = 1; //  teleport
   server_game_action(&maze,&request);

   // pick up shovel
   server_request_init(&maze,&request,fd_red,ACTION_PICKUP_SHOVEL,red->cell->pos.x,red->cell->pos.y);
   server_game_action(&maze,&request);
  
   // try to break immutable wall
   server_request_init(&maze,&request,fd_red,ACTION_MOVE,red_shovel->cell->pos.x+1,red_shovel->cell->pos.y);
   rc = server_game_action(&maze,&request);
   assertion = (rc == ERR_WALL )     &&
               (red->shovel == red_shovel)    &&
               (red->shovel->player == red);
   should("not allow players to walk into immutable cells",assertion,tc);
   
   // move red player next to mutable wall
   server_request_init(&maze,&request,fd_red,ACTION_MOVE,14,101);
   request.test_mode = 1; //  teleport
   server_game_action(&maze,&request);
   
   server_request_init(&maze,&request,fd_red,ACTION_MOVE,15,101);
   rc =server_game_action(&maze,&request);
   assertion = ( rc>=0) &&
               ( !player_has_shovel(red))                     &&
               ( red_shovel->cell->type == CELL_HOME)         &&
               ( red_shovel->player !=  red )                 &&
               ( red->cell->pos.x == 15)                      &&
               ( red->cell->type == CELL_FLOOR)               &&
               ( red->cell->is_mutable == CELLTYPE_IMMUTABLE) &&
               ( maze.wall[red->cell->pos.x][red->cell->pos.y] == 1);
   should("correctly break wall with shovel",assertion,tc);
   maze_destroy(&maze);
}

// Create 1 BLUE
// Create 1 RED 
// Passive Jail Red by walking into blue
// Create 2 BLUE
// Create 2 RED
// 2RED tags 2 BLUE
// 1BLUE Frees 2 Blue
void test_game_move(TestContext*tc)
{
    Maze maze;
    maze_build_from_file(&maze,"test.map");

    //////////////////
    // Add a player
    //////////////////
    GameRequest request;
    Pos  next;
    int assertion,rc,fd,id,team;
    fd   = randint()%9000 + 999;
    team = randint()%2;
    Player*player;
    server_game_add_player(&maze, fd, &player );
    assertion = (server_home_count_read(&maze.home[player->team]) == 1) ;
    should("increment home count on spawn",assertion,tc);


    //////////////////
    // Move a player
    //////////////////
    Cell*current = player->cell;
    next.x = player->cell->pos.x+1;
    next.y = player->cell->pos.y;  
    id = server_request_init(&maze,&request,fd,ACTION_MOVE,next.x,next.y);
    
    assertion = (player->id == 0 && id == 0 && player->team == 1);
    should("successfully find the player id and team from the fd",assertion,tc);

    rc = server_game_action(&maze, &request);
    assertion = rc >= 0;
    if (test_debug() && rc<0 ) fprintf(stderr,"Error Code: %d\n",rc);
    should("not error on action request",assertion,tc);
      
    assertion =  (player->cell->pos.x == next.x && player->cell->pos.y == next.y);
    should("successfully move the player's cell reference",assertion,tc);
    
    assertion = (maze.get[next.x][next.y].player == player && current->player != player);
    should("successfully update the cell's player reference", assertion,tc);
    
    //////////////////
    // Move into a Wall
    //////////////////
    current = player->cell; // This is test Safe Only
    server_request_init(&maze,&request,fd,ACTION_MOVE,next.x,next.y-1);
    
    rc = server_game_action(&maze,&request);
    assertion = rc == ERR_WALL;
    should("prevent walking into wall cells",assertion,tc);

    assertion = (player->cell == current && current->player == player);
    should("maintain cell and player references",assertion,tc);
    
    //////////////////
    // Try illegal move
    //////////////////
    server_request_init(&maze,&request,fd,ACTION_MOVE,150,99);
    
    rc = server_game_action(&maze,&request);
    assertion = rc == ERR_BAD_NEXT_CELL;
    should("should prevent moving to cells that are not adjacent",assertion,tc);
    
    //////////////////
    // Test Home Counter
    //////////////////

    request.test_mode = 1; // Enable supernatural jumping
    rc = server_game_action(&maze,&request);
    assertion = rc >= 0;
    assertion = (server_home_count_read(&maze.home[player->team]) == 0);
    should("should home count when players walk out of cell",assertion,tc);

    /////////////////
    // Make New Player
    /////////////////
    int fd_red = fd+1;
    Player*other ;
    rc = server_game_add_player(&maze,fd_red,&other);
    
    server_request_init(&maze,&request,fd_red,ACTION_MOVE,149,99);
    request.test_mode = 1;
    
    rc = server_game_action(&maze,&request); // move 
    assertion = (rc >= 0) && 
                (other->cell->pos.x == 149 && other->cell->pos.y == 99) &&
                (server_home_count_read(&maze.home[other->team]) == 0 );
    should("spawn and move player correctly on TEAM_RED",assertion,tc);

    ///////////////////////////////////////////
    // Tag Player (Unintentional i.e Passive)
    //////////////////////////////////////////
    
    next.x = 150; next.y = 99;
    server_request_init(&maze,&request,fd_red,ACTION_MOVE,next.x,next.y);

    rc = server_game_action(&maze,&request);
    assertion = (rc >= 0) && 
                (other->cell->type == CELL_JAIL) &&
                (other->cell->player == other )  &&
                (other->state == PLAYER_JAILED ) &&
                (player->cell->pos.x == next.x && player->cell->pos.y == next.y);
    should("correctly jail player walking into other on enemy turf",assertion,tc);
    
    ///////////////////////////////////////////
    // Make 2 new Players for Active tagging
    //////////////////////////////////////////
    
    int fd_blue = fd+2;
    Player*blue;
    server_game_add_player(&maze,fd_blue,&blue);
    server_request_init(&maze,&request,fd_blue,ACTION_MOVE,50,99);
    request.test_mode = 1;
    server_game_action(&maze,&request);
    
    int fd_tagger = fd+3;
    Player*tagger;
    server_game_add_player(&maze,fd_tagger,&tagger);
    server_request_init(&maze,&request,fd_tagger,ACTION_MOVE,49,99);
    request.test_mode = 1;
    server_game_action(&maze,&request);

    // active tagging
    server_request_init(&maze,&request,fd_tagger,ACTION_MOVE,50,99);
    rc = server_game_action(&maze,&request);
    assertion = ( rc >= 0 ) &&
                ( blue->cell->type == CELL_JAIL ) &&
                ( blue->cell->player == blue )    &&
                ( blue->state == PLAYER_JAILED )  &&
                ( tagger->cell->player == tagger) &&
                ( blue->cell->turf == opposite_team(blue->team) ) &&
                ( tagger->cell->pos.x == 50 && tagger->cell->pos.y == 99);
    
    should("correctly jail player walking into enemy on home turf",assertion,tc);

    ////////////////////////
    // Test Freeing 
    ////////////////////////
    
    // teleport player to write out side jail
    server_request_init(&maze,&request,fd,ACTION_MOVE,maze.jail[opposite_team(player->team)].min.x-1,99);
    request.test_mode = 1;
    server_game_action(&maze,&request);
    
    // step into the jail
    server_request_init(&maze,&request,fd,ACTION_MOVE,maze.jail[opposite_team(player->team)].min.x,99);
    rc = server_game_action(&maze,&request);
    assertion = (rc >=0) &&
                ( player->cell->player == player ) &&
                ( other->state == PLAYER_FREE )    &&
                ( player->state == PLAYER_FREE )   &&
                ( player->cell->pos.y == 99 )      &&
                ( player->cell->pos.x == maze.jail[opposite_team(player->team)].min.x);
    should("correctly free jailed players when moving into enemy jail cell while free",assertion,tc);


    maze_destroy(&maze);
}

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
    
    // ADD TESTS HERE
    run(test_pickup_drop_logic,"Objects",&tc);
    run(&test_game_move,"Basic Movement",&tc);
    run(&test_game_add_drop,"Game Add/Drop",&tc);
    run(&test_server_locks,"Server Locks",&tc);
    run(&test_plist,"PLists",&tc);
    run(&test_find_and_lock,"Find and Lock Empty Routine",&tc);
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}

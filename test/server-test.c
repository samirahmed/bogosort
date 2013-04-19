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

void test_game_move(TestContext*tc)
{
    Maze maze;
    maze_build_from_file(&maze,"test.map");

    //////////////////
    // Move a player
    //////////////////
    GameRequest request;
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
    Cell* current = player->cell; // This is test Safe Only
    Pos  next;
    next.x = current->pos.x+1;
    next.y = current->pos.y;
    id = server_request_init(&maze,&request,fd);
    
    assertion = (player->id == 0 && id == 0 && player->team == 1);
    should("successfully find the player id and team from the fd",assertion,tc);

    request.action  = ACTION_MOVE;
    request.current = current->pos;
    request.next    = next; 
    
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
    next.x = next.x;
    next.y = next.y-1;
    server_request_init(&maze,&request,fd);
    request.action  = ACTION_MOVE;
    request.current = current->pos;
    request.next    = next; 
    
    rc = server_game_action(&maze,&request);
    assertion = rc == ERR_BAD_NEXT_CELL;
    should("prevent walking into wall cells",assertion,tc);

    assertion = (player->cell == current && current->player == player);
    should("maintain cell and player references",assertion,tc);
    
    //////////////////
    // Try illegal move
    //////////////////
    
    current = player->cell; // This is test Safe Only
    next.x = 150;
    next.y = 99;
    server_request_init(&maze,&request,fd);
    request.action  = ACTION_MOVE;
    request.current = current->pos;
    request.next    = next; 
    
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

    /*rc = server_game_action(&)*/

    maze_destroy(&maze);
}

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
    
    // ADD TESTS HERE
    run(&test_game_move,"Basic Movement",&tc);
    run(&test_game_add_drop,"Game Add/Drop",&tc);
    run(&test_server_locks,"Server Locks",&tc);
    run(&test_plist,"PLists",&tc);
    run(&test_find_and_lock,"Find and Lock Empty Routine",&tc);
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}

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

/*Maze maze;  // global maze*/

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
    bzero(&tasks, sizeof(Task)*2 );
    
    test_task_init(&tasks[0],(Proc)&st_increment_home,50,&maze.home[team],NULL,NULL,NULL,NULL,NULL);
    test_task_init(&tasks[1],(Proc)&st_decrement_home,48,&maze.home[team],NULL,NULL,NULL,NULL,NULL);
    parallelize(tasks,2,thread_per_task);
    
    assertion = server_home_count_read(&maze.home[team]) == ((tasks[0].reps-tasks[1].reps)*thread_per_task);
    should("atomically increment and decrement home counter",assertion,tc);
   
    maze_destroy(&maze);

    ////////////////
    // PLIST COUNTER
    ////////////////
    
    thread_per_task = 500;
    team = randint()%NUM_TEAMS;
    // Assign a team at random 

    // create tasks
    bzero(&tasks, sizeof(Task)*2 );
    
    test_task_init(&tasks[0],(Proc)&st_increment_plist,20,&maze,&team,NULL,NULL,NULL,NULL);
    test_task_init(&tasks[1],(Proc)&st_decrement_plist,18,&maze,&team,NULL,NULL,NULL,NULL);
    parallelize(tasks,2,thread_per_task);
    
    assertion = server_plist_player_count(&maze.players[team]) == ((tasks[0].reps-tasks[1].reps)*thread_per_task);
    should("atomically increment and decrement player counter",assertion,tc);
   
    maze_destroy(&maze);
}

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
    
    // ADD TESTS HERE
    run(&test_server_locks,"Server Locks",&tc);
    run(&test_find_and_lock,"Find and Lock Empty Routine",&tc);
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}

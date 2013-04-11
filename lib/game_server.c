#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <strings.h>
#include "net.h"
#include "protocol.h"
#include "protocol_session.h"
#include "game_server.h"

/*****************/
/* LOCKING FUNCS */
/*****************/

extern void server_maze_lock(Maze*m , Pos current, Pos next)
{
  Cell C = m->get[current.x][current.y];
  Cell N = m->get[next.x][next.y];
  
  if (C.type == CELL_JAIL || N.type == CELL_JAIL ) server_jail_lock( &(m->jail[C.turf]) );

  Cell first;
  Cell second;
  
  // first is lower x value if x equal then lower y value.
  if ( C.pos.x < N.pos.x )
  {
    first = C;
    second = N;
  }
  else if ( N.pos.x < C.pos.x )
  {
    first = N;
    second = C;
  }
  else if ( C.pos.y < N.pos.y )
  {
    first = C;
    second = N;
  }
  else if ( N.pos.y < C.pos.y )
  {
    first = N;
    second = C;
  }
  else  // current == next
  {
    first = C;
    second = N;
  }

  cell_lock(&first);
  if (!(current.x == next.x && current.y == next.y ))
  {
    // if current != next;
    cell_lock(&second);
  }
}

extern void server_maze_unlock(Maze*m, Pos current, Pos next)
{
  Cell C = m->get[current.x][current.y];
  Cell N = m->get[current.x][current.y];
  
  // Order of cell unlock doesn't matter so long as jail is last unlocked
  cell_unlock(&C);
  cell_unlock(&N);

  if (C.type == CELL_JAIL || N.type == CELL_JAIL ) server_jail_unlock( &(m->jail[C.turf]) );
}

extern void server_jail_lock(Jail * jail)
{
  pthread_mutex_lock(&(jail->jail_recursive_lock));
}

extern void server_jail_unlock(Jail * jail)
{
  pthread_mutex_unlock(&(jail->jail_recursive_lock));
}

/****************/
/* HOME METHODS */
/****************/

extern int server_home_count_read(Home* home)
{
  int count;
  pthread_rwlock_rdlock(&home->count_wrlock);
  count = home->count;
  pthread_rwlock_unlock(&home->count_wrlock);
  return count;
}

extern int server_home_count_increment(Home* home)
{
  int count;
  pthread_rwlock_wrlock(&home->count_wrlock);
  home->count--;
  count = home->count;
  pthread_rwlock_unlock(&home->count_wrlock);
  return count;
}

extern int server_home_count_decrement(Home* home)
{
  int count;
  pthread_rwlock_wrlock(&home->count_wrlock);
  home->count--;
  count = home->count;
  pthread_rwlock_unlock(&home->count_wrlock);
  return count;
}

/*****************/
/* JAIL  METHODS */
/*****************/

extern int _server_jailbreak( Maze*m, Team_Types team )
{
   int startx, starty, stopx, stopy, xx, yy;
   Player* prisoner;
   Jail* jail = &(m->jail[team]);
   startx = jail->min.x;
   starty = jail->min.y;
   stopx  = jail->max.x;
   stopy  = jail->max.y;

   for (xx = startx; xx< stopx ; xx++ )
   {
      for( yy = starty; yy < stopy ; yy++ )
      {
        prisoner = m->get[xx][yy].player;
        if ( prisoner )
        {
          if (prisoner->state == PLAYER_JAILED)
          {
             prisoner->state = PLAYER_FREE;
          }
        }
      }
   }

   return 1;
}

/*****************/
/* PLIST METHODS */
/*****************/

/****************/
/* MOVE METHODS */
/****************/ 

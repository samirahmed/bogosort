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

extern void server_object_write_lock(Maze*m)
{
  pthread_rwlock_wrlock(&(m->object_wrlock));
}

extern void server_object_read_lock(Maze*m)
{
  pthread_rwlock_rdlock(&(m->object_wrlock));
}

extern void server_object_unlock(Maze*m)
{
  pthread_rwlock_unlock(&(m->object_wrlock));
}

/****************/
/* HOME METHODS */
/****************/
extern void server_hash_id( Maze* m, int key, Cell** cell, Team_Types team)
{
  Home*home = &m->home[team];
  unsigned int yy,xx, xlen, ylen;
  xlen = (home->max.x-home->min.x);
  ylen = (home->max.y-home->min.y);
  
  yy = (unsigned) ( key / xlen )%( ylen );
  yy += home->min.y;
  xx = (unsigned) key % xlen;
  xx += home->min.x;
  /*fprintf(stderr,"%d,%d\n",xx,yy);*/
  *cell = &(m->get[xx][yy]);
}

// try many home cells for a lock
// query 0 = unoccupied
// query 1 = not holding object
extern int server_find_empty_home_cell_and_lock(Maze*m, Team_Types team, Cell** cell ,int id, int query)
{
   int try, found, home_size;
   
   try = 0;
   found = -1;
   Cell * c;
   Home *home = &(m->home[team]);
   home_size = (home->max.x-home->min.x)*(home->max.y-home->min.y);
   while ( (try < home_size*2) && (found!=0) )
   {
      server_hash_id( m, id+try, cell, team);
      c = *cell;
      try++;
      found = pthread_mutex_trylock(&(c->lock));
     
      if (found != 0) { continue;}
      else if ((query==0 && !cell_is_unoccupied(c)) || (query!=0 && cell_is_holding(c))) 
      {
        found = -1;
        cell_unlock(c);
      }
   }
   
   if (found != 0) return -1;
   return 0;
}

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

/*****************/
/* PLIST METHODS */
/*****************/

/******************/
/* ACTION METHODS */
/* _* = unsafe    */
/******************/ 
extern int _server_action_drop_flag(Maze*m , Player* player)
{
    if (!player) return -1;
    if (player->cell->object) return -2; // FIXME can't drop flag,, take flag to jail with you
    if (!player->flag) return 1;         // no need to drop
    
    Object*object = player->flag; 
    Cell* cell = player->cell;
    
    // update everbodies references
    _server_action_update_cell_and_player(m,object,cell,0);
    cell->object = object;
    player->flag = 0;
    return 0;
}

extern int _server_action_player_reset_shovel(Maze*m, Player*player)
{
  if (!player) return -1;
  if (!player->shovel) return -2;
  
  int rc;
  Object * object = player->shovel;
  Cell * cell = player->cell;
  Cell * next;

  // find next cell and set its object
  rc = server_find_empty_home_cell_and_lock(m, object->team, &next, 0, 1); // 0=seed, 1=not-holding query
  if (rc < 0) return -1;
  _server_action_update_cell_and_player(m,object,next,0);
  next->object = object;
  cell_unlock(next);

  // drop my shovel
  cell->object=0;
  player->shovel= 0;
  
  return 0;
}

extern int _server_action_drop_shovel(Maze*m , Player*player)
{
   if (!player) return -1;
   if (player->cell->object) return -2; // CAN'T DROP HERE
   if (!player->shovel) return 1;       // no need to do more, shovel dropped

   Object* object= player->shovel ;
   Cell* cell = player->cell ;

   // update everybody references
   _server_action_update_cell_and_player(m,object,cell,0);
   cell->object = object;
   player->shovel = 0; 
   return 0;
}

extern int _server_action_pickup_object(Maze*m, Player* player)
{
    if (!player) return -1;
    if (!player->cell->object) return -2;  // nothing to pickup
    if (!player->flag) return 1;  // no need to drop
    
    Object * object = player->cell->object;
    Cell* cell = player->cell;
    
    // no need to lock for reading immutable properties
    if (player->flag && object->type == OBJECT_FLAG ) return -3;  // already has a flag
    if (player->shovel && object->type == OBJECT_SHOVEL ) return -4; // already has a shovel
   
    _server_action_update_cell_and_player(m,object,cell,player);
    // kill cell->object link
    cell->object = 0;
    
    // pickup object
    if (object->type == OBJECT_SHOVEL)
    {
      player->shovel = object;
    }
    else
    {
      player->flag = object;
    }
    
    return 0;
}

extern int _server_action_update_cell(Maze*m, Object* object, Cell* newcell )
{
  if (!object) return -1;
  
  server_object_write_lock(m);
  object->cell = newcell;
  server_object_unlock(m); 
  return 0;
}

extern int _server_action_update_cell_and_player(Maze*m, Object* object, Cell* newcell, Player* player)
{
  if (!object) return -1;
  
  server_object_write_lock(m);
  object->cell = newcell;
  object->player = player;
  server_object_unlock(m); 
  return 0;
}

extern int _server_action_update_player(Maze*m, Player*player, Cell*newcell)
{
  if (!player) return -1;
  player->cell = newcell;
  _server_action_update_cell(m, player->shovel, newcell );
  _server_action_update_cell(m, player->shovel, newcell );
  return 0;
}

extern int _server_action_move_player(Maze*m, Cell* currentcell , Cell* nextcell )
{
   if (!currentcell || !nextcell) return -1;
   if (currentcell == nextcell ) return 1;    // no need to move
   nextcell->player = currentcell->player;
   currentcell->player = 0;
   _server_action_update_player(m, nextcell->player, nextcell);
   return 0;
}

extern int _server_find_empty_jail_cell(Maze*m, Team_Types team, Pos*pos)
{
  int xx,yy;
  Jail * jail = &(m->jail[team]);
  Cell cell;
   for (xx = jail->min.x; xx< jail->max.y ; xx++ )
   {
      for( yy = jail->min.y; yy < jail->max.y ; yy++ )
      {
        cell = m->get[xx][yy];
        if ( cell_is_unoccupied(&cell) )
        {
          pos->x = cell.pos.x;
          pos->y = cell.pos.y;
          return 0;
        }
      }
   }

  return -1;
}

extern int _server_action_jail_player(Maze*m, Cell* currentcell)
{
  if (!currentcell->player) return -1;
  Player * player = currentcell->player;
  if (currentcell->player->state == PLAYER_JAILED) return 1; // no need to jail
  
  Pos pos;
  int rc;

  // find the next jail cell
  rc = _server_find_empty_jail_cell(m, opposite_team(player->team), &pos );
  if (rc < 0) return -3;
  Cell * nextcell = &(m->get[pos.x][pos.y]);

  // drop shovel and flag if any
  _server_action_player_reset_shovel(m,player);
  _server_action_drop_flag(m,player);
  
  // update the player state
  currentcell->player->state = PLAYER_JAILED;
  if (currentcell != nextcell )
  {
    _server_action_move_player(m,currentcell, nextcell);
  }
  return 0;
}

extern int _server_action_jailbreak( Maze*m, Team_Types team )
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

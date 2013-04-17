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

// This will lock down the current cell and next cell
// If you only want to lock down one current = next
extern void server_maze_lock(Maze*m , Pos current, Pos next)
{
  Cell C = m->get[current.x][current.y];
  Cell N = m->get[next.x][next.y];
  
  Cell first;
  Cell second;
  int lock_second=0;
  
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
 
  // First Lock Cells
  cell_lock(&first);
  if (!(current.x == next.x && current.y == next.y ))
  {
    // if current != next;
    cell_lock(&second);
    lock_second=1;
  }

  // Now Lock First Cell's object/players and player's objects
  if (first.object){ object_lock(first.object); }
  if (first.player)
  { 
    player_lock(first.player); 
    if (first.player->shovel){ object_lock(first.player->shovel); }
    if (first.player->flag){ object_lock(first.player->flag); }
  }

  // if second cell is not the same repeat
  if (lock_second)
  {
      if (second.object){ object_lock(second.object); }
      if (second.player)
      { 
        player_lock(second.player); 
        if (second.player->shovel){ object_lock(second.player->shovel); }
        if (second.player->flag){ object_lock(second.player->flag); }
      }
  }

}


// This will unlock 2 cells current and next,
// doesn't matter if they are the same or different
extern void server_maze_unlock(Maze*m, Pos current, Pos next)
{
  Cell C = m->get[current.x][current.y];
  Cell N = m->get[current.x][current.y];
 
  // Unlock in reverse order Unlock N
  if (N.object) object_unlock(N.object);
  if (N.player)
  {
    if (N.player->shovel) object_unlock(N.player->shovel);
    if (N.player->flag) object_unlock(N.player->flag);
    player_unlock(N.player);
  }
  
  // Unlock C
  if (C.object) object_unlock(C.object);
  if (C.player)
  {
    if (C.player->shovel) object_unlock(C.player->shovel);
    if (C.player->flag) object_unlock(C.player->flag);
    player_unlock(C.player);
  }
  
  // Order of cell unlock doesn't matter so long as jail is last unlocked
  cell_unlock(&C);
  cell_unlock(&N);
}

extern void object_lock(Object*object)
{
  pthread_mutex_lock(&(object->lock));
}

extern void object_unlock(Object*object)
{
  pthread_mutex_lock(&(object->lock));
}

extern void player_lock(Player*player)
{
  pthread_mutex_lock(&(player->lock));
}

extern void player_unlock(Player*player)
{
  pthread_mutex_lock(&(player->lock));
}

// DELETE LATER
extern void server_object_write_lock(Maze*m)
{
  pthread_rwlock_wrlock(&(m->object_wrlock));
}

// DELETE LATER
extern void server_object_read_lock(Maze*m)
{
  pthread_rwlock_rdlock(&(m->object_wrlock));
}

// DELETE LATER
extern void server_object_unlock(Maze*m)
{
  pthread_rwlock_unlock(&(m->object_wrlock));
}

/****************/
/* HOME METHODS */
/****************/
extern void server_home_hash( Maze* m, int key, Cell** cell, Team_Types team)
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
      server_home_hash( m, id+try, cell, team);
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
  pthread_rwlock_rdlock(&(home->count_wrlock));
  count = home->count;
  pthread_rwlock_unlock(&(home->count_wrlock));
  return count;
}

extern int server_home_count_increment(Home* home)
{
  int count;
  pthread_rwlock_wrlock(&(home->count_wrlock));
  home->count++;
  count = home->count;
  pthread_rwlock_unlock(&(home->count_wrlock));
  return count;
}

extern int server_home_count_decrement(Home* home)
{
  int count;
  pthread_rwlock_wrlock(&(home->count_wrlock));
  home->count--;
  count = home->count;
  pthread_rwlock_unlock(&(home->count_wrlock));
  return count;
}

/******************/
/* PLAYER METHODS */
/******************/

// Copy the players position into the Position given
extern int player_get_position(Player * player, Pos * pos)
{
  int rc = -1;
  if (player->cell)
  {
    pos->x = (player->cell)->pos.x;
    pos->y = (player->cell)->pos.y;
    rc =0;
  }
  return rc;
}

// Convenience methdos, true if a player has a shovel, false otherwise
extern int player_has_shovel(Player * player)
{
  return ((player->shovel) ? 1 : 0);
}

// Convenience methods, true of player has a flag, false otherwise
extern int player_has_flag(Player * player)
{
  return ( (player->flag) ? 1 : 0 );
}

// Find a position for a player. Ensure that player doesn't have an object or shovel
// Returns -1 if unable to find a home cell
// Returns -2 if player is holding an object, they can't be spawned
extern int player_spawn(Maze* m, Player* player)
{
  int rc = 0;
  Cell* homecell;
  rc = server_find_empty_home_cell_and_lock(m, player->team, &homecell, player->id, 0); // 0=player query
  if (rc < 0) return -1;
  if (player_has_flag(player) || player_has_shovel(player)) return -2;
   
  // add player to cell.  
  player->cell = homecell;
  homecell->player = player;
  server_home_count_increment(&m->home[player->team]);
  cell_unlock(homecell);
  return rc;
}

/*****************/
/* JAIL  METHODS */
/*****************/
extern void server_jail_hash( Maze* m, int key, Cell** cell, Team_Types team)
{
  Jail*jail = &m->jail[team];
  unsigned int yy,xx, xlen, ylen;
  xlen = (jail->max.x-jail->min.x);
  ylen = (jail->max.y-jail->min.y);
  
  yy = (unsigned) ( key / xlen )%( ylen );
  yy += jail->min.y;
  xx = (unsigned) key % xlen;
  xx += jail->min.x;
  *cell = &(m->get[xx][yy]);
}

// try many jail cells for a lock
// query 0 = unoccupied
// query 1 = not holding object
extern int server_find_empty_jail_cell_and_lock(Maze*m, Team_Types team, Cell** cell ,int id, int query)
{
   int try, found, jail_size;
   
   try = 0;
   found = -1;
   Cell * c;
   Jail *jail = &(m->jail[team]);
   jail_size = (jail->max.x-jail->min.x)*(jail->max.y-jail->min.y);
   while ( (try < jail_size*2) && (found!=0) )
   {
      server_jail_hash( m, id+try, cell, team);
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

/*****************/
/* PLIST METHODS */
/*****************/

// Thread safe player_count read
extern int server_plist_player_count( Plist * plist )
{
   int result;
   pthread_rwlock_rdlock(&plist->plist_wrlock);
   result = plist->count;
   pthread_rwlock_unlock(&plist->plist_wrlock);
   return result;
}

// Thread safe increment count
extern int server_plist_player_count_increment( Plist * plist )
{
   int result;
   pthread_rwlock_wrlock(&plist->plist_wrlock);
   plist->count++;
   result = plist->count;
   pthread_rwlock_unlock(&plist->plist_wrlock);
   return result;
}

// Thread safe decrement count
extern int server_plist_player_count_decrement( Plist * plist )
{
   int result;
   pthread_rwlock_wrlock(&plist->plist_wrlock);
   plist->count--;
   result = plist->count;
   pthread_rwlock_unlock(&plist->plist_wrlock);
   return result;
}

// Add player to plist. returns the players id if successful
// Returns -1 if player already exists
// Returns -2 if it can't find an empty slot
extern int server_plist_add_player(Plist* plist, int fd)
{
  int rc,empty, ii ;
  rc = 0;
  empty = -1;
  
  // lock for write access
  pthread_rwlock_wrlock(&plist->plist_wrlock);
  
  // first check if we are at our max, if so unlock and return rc
  if (plist->count == plist->max) rc = -2;
  else 
  {
    for (ii=0;ii<plist->max;ii++)
    {
      if ( plist->at[ii].fd == fd ) { rc = -1; break; }
      if ( plist->at[ii].fd == -1 && empty == -1) empty = ii;
    }
    
    if (empty == -1) rc = -2;
    if (rc >= 0)
    {
       Player * player = &plist->at[empty];
       player_init(player);
       player->team = plist->team;
       player->id = empty;
       player->fd = fd;
       plist->count++;
       rc = player->id;
    }
  }
  pthread_rwlock_unlock(&plist->plist_wrlock);
  
  return rc;

  /*if (server_plist_player_count(plist) >= plist->max) return -2;*/

  // ensure that no players have fd that is similar if so return -1
  /*rc = server_plist_find_player_by_fd(plist,fd);*/
  /*if (rc!=-1) return -1;*/
  
  // find and empty location for a player 
  /*empty = server_plist_find_player_by_fd(plist,-1);*/
  /*if ( empty == -1 ) return -2;*/

  // create a new player object and then init
  /*pthread_rwlock_wrlock(&plist->plist_wrlock);*/
  /*Player * player = &plist->at[empty];*/
  /*player_init(player);*/
  /*player->team = plist->team;*/
  /*player->id = empty;*/
  /*player->fd = fd;*/
  
  /*server_plist_player_count_increment(plist);*/

}

extern int server_plist_find_player_by_fd(Plist* plist, int fd )
{
   int result, ii ;
   result = -1;
   pthread_rwlock_rdlock(&plist->plist_wrlock);
   for (ii=0; ii< plist->max ; ii++ )
   {
      if (plist->at[ii].fd == fd )
      {
        result = ii;
        break;
      }
   }
   pthread_rwlock_unlock(&plist->plist_wrlock);
   return result;
}

extern int server_plist_drop_player_by_fd(Maze*m, Plist*plist, int fd)
{
   int id;
   id = server_plist_find_player_by_fd(plist,fd);
   if (id == -1) return -1;
   
   server_plist_drop_player_by_id(m,plist,id);
   return id;
}

extern void server_plist_drop_player_by_id(Maze*m, Plist* plist, int id )
{
  Player *player;
  player = &plist->at[id];
  
  // since we are changing the fds, get a write lock on the plist
  pthread_rwlock_wrlock(&plist->plist_wrlock);
  
  if (player->fd != -1 )  
  { 
    player->fd = -1;
    if (player->cell)
    {
      if (home_contains(&(player->cell->pos),&(m->home[plist->team])))
      {
         server_home_count_decrement(&m->home[plist->team]); 
      }
    }
  }
  pthread_rwlock_unlock(&plist->plist_wrlock);

  server_plist_player_count_decrement(plist);
}

/******************/
/* ACTION METHODS */
/* _* = unsafe    */
/******************/ 
extern int _server_action_drop_flag(Maze*m , Player* player)
{
    if (!player) return -1;
    if (player->cell->object) return -2; // FIXME can't drop flag, take flag to jail with you
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
  _server_action_update_cell_and_player(m,object,next,0);  // move object and kill player link
  next->object = object;  // link object to cell
  cell_unlock(next); // unlock this new cell

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
    Cell* cell = player->cell;
    
    if (!player->cell->object) return -2;  // nothing to pickup
    Object * object = player->cell->object;
    
    
    if (player->flag && object->type == OBJECT_FLAG ) return -3;  // already has a flag
    if (player->shovel && object->type == OBJECT_SHOVEL ) return -4; // already has a shovel
   
    _server_action_update_cell_and_player(m,object,cell,player);
    cell->object = 0; // kill cell->object link
    
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
  object->cell = newcell;
  return 0;
}

extern int _server_action_update_cell_and_player(Maze*m, Object* object, Cell* newcell, Player* player)
{
  if (!object) return -1;
  object->cell = newcell;
  object->player = player;
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

extern int _server_action_jail_player(Maze*m, Cell* currentcell)
{
  if (!currentcell->player) return -1;
  Player * player = currentcell->player;
  if (player->state == PLAYER_JAILED) return 1; // no need to jail
  
  Cell * nextcell;
  int rc;

  // find the next jail cell
  rc = server_find_empty_jail_cell_and_lock(m,opposite_team(player->team),&nextcell,player->id,0);
  if (rc < 0) return -3;

  // drop shovel and flag if any
  _server_action_player_reset_shovel(m,player);
  _server_action_drop_flag(m,player);
  
  // update the player state
  player->state = PLAYER_JAILED;
  if (currentcell != nextcell )
  {
    _server_action_move_player(m,currentcell, nextcell);
  }
  return 0;
}

extern int _server_action_jailbreak( Maze*m, Team_Types team, Cell*current, Cell*next )
{
   int startx, starty, stopx, stopy, xx, yy;
   Player* prisoner;
   Jail* jail = &(m->jail[team]);
   startx = jail->min.x;
   starty = jail->min.y;
   stopx  = jail->max.x;
   stopy  = jail->max.y;

   // Lock all the cells except current and next
   for (xx = startx; xx< stopx ; xx++ )
   {
      for( yy = starty; yy < stopy ; yy++ )
      {
        
        Cell cell = m->get[xx][yy];
        
        if (cell.pos.x == current->pos.x && cell.pos.y == current->pos.y ) continue;
        if (cell.pos.x == next->pos.x && cell.pos.y == next->pos.y ) continue;
        server_maze_lock(m, cell.pos, cell.pos); // Lock down cell 
        prisoner = cell.player;
        if ( prisoner )
        {
          if (prisoner->state == PLAYER_JAILED)
          {
             prisoner->state = PLAYER_FREE;
          }
        }
      }
   }

   // Unlock All the cells except current and next
   for (xx = startx; xx< stopx ; xx++ )
   {
      for( yy = starty; yy < stopy ; yy++ )
      {
        Cell cell = m->get[xx][yy];
        
        if (cell.pos.x == current->pos.x && cell.pos.y == current->pos.y ) continue;
        if (cell.pos.x == next->pos.x && cell.pos.y == next->pos.y ) continue;
        server_maze_unlock(m, cell.pos, cell.pos); // Lock down cell 
      }
   }

   return 1;
}

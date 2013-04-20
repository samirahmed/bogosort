#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <strings.h>
#include "net.h"
#include "types.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_session.h"
#include "game_server.h"

/*******************/
/* REQUEST METHODS */
/*******************/

extern int server_request_init(Maze*m,GameRequest*request,int fd,Action_Types action, Pos pos)
{
  int team;
  int id;
  
  server_fd_to_id_and_team(m, fd, &team, &id);
  bzero(request,sizeof(request));
  request->team   = team;
  request->fd     = fd;
  request->id     = id;
  request->pos.x  = pos.x;
  request->pos.y  = pos.y;
  request->action = action;
  
  return 0;
}

extern int server_fd_to_id_and_team(Maze*m,int fd, int *team_ptr, int*id_ptr)
{
  int id, team;
  team = TEAM_RED;
  Plist* team_red = &m->players[TEAM_RED];
  id = server_plist_find_player_by_fd(team_red,fd);
  if (id == -1) 
  {
    team = TEAM_BLUE;
    Plist* team_blue = &m->players[TEAM_BLUE];
    id = server_plist_find_player_by_fd(team_blue,fd);
    if (id ==-1) return ERR_NO_PLAYER;
  }
  *team_ptr = team;
  *id_ptr   = id;
  return 0; 
}

/******************/
/* SERVER METHODS */
/******************/

// Returns the player id if it is successfully added to the game
// Returns -1 if player exists
// Returns -2 if player cannot be added to plist for some reason
// Returns -3 if player spawn in home cell failed
extern int server_game_add_player(Maze*maze,int fd, Player**player)
{
  int rc,team,id;
  
  // get team
  server_maze_property_lock(maze);
  maze->last_team = opposite_team(maze->last_team);
  team = maze->last_team;
  server_maze_property_unlock(maze);
 
  *player = &maze->players[team].at[0]; // return first player if so that player pointer is at least set

  rc = server_plist_find_player_by_fd(&maze->players[team],fd);
  if (proto_debug() && rc>=0) fprintf(stderr,"player with fd %d already exists: find rc=%d\n",fd, rc);
  if (rc >= 0 ) return -1;
  
  rc = server_plist_add_player(&maze->players[team],fd);
  if (proto_debug() && rc<0) fprintf(stderr,"player with fd %d could not be added: rc=%d\n",fd, rc);
  if (rc < 0 ) return -2;
  id = rc;

  rc = server_player_spawn(maze,&(maze->players[team].at[id]) );
  if (proto_debug() && rc<0) fprintf(stderr,"player with fd %d could not be spawned: rc=%d\n",fd,rc); 
  if (rc < 0) return -3;
  
  *player = &(maze->players[team].at[id]);

  return id;
}

// Removes player from plist, servers connection with players/flags/shovels
extern void server_game_drop_player(Maze*maze,int team, int id)
{
  Cell* cell;
  int rc;
  Player*player = &maze->players[team].at[id];
  
  // Acquire Lock
  rc = server_maze_lock_by_player(maze, player, 0);
  if (proto_debug() && rc <0) 
  {
    fprintf(stderr,"FATAL: Optimistic Lock Fail\n" ); 
    return; 
  }
  
  // Get Cell Handle
  cell = player->cell;

  // drop player, unlock player and player's objects
  _server_drop_handler(maze,player);
  
  // Kill FD
  server_plist_drop_player_by_id(maze, &maze->players[team] , id);

  // Open cell again
  server_maze_unlock(maze, cell->pos, cell->pos);
  
  if (proto_debug()) fprintf(stderr,"Dropped Team:%d Player:%d\n",team,id);
}

// Processes move from cell to next cell.
// Return ERR_BAD_PLAYER_ID = Unauthenticated Request
// Return ERR_BAD_NEXT_CELL = Bad Next Position
extern int server_game_action(Maze*maze , GameRequest* request)
{
  int team    = request->team;
  int id      = request->id;
  Pos next    = request->pos;
  int fd      = request->fd;
  Action_Types action  = request->action;
  
  // Get the player 
  Player*player = &(maze->players[team].at[id]);
  Cell *cell, *currentcell, *nextcell;
  if ( !server_validate_player(maze,team, id, fd) ) return ERR_BAD_PLAYER_ID;
  
  int once = 1;
  int rc = 0;

  // Lock the cell
  server_maze_lock_by_player(maze, player, &next);
  cell = player->cell;

  while(once--)
  {

    currentcell = cell;
    nextcell    = &maze->get[next.x][next.y];
    
    // Check that the next cell is near the current cell
    // jump allows us to move player to anywhere
    if (!cell_is_near(currentcell, nextcell) && !request->test_mode) {rc= ERR_BAD_NEXT_CELL; break;}
    
    // Delegate the Action Accordingly
    switch(action)
    {
      case ACTION_NOOP: 
        rc = 0;
      break;
      
      case ACTION_MOVE: 
        rc = _server_game_move(maze,player,currentcell,nextcell);
        if (rc>=0 && proto_debug())
        {
          fprintf(stderr,"team:%d,id:%d from %d,%d to %d,%d\n",
            team,id,currentcell->pos.x,currentcell->pos.y,player->cell->pos.x,player->cell->pos.y);
        }
      break;
      
      case ACTION_DROP_FLAG: 
        rc =_server_action_drop_flag(maze,player); 
      break;
      
      case ACTION_DROP_SHOVEL: 
        rc = _server_action_drop_shovel(maze,player); 
      break;
      
      case ACTION_PICKUP_FLAG: 
        rc = _server_action_pickup_object(maze,player);
      break;

      case ACTION_PICKUP_SHOVEL: 
        rc = _server_action_pickup_object(maze,player);
      break;

      default:
        fprintf(stderr,"Bad Action Type %d",action);
        rc = ERR_BAD_ACTION;
      break;
    }
  }
  if (proto_debug() && rc<0) fprintf(stderr,"Error: %d for Action:%d | Id:%d | Team%d\n",rc,action,id,team);
  server_maze_unlock(maze ,cell->pos, next);
  return rc;
}

extern int _server_game_move(Maze*m, Player*player, Cell* current, Cell*next)
{
  int rc = 0;

  // Check if player is jailed if so go to jail move handler
  if (player->state == PLAYER_JAILED && next->type != CELL_WALL) 
  { 
    if (next->type!=CELL_JAIL && cell_is_unoccupied(next))
    rc = _server_action_move_player(m,current,next);
  }
  else if ( next->type == CELL_WALL )
  {
    rc = _server_game_wall_move(m,player,current,next);
  }
  else
  {
    rc = _server_game_floor_move(m,player,current,next);
  }
  
  if (rc>=0) rc = _server_game_state_update(m,player,current,next);
  
  return rc;
}

extern int _server_game_state_update(Maze*m, Player*player, Cell*current, Cell*next)
{
  int rc=0;
  if (current->type!=CELL_HOME && next->type==CELL_HOME && player->team == next->turf )
  {
    server_home_count_increment(&m->home[player->team]);
  }
  if (current->type==CELL_HOME && next->type!=CELL_HOME && player->team == current->turf )
  {
    server_home_count_decrement(&m->home[player->team]);
  }

  return rc;
}

extern int _server_game_wall_move(Maze*m,Player*player, Cell*current, Cell*next)
{
  if (!player->shovel) return ERR_BAD_NEXT_CELL;
  
  int rc = ERR_NOOP;
  // LOCK THE broken walls for writing
  // BREAK THE WALL
  // RESET THE SHOVEL
  rc = _server_action_player_reset_shovel(m,player);
  // MOVE THE PLAYER
  rc = _server_action_move_player(m,current,next);
  // UNLOCK the broken walls
  //
  return rc;
}

extern int _server_game_floor_move(Maze*m, Player*player, Cell*current, Cell*next)
{
  int rc = ERR_NOOP;
  if ( cell_is_unoccupied(next) )
  {
    // check if this qualifies for a free
    if (next->type==CELL_JAIL && current->type!=CELL_JAIL && next->turf!=player->team )
    {
      _server_action_jailbreak(m, player->team, current, next );
    }
    
    rc = _server_action_move_player(m, current, next);
  }
  else
  {
    // if same team in next cell or already jailed
    if ( next->player->team == player->team) return ERR_CELL_OCCUPIED;
    if ( next->player->state==PLAYER_JAILED ) return ERR_CELL_OCCUPIED;
    if ( next->player->team != player->team && next->turf != player->team )
    {
      rc = _server_action_jail_player(m,current);
    }
    if ( next->player->team != player->team && next->turf == player->team )
    {
      _server_action_jail_player(m,next);
      if(cell_is_unoccupied(next)) rc = _server_action_move_player(m, current, next);
    }
  }

  return rc;
}

extern int server_validate_player( Maze*m, Team_Types team, int id , int fd )
{
  int rc;
  rc = 1;
  Plist*plist = &m->players[team];
  server_plist_read_lock(plist);
  Player*player = &plist->at[id];
  if (player->fd != fd ) rc = 0; 
  server_plist_unlock(plist);
  
  return rc;
}

/*****************/
/* LOCKING FUNCS */
/*****************/

// Returns -1 if cannot lock.  SHOULD NEVER RETURN -1.
extern int server_maze_lock_by_player(Maze*m, Player*player , Pos * next )
{
  Cell*cell;
  player_lock(player);
  cell = player->cell;
  player_unlock(player);
  int found = 0;
  int try = 10;
  int rc = -1;

  // optimistic locking, logically shouldn't take more than 2 tries
  while(try-->0 && !found )
  {
    // if next is null or zero, lock just the one cell
    next ? server_maze_lock(m,cell->pos,*next) : server_maze_lock(m,cell->pos,cell->pos);

    if ( player->cell == cell && cell->player == player)
    {
      found = 1;
      rc = 0;
    }
    else
    {
      // unlock the cells/ cell depending on if next is null or zero
      next? server_maze_unlock(m,cell->pos,*next): server_maze_lock(m,cell->pos,cell->pos);
    }
  }
  return rc;
}

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

  // Now Lock the cell heirarhcy
  _server_root_lock(m,&first);

  // if second cell is not the same repeat
  if (lock_second)
  {
    _server_root_lock(m,&second);
  }

}

// This will unlock 2 cells current and next,
// doesn't matter if they are the same or different
extern void server_maze_unlock(Maze*m, Pos current, Pos next)
{
  Cell C = m->get[current.x][current.y];
  Cell N = m->get[next.x][next.y];

  _server_root_unlock(m,&C);
  _server_root_unlock(m,&N);
  
  // Order of cell unlock doesn't matter so long as jail is last unlocked
  cell_unlock(&C);
  cell_unlock(&N);

   /*Unlock in reverse order Unlock N*/
  /*if (N.object) object_unlock(N.object);*/
  /*if (N.player)*/
  /*{*/
    /*if (N.player->shovel) object_unlock(N.player->shovel);*/
    /*if (N.player->flag) object_unlock(N.player->flag);*/
    /*player_unlock(N.player);*/
  /*}*/
  
   /*Unlock C*/
  /*if (C.object) object_unlock(C.object);*/
  /*if (C.player)*/
  /*{*/
    /*if (C.player->shovel) object_unlock(C.player->shovel);*/
    /*if (C.player->flag) object_unlock(C.player->flag);*/
    /*player_unlock(C.player);*/
  /*}*/
  
}

// The cell is the root of all the objects. A cell root lock
// will lock all the players and objects nested in the cell.
// It will check if a player has dropped and if so, drop all change all the references.
extern void _server_root_lock(Maze*m, Cell* cell)
{
  if (cell->object){ object_lock(cell->object); }
  if (cell->player)
  { 
    player_lock(cell->player); 
    if (cell->player->shovel){ object_lock(cell->player->shovel); }
    if (cell->player->flag){ object_lock(cell->player->flag); }
    
    // Locked the player BUT he/she could be dropped
    // Cleanup references if dropped
    /*server_plist_read_lock(&m->players[cell->player->team]);    */
    /*if (cell->player->fd = -1)*/
    /*{*/
      /*_server_drop_handler(m,cell->player); */
      /*player_unlock(cell->player);*/
    /*}*/
    /*server_plist_unlock(&m->players[cell->player->team]);    */
  }
}

extern void _server_root_unlock(Maze*m, Cell*cell)
{
  // Unlock in reverse order Unlock cell
  if (cell->object) object_unlock(cell->object);
  if (cell->player)
  {
    if (cell->player->shovel) object_unlock(cell->player->shovel);
    if (cell->player->flag) object_unlock(cell->player->flag);
    player_unlock(cell->player);
  }
}

extern void server_maze_property_lock(Maze*m)
{
    pthread_mutex_lock(&m->state_lock);
}

extern void server_maze_property_unlock(Maze*m)
{
    pthread_mutex_unlock(&m->state_lock);
}

extern void object_lock(Object*object)
{
  pthread_mutex_lock(&(object->lock));
}

extern void object_unlock(Object*object)
{
  pthread_mutex_unlock(&(object->lock));
}

extern void player_lock(Player*player)
{
  pthread_mutex_lock(&(player->lock));
}

extern void player_unlock(Player*player)
{
  pthread_mutex_unlock(&(player->lock));
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

extern void server_plist_read_lock(Plist*plist)
{
  pthread_rwlock_rdlock(&plist->plist_wrlock);
}

extern void server_plist_write_lock(Plist*plist)
{
  pthread_rwlock_wrlock(&plist->plist_wrlock);
}

extern void server_plist_unlock(Plist*plist)
{
  pthread_rwlock_unlock(&plist->plist_wrlock);
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
     
      if (found != 0) { 
      continue;
      }

      else if ((query==0 && !cell_is_unoccupied(c)) || (query!=0 && cell_is_holding(c))) 
      {
        found = -1;
        cell_unlock(c);
      }
      else { 
        break;
      }
   }
   
   if (found != 0) 
   {
    return -1;
   }
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
extern int server_player_spawn(Maze* m, Player* player)
{
  int rc = 0;
  Cell* homecell;
  rc = server_find_empty_home_cell_and_lock(m, player->team, &homecell, player->id, 0); // 0=player query
  if (proto_debug() && rc<0) fprintf(stderr,"home size: %d\n",server_home_count_read(&m->home[player->team])); 
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
}

// Returns the id of the player with corresponding fd
// Only to be used by threads when they don't hold a write lock on
// the plist... i.e can't be used by add or drop
// (otherwise it will cause deadlock)
// Returns -1 if nothing is found
// Returns the id if found
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

// Drop a player by fd
// Returns -1 if the corresponding fd is not found
extern int server_plist_drop_player_by_fd(Maze*m, Plist*plist, int fd)
{
   int id;
   id = server_plist_find_player_by_fd(plist,fd);
   if (id == -1) return -1;
   
   server_plist_drop_player_by_id(m,plist,id);
   return id;
}

// Drop player by id (rather than the drop player by fd)
// Will always the drop the player in that position. if that player is dropped
// it will have no effect. This doesn't actually unset the player's cell and flags
// but just drops it from the the plist.  That has to be done by the drop handler
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
  plist->count--;
  pthread_rwlock_unlock(&plist->plist_wrlock);

}

/******************/
/* ACTION METHODS */
/* _* = unsafe    */
/******************/ 
extern int _server_action_drop_flag(Maze*m , Player* player)
{
    if (!player)                              return ERR_NO_PLAYER;
    //FIXME can't drop flag,take flag with you(jail/heaven)
    if (player->cell && player->cell->object) return ERR_NO_OBJECT; 
    if (!player->flag)                        return 1;  // no need to drop
    
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
  if (!player)         return ERR_NO_PLAYER;
  if (!player->shovel) return ERR_NO_OBJECT;
  
  int rc;
  Object * object = player->shovel;
  Cell * cell = player->cell;
  Cell * next;

  // find next cell and set its object
  rc = server_find_empty_home_cell_and_lock(m, object->team, &next, 0, 1); // 0=seed, 1=not-holding query
  if (rc < 0) return -1;
  _server_action_update_cell_and_player(m,object,next,0);  // move object and kill player link
  next->object = object;  // link object to cell

  object_unlock(object); // unlock object
  cell_unlock(next); // unlock this new cell

  // drop my shovel
  cell->object=0;
  player->shovel= 0;
  
  return 0;
}

extern int _server_action_drop_shovel(Maze*m , Player*player)
{
   if (!player)              return ERR_NO_PLAYER;
   if (player->cell->object) return ERR_CELL_OCCUPIED; // CAN'T DROP HERE
   if (!player->shovel)      return 1;       // no need to do more, shovel dropped

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
    if (!player) return ERR_NO_PLAYER;
    Cell* cell = player->cell;
    
    if (!player->cell->object) return ERR_NO_OBJECT;  // nothing to pickup
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
  if (!object) return ERR_NO_OBJECT;
  object->cell = newcell;
  return 0;
}

extern int _server_action_update_cell_and_player(Maze*m, Object* object, Cell* newcell, Player* player)
{
  if (!object) return ERR_NO_OBJECT;
  object->cell = newcell;
  object->player = player;
  return 0;
}

extern int _server_action_update_player(Maze*m, Player*player, Cell*newcell)
{
  if (!player) return ERR_NO_PLAYER;
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
  if (!currentcell->player) return ERR_NO_PLAYER;
  Player * player = currentcell->player;
  if (player->state == PLAYER_JAILED) return 1; // no need to jail
  
  Cell * nextcell;
  int rc;

  // find the next jail cell
  rc = server_find_empty_jail_cell_and_lock(m,opposite_team(player->team),&nextcell,player->id,0);
  if (rc < 0) return ERR_JAIL_FULL;

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
        server_maze_unlock(m, cell.pos, cell.pos); // unlock cell ... 
      }
   }

   return 1;
}

extern void _server_drop_handler(Maze*m, Player*player)
{
  // drop shovel and flag if any
  _server_action_player_reset_shovel(m,player);
  _server_action_drop_flag(m,player);

  // kill player-cell references
  Cell* cell = player->cell;
  if (cell) cell->player = 0;
  player->cell = 0;

  player_unlock(player);
}

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <pthread.h>
#include "net.h"
#include "protocol.h"
#include "protocol_session.h"
#include "game_maze.h"

extern void jail_init(Jail* jail, Pos min, Pos max, Team_Types team)
{
  bzero(jail,sizeof(Jail));
  jail->min.x = min.x;
  jail->min.y = min.y;
  jail->max.x = max.x;
  jail->max.y = max.y;
  jail->team  = team;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE ); //SET TYPE TO RECURSIVE i,e REENTRANT MUTEX
  pthread_mutex_init(&jail->jail_recursive_lock,&attr);
  pthread_mutexattr_destroy(&attr);  // after initialization this attr object doesn't affect the pthread  
}

extern void jail_lock(Jail * jail)
{
  pthread_mutex_lock(&(jail->jail_recursive_lock));
}

extern void jail_unlock(Jail * jail)
{
  pthread_mutex_unlock(&(jail->jail_recursive_lock));
}

extern void home_init(Home* home, Pos min, Pos max, Team_Types team)
{
  bzero(home,sizeof(Home));
  home->min.x = min.x;
  home->min.y = min.y;
  home->max.x = max.x;
  home->max.y = max.y;
  home->team  = team;
  home->count = 0;
  pthread_rwlock_init(&home->count_wrlock,NULL);
}

extern int home_count_read(Home* home)
{
  int count;
  pthread_rwlock_rdlock(&home->count_wrlock);
  count = home->count;
  pthread_rwlock_unlock(&home->count_wrlock);
  return count;
}

extern int home_count_increment(Home* home)
{
  int count;
  pthread_rwlock_wrlock(&home->count_wrlock);
  home->count--;
  count = home->count;
  pthread_rwlock_unlock(&home->count_wrlock);
  return count;
}

extern int home_count_decrement(Home* home)
{
  int count;
  pthread_rwlock_wrlock(&home->count_wrlock);
  home->count--;
  count = home->count;
  pthread_rwlock_unlock(&home->count_wrlock);
  return count;
}

extern void plist_init(Plist* plist, Team_Types team, int max_players )
{
   bzero(plist,sizeof(Plist));
   plist->count = 0;
   plist->next  = 0;
   plist->team  = team;
   plist->max   = max_players;
   pthread_rwlock_init(&(plist->plist_wrlock),NULL);
}

extern Cell_Types cell_calculate_type(char cell)
{
	switch(cell)
  {
		case ' ':return CELL_FLOOR;
		case '#':return CELL_WALL;
		case 'h':return CELL_HOME;
		case 'H':return CELL_HOME;
		case 'j':return CELL_JAIL;
		case 'J':return CELL_JAIL;
	}
	return 0;
}

extern Team_Types cell_calculate_turf(int x, int max)
{
	return x< (max/2) ?  TEAM_RED : TEAM_BLUE ;
}

extern Mutable_Types cell_calculate_mutable(char cell,int x,int y,int max_x,int max_y)
{
	if(cell=='#')
  {
		if(x==0)                return CELLTYPE_IMMUTABLE;
		else if(x==(max_x-1))   return CELLTYPE_IMMUTABLE;
		else if(y==0)           return CELLTYPE_IMMUTABLE;
		else if(y==(max_y-1))   return CELLTYPE_IMMUTABLE;
		else                    return CELLTYPE_MUTABLE;
	}
	return CELLTYPE_IMMUTABLE;
}

extern void cell_init( Cell* cell, int x, int y, Team_Types turf, Cell_Types type, Mutable_Types is_mutable)
{
    Mutable_Types is_really_mutable;
    is_really_mutable = CELLTYPE_IMMUTABLE;
    if (type == CELL_WALL && is_mutable) is_really_mutable = CELLTYPE_MUTABLE;    

    bzero(cell,sizeof(Cell));
    cell->pos.x = x;
    cell->pos.y = y;
    cell->turf = turf;
    cell->type = type;
    cell->is_mutable = is_really_mutable;
}


extern int cell_is_near(Cell* current, Cell* next)
{
  if (current->pos.x == next->pos.x)
  {
      if (next->pos.y == current->pos.y+1) return 1;
      if (next->pos.y == current->pos.y-1) return 1;
      return 0;
  }
  if (current->pos.y == next->pos.y)
  {
      if (next->pos.x == current->pos.x+1) return 1;
      if (next->pos.x == current->pos.x-1) return 1;
      return 0;
  }

  return 0;
}

extern int cell_is_unoccupied(Cell* cell)
{
    return (cell->cell_state == CELLSTATE_OCCUPIED || cell->cell_state == CELLSTATE_OCCUPIED_HOLDING );
}

extern int cell_is_walkable(Cell * cell)
{
    return (cell->type != CELL_WALL );
}

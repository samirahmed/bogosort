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
#include "game_commons.h"

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


extern void maze_init(Maze * m, int max_x, int max_y)
{
     bzero(m,sizeof(Maze));
     
     // Setup position variables
     m->max.x = (unsigned short int) max_x;
     m->max.y = (unsigned short int) max_y;
     m->min.x = 0;
     m->min.y = 0;

     // Configure objects for indexing
     m->objects[FLAG + 2*(TEAM_RED)].type    = FLAG;
     m->objects[FLAG + 2*(TEAM_RED)].team    = TEAM_RED;
     m->objects[FLAG + 2*(TEAM_BLUE)].type   = FLAG;
     m->objects[FLAG + 2*(TEAM_BLUE)].team   = TEAM_BLUE;
     m->objects[SHOVEL + 2*(TEAM_RED)].type  = SHOVEL;
     m->objects[SHOVEL + 2*(TEAM_RED)].team  = TEAM_RED;
     m->objects[SHOVEL + 2*(TEAM_BLUE)].type = SHOVEL;
     m->objects[SHOVEL + 2*(TEAM_BLUE)].team = TEAM_BLUE;
     
     // Setup locks
     pthread_rwlock_init(&m->wall_wrlock,NULL);
     pthread_rwlock_init(&m->object_wrlock,NULL);

     // Initialize cells
     m->get = (Cell **)malloc(m->max.x*(sizeof(Cell*)));
     if(m->get == NULL ) fprintf(stderr,"Unable to Initiale %d columns\n",m->max.x);

     unsigned int col;
     for( col = m->min.x; col < m->max.x ; col++ )
     {
        if( ( m->get[col]=(Cell*)malloc(m->max.y*sizeof(Cell))) == NULL ) 
        fprintf(stderr,"Unable to Initialize %d rows for %d column\n",m->max.y,col);
     }
     fprintf(stderr,"Successful Initialize %d col(x) by %d row(y) maze\n",m->max.x,m->max.y);

     // Initialize wall
     m->wall = (int **)(malloc(m->max.x*sizeof(int*)));
     if(m->get == NULL ) fprintf(stderr,"Unable to Initialize wall %d columns\n",m->max.x);

     col = 0;
     for( col = m->min.x; col < m->max.x ; col++ )
     {
        if( ( m->wall[col]=(int*)malloc(m->max.y*sizeof(int))) == NULL ) 
        fprintf(stderr,"Unable to Initialize walls %d rows for %d column\n",m->max.y,col);
     }
     fprintf(stderr,"Successful Initialize %d col(x) by %d row(y) maze walls \n",m->max.x,m->max.y);

}

//	|--->X Axis
//	|
// 	V
// 	Y Axis
void maze_fill_helper(Maze* map, char buffer[][MAX_COL_MAZE],int max_x, int max_y)
{
	// Fill cells
  int x,y;	
	for(y = map->min.y; y < map->max.y ; y++) 				  //Y Axis = Row
  {
    for(x= map->min.x ; x < map->max.x ; x++)        //X Axis = Column
    {
      cell_init(&(map->get[x][y]), 
                  x, 
                  y, 
                  cell_calculate_turf(x,map->max.x),
                  cell_calculate_type(buffer[y][x]), 
                  cell_calculate_mutable(buffer[y][x],x,y,max_x,max_y));
		}
  }

  int team;
  for ( team = 0; team < NUM_TEAMS; team++ )
  {
      // Set the jail
      Pos jail_min, jail_max,  home_min, home_max;
      jail_min.x = map->max.x;
      jail_min.y = map->max.y;
      home_min.x = map->max.x;
      home_min.y = map->max.y;
      
      jail_max.x = map->min.x;
      jail_max.y = map->min.y;
      home_max.x = map->min.x;
      home_max.y = map->min.y;
      
      Cell c;
      for( y = map->min.y; y < map->max.y ; y++) 				  
      {
        for( x = map->min.x + (team*map->max.x/2) ; x < map->max.x/2+(team*(map->max.x/2)) ; x++)       
        {
           c = map->get[x][y];
           if (c.type == CELL_JAIL)
           {
              if ( x < jail_min.x ) jail_min.x = x;
              if ( y < jail_min.y ) jail_min.y = y;
              if ( x > jail_max.x ) jail_max.x = x;
              if ( y > jail_max.y ) jail_max.y = y;
           }
           if (map->get[x][y].type == CELL_HOME)
           {
              if ( x < home_min.x ) home_min.x = x;
              if ( y < home_min.y ) home_min.y = y;
              if ( x > home_max.x ) home_max.x = x;
              if ( y > home_max.y ) home_max.y = y;
           }
        }
      }

      // have to set it one greater than it really is to allow standard looping in the future
      jail_max.x++;
      jail_max.y++;
      home_max.x++;
      home_max.y++;

      jail_init(&map->jail[team], jail_min, jail_max, team);
      home_init(&map->home[team], home_min, home_max, team);
      
      int max_players;
      max_players = (home_max.x - home_min.x) * ( home_max.y - home_min.y );
      max_players = max_players >= 50 ? max_players-10: max_players;
      plist_init(&map->players[team], team, max_players );
  }
}

extern int maze_build_from_file(Maze*map, char* filename)
{
  FILE* fp;
	char buffer[MAX_ROW_MAZE][MAX_COL_MAZE];		//This size buffer is okay for now
	int rowLen = 0; 			                      //Number of chars in one line of the file
	int colLen = 0; 			                      //Index for row into the buffer
	fp = fopen(filename,"r"); 		              //Open File

  // close the file and check for errors
  if(fp==NULL)  
  { 				
		fprintf(stderr,"Error opening file %s for reading\n",filename);
		return -1;
	}
	else
  { 					//Read in the file
		fgets(buffer[rowLen],MAX_COL_MAZE,fp);
		colLen = strlen(buffer[rowLen++]);
		while((fgets(buffer[rowLen++],MAX_COL_MAZE,fp))!=NULL);
		colLen--;
		rowLen--;		
		maze_init(map,colLen,rowLen);
		maze_fill_helper(map, buffer,colLen,rowLen);
	}
  //Close the file and check for error
	if((fclose(fp))!=0) fprintf(stderr,"Error closing file");
	
  return 1;
}

extern void maze_dump(Maze*map)
{
	int x,y;
	FILE* dumpfp;
	dumpfp = fopen("dumpfile.text","w");
	for( y = map->min.x; y < map->max.y; y++ )
  {
		for( x = map->min.y; x < map->max.x; x++ )
    {
      if(map->get[x][y].type==CELL_WALL)
			{
				fprintf(stdout,"#");
				fprintf(dumpfp,"#");
			}
			else if(map->get[x][y].type==CELL_FLOOR)
			{
				fprintf(stdout," ");
				fprintf(dumpfp," ");
			}
			else if(map->get[x][y].type==CELL_JAIL && map->get[x][y].turf==TEAM_RED)
			{
				fprintf(stdout,"j");
				fprintf(dumpfp,"j");
			}
			else if(map->get[x][y].type==CELL_HOME && map->get[x][y].turf==TEAM_RED)
			{
				fprintf(stdout,"h");
				fprintf(dumpfp,"h");
			}
			else if(map->get[x][y].type==CELL_JAIL && map->get[x][y].turf==TEAM_BLUE)
			{
				fprintf(stdout,"J");
				fprintf(dumpfp,"J");
			}
			else if(map->get[x][y].type==CELL_HOME && map->get[x][y].turf==TEAM_BLUE)
			{
				fprintf(stdout,"H");
				fprintf(dumpfp,"H");
			}
    }
		fprintf(stdout,"\n");
		fprintf(dumpfp,"\n");
	}
	close((int)dumpfp);
}

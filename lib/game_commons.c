#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_session.h"
#include "game_commons.h"
#include "types.h"

/******************/
/* UTIL   METHODS */
/******************/

extern Team_Types opposite_team(Team_Types team)
{
  return (team == TEAM_RED ? TEAM_BLUE: TEAM_RED);
}

extern long double tick()
{
  struct timeval tval;
  gettimeofday(&tval,NULL);
  return (long double)tval.tv_sec + (long double)(tval.tv_usec/1000000.0);
}

/******************/
/* OBJECT METHODS */
/******************/

extern int object_get_index(Team_Types team , Object_Types object)
{
    return team+(object*2);
}

extern Object* object_get(Maze*m, Object_Types object,Team_Types team)
{
  return &m->objects[object_get_index(team,object)];
}

void object_init(Maze*m, Object_Types object, Team_Types team)
{
  Object* oo  = object_get(m,object,team);
  bzero(oo,sizeof(Object));
  oo->type = object;
  oo->team = team;
  if (oo->type == OBJECT_SHOVEL) 
  {
    oo->visiblity = 1;
    int rx = m->home[team].max.x-1;
    int ry = m->home[team].max.y-1;
    Cell *cell = &m->get[rx][ry];
    cell->object = oo;
    oo->cell = cell;
  }
  if (oo->type == OBJECT_FLAG)
  {
    oo->visiblity = 0;
    int rx, ry, found;
    found = 0;
    while( found==0 )
    {
      // Hard coded 
      rx = 100*(team)+49;
      ry = 99;
      
      // Uncomment for randomized starting position
      int size =(m->max.x / NUM_TEAMS);
      rx = randint()%(size+(team*size));
      ry = randint()%(m->max.y);
      
      if (rx >= m->max.x || ry >= m->max.y ) continue;
      if (m->get[rx][ry].type != CELL_FLOOR) continue;
      
      // found viable cell to place this object in 
      Cell* cell = &m->get[rx][ry];
      cell->object = oo;
      oo->cell = cell;
      found = 1;
    }
  }

  pthread_mutex_init(&oo->lock,NULL);
}

/******************/
/* PLAYER METHODS */
/******************/

// initialize player bits
// sets the fd = -1 until it is assigned
extern void player_init(Player* player)
{
  bzero(player,sizeof(Player));
  player->fd=-1;
  pthread_mutex_init(&player->lock,NULL);
}

extern void jail_init(Jail* jail, Pos min, Pos max, Team_Types team)
{
  bzero(jail,sizeof(Jail));
  jail->min.x = min.x;
  jail->min.y = min.y;
  jail->max.x = max.x;
  jail->max.y = max.y;
  jail->team  = team;
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

extern void plist_init(Plist* plist, Team_Types team, int max_players )
{
   int ii;
   bzero(plist,sizeof(Plist));
   plist->count = 0;
   plist->next  = 0;
   plist->team  = team;
   plist->max   = max_players;
   pthread_rwlock_init(&(plist->plist_wrlock),NULL);
   for (ii=0;ii<plist->max;ii++)
   {
     plist->at[ii].fd = -1;
   }
}

/*********************/
/* HOME BOUNDS CHECK */
/*********************/

extern int home_contains(Pos* query, Home*home)
{
  return ( (query->x < home->max.x) && 
           (query->y < home->max.y) && 
           (query->x >= home->min.x) && 
           (query->y >= home->min.y) );
}

/*********************/
/* MAZE LOCK METHODS */
/*********************/

extern long long  maze_next_read_then_increment(Maze*m)
{
  long long result;
  pthread_mutex_lock(&m->next_lock);
  result = m->next;
  m->next++;
  pthread_mutex_unlock(&m->next_lock);
  return result;
}

extern long long  maze_next_read_only(Maze*m)
{
  long long result;
  pthread_mutex_lock(&m->next_lock);
  result = m->next;
  pthread_mutex_unlock(&m->next_lock);
  return result;
}

extern void maze_set_state(Maze*m, Game_State_Types state)
{
  pthread_mutex_lock(&(m->state_lock));
  m->current_game_state = state;
  pthread_mutex_unlock(&(m->state_lock));
}

extern Game_State_Types  maze_get_state(Maze *m)
{
  Game_State_Types current_game_state;
  pthread_mutex_lock(&(m->state_lock));
  current_game_state = m->current_game_state;
  pthread_mutex_unlock(&(m->state_lock));
  return current_game_state;
}

/****************/
/* CELL METHODS */
/****************/

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
    pthread_mutex_init(&cell->lock,NULL);
}

extern void cell_lock(Cell* cell)
{
  pthread_mutex_lock(&(cell->lock));
  cell->thread = (unsigned int)(size_t)pthread_self();
}

extern void cell_unlock(Cell* cell)
{
  if ( cell->thread == (unsigned int)(size_t) pthread_self())cell->thread = 0;
  pthread_mutex_unlock(&(cell->lock));
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
   return ( cell->player ? 0: 1 );
}

extern int cell_is_holding(Cell* cell)
{
   return ( cell->object ? 1: 0 );
}

extern int cell_is_walkable(Cell * cell)
{
    return (cell->type != CELL_WALL );
}

/***********************/
/* MAZE HELPER METHODS */
/***********************/

extern Cell_Types maze_calculate_cell_type(char cell)
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

extern Team_Types maze_calculate_turf(int x, int max)
{
	return x< (max/2) ?  TEAM_RED : TEAM_BLUE ;
}

extern Mutable_Types maze_calculate_mutable(char cell,int x,int y,int max_x,int max_y)
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

/****************/
/* MAZE METHODS */
/****************/

extern void maze_init(Maze * m, int max_x, int max_y)
{
     bzero(m,sizeof(Maze));
     
     // Setup position variables
     m->max.x = (unsigned short int) max_x;
     m->max.y = (unsigned short int) max_y;
     m->min.x = 0;
     m->min.y = 0;

     // Setup locks
     pthread_rwlock_init(&m->wall_wrlock,NULL);
     pthread_rwlock_init(&m->object_wrlock,NULL);
     pthread_mutex_init(&m->state_lock,NULL);
     pthread_mutex_init(&m->current_lock,NULL);
     pthread_mutex_init(&m->next_lock,NULL);
     pthread_cond_init(&m->current_cond,NULL);
     m->current_game_state = GAME_STATE_WAITING; 

     // Initialize cells
     m->get = (Cell **)malloc(m->max.x*(sizeof(Cell*)));
     if(m->get == NULL ) fprintf(stderr,"Unable to Initiale %d columns\n",m->max.x);

     unsigned int col;
     for( col = m->min.x; col < m->max.x ; col++ )
     {
        if( ( m->get[col]=(Cell*)malloc(m->max.y*sizeof(Cell))) == NULL ) 
        fprintf(stderr,"Unable to Initialize %d rows for %d column\n",m->max.y,col);
     }
     if (proto_debug()) fprintf(stderr,"Successful Initialize %d col(x) by %d row(y) maze\n",m->max.x,m->max.y);

     // Initialize wall
     m->wall = (int **)(malloc(m->max.x*sizeof(int*)));
     if(m->get == NULL ) fprintf(stderr,"Unable to Initialize wall %d columns\n",m->max.x);

     col = 0;
     for( col = m->min.x; col < m->max.x ; col++ )
     {
        if( ( m->wall[col]=(int*)malloc(m->max.y*sizeof(int))) == NULL ) 
        fprintf(stderr,"Unable to Initialize walls %d rows for %d column\n",m->max.y,col);
     }
     if (proto_debug() ) fprintf(stderr,"Successful Initialize %d col(x) by %d row(y) maze walls \n",m->max.x,m->max.y);

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
                  maze_calculate_turf(x,map->max.x),
                  maze_calculate_cell_type(buffer[y][x]), 
                  maze_calculate_mutable(buffer[y][x],x,y,max_x,max_y));
		}
  }
  
  // Fill walls with zeros
  for(x = map->min.x; x<map->max.x ; x++ ) for(y= map->min.y; y<map->max.y ; y++ ) map->wall[x][y]=0;

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
      object_init(map,OBJECT_SHOVEL,team);
      object_init(map,OBJECT_FLAG,team);
  }

  // Set cell Wall mutablity
  for(team=0;team<NUM_TEAMS;team++)
  {
    int xx,yy;
    
    for(xx=map->home[team].min.x-1; xx<=map->home[team].max.x ;xx++ )
    for(yy=map->home[team].min.y-1; yy<=map->home[team].max.y ;yy++ )
    if (map->get[xx][yy].type == CELL_WALL) map->get[xx][yy].is_mutable = CELLTYPE_IMMUTABLE;
    
    for(xx=map->jail[team].min.x-1; xx<=map->jail[team].max.x ;xx++ )
    for(yy=map->jail[team].min.y-1; yy<=map->jail[team].max.y ;yy++ )
    if (map->get[xx][yy].type == CELL_WALL) map->get[xx][yy].is_mutable = CELLTYPE_IMMUTABLE;
  }
  
}

extern int maze_build_from_file(Maze*map, char* filename)
{
	int rowLen,colLen; 
  rowLen = 0; 			                      //Number of chars in one line of the file
	colLen = 0; 			                      //Index for row into the buffer
	char buffer[MAX_ROW_MAZE][MAX_COL_MAZE]; //This size buffer is okay for now
  FILE* fp;
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

extern void maze_destroy(Maze*maze)
{
     int col;
     for( col = maze->min.x; col < maze->max.x ; col++ )
     {
        free( maze->wall[col] ); 
        free( maze->get[col] );
     }
     free( maze->wall);
     free( maze->get);
     
     if(proto_debug()) fprintf(stderr,"cleaned up maze struct\n");
     bzero(maze,sizeof(Maze)); 
}

extern void maze_object_dump(Object*object,FILE*fd,int indent)
{
  fprintf(fd,"\n\t");
  if( indent>1 ) fprintf(fd,"\t");
  fprintf(fd,"[" COLOR_OKPINK "object" COLOR_END " team:%1d type:%1d]",
          object->team,object->type);
}

extern void maze_player_dump(Player*player,FILE*fd)
{
  fprintf(fd,"\n\t["COLOR_OKGREEN"player "COLOR_END"team:%1d id:%03d state:%1d]",
          player->team,player->id,player->state);
  if (player->shovel) maze_object_dump(player->shovel,fd,2);
  if (player->flag) maze_object_dump(player->flag,fd,2);
}

extern void maze_cell_dump(Cell*c, FILE*fd)
{
    fprintf(fd,"[cell %03d,%03d type:%1d]",c->pos.x,c->pos.y,c->type);
    if (c->object) maze_object_dump(c->object,fd,1);
    if (c->player) maze_player_dump(c->player,fd);
    fprintf(fd,"\n");
}

extern void maze_text_dump(Maze*m, char* filename)
{
	int x,y;
	FILE* fd;
  if (strncmp(filename,"console",sizeof("console")-1)==0) fd = stderr;
  else fd =fopen(filename,"w");

  fprintf(fd,"[MAZE STATE:%1d]\n",m->current_game_state);

  for( y = m->min.x; y < m->max.y; y++ )
  {
		for( x = m->min.y; x < m->max.x; x++ )
    {
      Cell*cell= &m->get[x][y];
      if ( cell->player || cell->object )
      maze_cell_dump(cell, fd);
    }
  }
  fprintf(fd,"\n");
  fprintf(stderr,"Server Text Dump Completed\n");
  fflush(fd);
  close((int)(size_t)fd);
}

extern void maze_ascii_dump(Maze*map, char* filename)
{
	int x,y;
	FILE* dumpfp;
  if (strncmp(filename,"console",sizeof("console")-1)==0) dumpfp = stderr; 
  else dumpfp =fopen(filename,"w");

  for( y = map->min.x; y < map->max.y; y++ )
  {
		for( x = map->min.y; x < map->max.x; x++ )
    {
      if(map->get[x][y].player)
      {
        Player*player = map->get[x][y].player;
        if (player->team == TEAM_RED)
        {
				  fprintf(dumpfp,"R");
        }
        else
        {
				  fprintf(dumpfp,"B");
        }
      }
      else if(map->get[x][y].type==CELL_WALL)
			{
				fprintf(dumpfp,"#");
			}
			else if(map->get[x][y].type==CELL_FLOOR)
			{
				fprintf(dumpfp," ");
			}
			else if(map->get[x][y].type==CELL_JAIL && map->get[x][y].turf==TEAM_RED)
			{
				fprintf(dumpfp,"j");
			}
			else if(map->get[x][y].type==CELL_HOME && map->get[x][y].turf==TEAM_RED)
			{
				fprintf(dumpfp,"h");
			}
			else if(map->get[x][y].type==CELL_JAIL && map->get[x][y].turf==TEAM_BLUE)
			{
				fprintf(dumpfp,"J");
			}
			else if(map->get[x][y].type==CELL_HOME && map->get[x][y].turf==TEAM_BLUE)
			{
				fprintf(dumpfp,"H");
			}
    }
		fprintf(dumpfp,"\n");
	}
  fprintf(stderr,"Server ASCII dump completed\n");
  fflush(dumpfp);
	close((int)(size_t)dumpfp);
}

/****************************/
/* MARSHALL AND COMPRESSION */
/****************************/

extern int decompress_is_ignoreable(int * compressed)
{
  return ( *compressed & 0x80000000 ? 0 : 1 ) ;
}

extern void compress_player(Player* player, int* compressed , Player_Update_Types update_type )
{
   *compressed = 0;
   *compressed = ((0x000000ff & (player->cell->pos.y))          |
                  (0x0000ff00 & ((player->cell->pos.x) << 8 ))  |
                  (0x00ff0000 & ( player->id           << 16 )) |
                  (0x01000000 & ( player->state        << 24 )) |
                  (0x02000000 & ( player->team         << 25 )) |
                  (0x0c000000 & ( update_type          << 26 )) |
                  (0x80000000)); // do not ignore bit
}

extern void decompress_player(Player* player, int* compressed, Player_Update_Types *update_type)
{
  player->client_position.y =  (unsigned)( 0x000000ff & *compressed );
  player->client_position.x =  (unsigned)((0x0000ff00 & *compressed ) >> 8  );
  player->id                =  (unsigned)((0x00ff0000 & *compressed ) >> 16 );
  
  player->state   = (Player_State_Types )((0x01000000 & *compressed ) >> 24 );
  player->team    = (Team_Types         )((0x02000000 & *compressed ) >> 25 );
  *update_type    = (Player_Update_Types)((0x0c000000 & *compressed ) >> 26 );
}

extern void compress_object(Object* object, int* compressed)
{
   *compressed = 0;
   *compressed |=  (0x000000ff & ( object->cell->pos.y));
   *compressed |=  (0x0000ff00 & ((object->cell->pos.x) << 8 ));
   *compressed |=  (0x04000000 & ((object->type)        << 26 ));
   *compressed |=  (0x08000000 & ((object->team)        << 27 ));
   *compressed |=  (0x80000000); // do not ignore bit

   if (object->player)
   {
      *compressed |= (0x00ff0000 & ( object->player->id)   << 16);
      *compressed |= (0x01000000 & ( object->player->team) << 24);
      *compressed |= (0x02000000 ); // has player
   }
}

extern void decompress_object(Object* object, int* compressed)
{
  object->client_position.y =  (unsigned)( 0x000000ff & *compressed );
  object->client_position.x =  (unsigned)((0x0000ff00 & *compressed ) >> 8  );
  object->type =           (Object_Types)((0x04000000 & *compressed ) >> 26 );
  object->team =             (Team_Types)((0x08000000 & *compressed ) >> 27 );
  object->client_has_player =  (unsigned)((0x02000000 & *compressed ) >> 25 );

  if (object->client_has_player)
  {
     object->client_player_id =      (unsigned)((0x00ff0000 & *compressed ) >> 16 );
     object->client_player_team =  (Team_Types)((0x01000000 & *compressed ) >> 24 );
  }

}

extern void compress_game_state(Game_State_Types gstate, int* compressed)
{
  *compressed &=  0xfff8ffff; // clear state bits
  *compressed |= (0x00070000 & (gstate << 16)); // write state bits
}

extern int decompress_game_state(Game_State_Types *gstate, int* compressed)
{
  int  val;
  val = (((unsigned)(*compressed & 0x00070000)) >> 16) ;
  if ( val>GAME_STATE_ERROR ) return -1;
  *gstate = (Game_State_Types)val;
  return 0;
}

extern void compress_broken_wall(Pos * position, int* compressed)
{
   *compressed &= 0x7fff0000;   // clear used bits
   *compressed |=  (0x000000ff & ( position->y));
   *compressed |=  (0x0000ff00 & ( position->x << 8 ));
   *compressed |=  (0x80000000); // set do not ignore
}

extern void decompress_broken_wall(Pos * position, int* compressed)
{
  position->y =  (unsigned)( 0x000000ff & *compressed );
  position->x =  (unsigned)((0x0000ff00 & *compressed ) >> 8  );
}


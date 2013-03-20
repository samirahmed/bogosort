
#include "maze.h"

extern cell_init(Cell* cell, int x, int y, Team turf, Cell_Types type, Mutable_Types is_mutable)
{
    Mutable_Types is_really_mutable;
    is_really_mutable = CELLTYPE_IMMUTABLE;
    if (type == CELL_WALL && is_mutable) is_really_mutable = CELLTYPE_MUTABLE;    

    bzero(cell,sizeof(Cell));
    cell->x = x;
    cell->y = y;
    cell->turf = turf;
    cell->type = type;
    cell->is_mutable = is_really_mutable;
    return cell;
}

extern int cell_is_unoccupied(Cell* cell)
{
    return (cell->cell_state == CELLSTATE_OCCUPIED || cell->cell_state == CELLSTATE_OCCUPIED_HOLDING );
}

extern int cell_is_walkable_type(Cell * cell)
{
    return (cell->type != CELL_WALL );
}

extern void cell_unmarshall_from_header(Cell * cell, Proto_Msg_Hdr *hdr)
{
    short int x;
    short int y;
    x = (unsigned)((0xffff0000 & hdr->pstate.v0.raw)>>16);
    y = (unsigned)((0x0000ffff & hdr->pstate.v0.raw));

    cell->type =        (Cell_Types)        (unsigned)((0x0000000f & (hdr->pstate.v1.raw)));
    cell->turf =        (Team_Types)        (unsigned)((0x000000f0 & (hdr->pstate.v1.raw))>>4);
    cell->player_type = (Team_Types)        (unsigned)((0x00000f00 & (hdr->pstate.v1.raw))>>8);
    cell->object_type = (Object_Types)      (unsigned)((0x0000f000 & (hdr->pstate.v1.raw))>>12);
    cell->cell_state =  (Cell_State_Types)  (unsigned)((0x000f0000 & (hdr->pstate.v1.raw))>>16);
    cell->is_mutable =  (Mutable_Types)     (unsigned)((0x00f00000 & (hdr->pstate.v1.raw))>>20);
}

extern void cell_marshall_into_header(Cell * cell, Proto_Msg_Hdr * hdr)
{
    unsigned int position;
    position = (0xffff0000 & (cell->x<<16))|(0x0000ffff & cell->y);

    unsigned int cellinfo;
    cellinfo = 
               (0x0000000f & (cell->type )) |
               (0x000000f0 & (cell->turf << 4)) | 
               (0x00000f00 & (cell->player_type << 8)) | 
               (0x0000f000 & (cell->object_type << 12) ) | 
               (0x000f0000 & (cell->cell_state << 16) ) | 
               (0x00f00000 & (cell->is_mutable << 20) );
    hdr->pstate.v0.raw = position;
    hdr->pstate.v1.raw = cellinfo;
}

extern void maze_init(Maze * m, int max_x, int max_y)
{
     m->max_x = (unsigned short int) max_x;
     m->max_y = (unsigned short int) max_y;
    
     m->pos = (Cell **)malloc(m->max_x*(sizeof(Cell*)));
     if(m->pos == NULL ) fprintf(stderr,"Unable to Initiale %d columns",m->max_x);

     unsigned short int col;
     for( col = 0; i < m->max_x ; col++ )
     {
        if( ( pos[col]=(Cell*)malloc(m->max_y*sizeof(Cell)) == NULL ) 
        fprintf(stderr,"Unable to Initialize %d rows for %d column",m->max_y,col);
     }
     fprintf(stderr,"Successful Initialize %d col(x) by %d row(y) maze",m->max_x,m->max_y);
}

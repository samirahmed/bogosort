#ifndef __DAGAME_MAZE_H__
#define __DAGAME_MAZE_H__
/******************************************************************************
* Copyright (C) 2013 by Katsu Kawakami, Will Seltzer, Samir Ahmed, 
* Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <pthread.h>
#include "protocol.h"
#include "protocol_session.h"


typedef enum{
    CELLTYPE_IMMUTABLE,
    CELLTYPE_MUTABLE
} Mutable_Types;

typedef enum{
    TEAM_RED,
    TEAM_BLUE
} Team_Types;

typedef enum{
    OBJECT_SHOVEL,
    OBJECT_RED_FLAG,
    OBJECT_BLUE_FLAG
} Object_Types;

typedef enum{
    CELL_WALL,
    CELL_FLOOR,
    CELL_JAIL,
    CELL_HOME
} Cell_Types;

typedef enum{
    CELLSTATE_EMPTY,
    CELLSTATE_OCCUPIED,
    CELLSTATE_HOLDING,
    CELLSTATE_OCCUPIED_HOLDING
} Cell_State_Types;

typedef enum{
    FREE,
    JAILED
} Player_State_Types;

typedef struct{
    
    short int        x;
    short int        y;
    Cell_Types       type;
    Cell_State_Types cell_state;
    Team             turf;
    Team             player_type;
    Mutable_Types    is_mutable;
    Object_Types     object_type;
    //Player*       player;
    //Object*       object;

} Cell;

typedef struct{
    Cell[][] pos;
    unsigned short int max_x;
    unsigned short int max_y;
} Maze;

extern void cell_init(Cell* cell,int x, int y, Team turf, Cell_Types type, Mutable_Types is_mutable);
extern int cell_is_unoccupied(Cell* cell);
extern int cell_is_walkable_type(Cell * cell);
extern void cell_unmarshall_from_header(Cell * cell, Proto_Msg_Hdr *hdr);
extern void cell_marshall_into_header(Cell * cell, Proto_Msg_Hdr * hdr);

extern void maze_init(int max_x, int max_y);

#endif

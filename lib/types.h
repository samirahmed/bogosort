#ifndef __DA_GAME_TYPES_H__
#define __DA_GAME_TYPES_H__
/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Samir Ahmed Boston University
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

#include <stdint.h>

/* NETWORK TYPES */

/* native word sized unsigned and signed values */
typedef unsigned long uval;
typedef long sval;


/* GAME TYPES */
typedef enum{
    DROP_FLAG,
    DROP_SHOVEL,
    PICKUP_FLAG,
    PICKUP_SHOVEL
} Action;

typedef enum{
    TEAM_RED,
    TEAM_BLUE
} Team_Types;

typedef enum{
    VISIBLE,
    INVISIBLE
} Visible_Types;

typedef enum{
    FLAG,
    SHOVEL
} Object_Types; 

typedef enum{
    FREE,
    JAILED
} Player_State_Types; 

typedef enum{
    CELLTYPE_IMMUTABLE,
    CELLTYPE_MUTABLE
} Mutable_Types;

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

#endif /* __DA_GAME_TYPES_H__ */

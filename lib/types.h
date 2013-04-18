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

#define PTHREAD_STACK_SIZE 16384

/* NETWORK TYPES */

/* native word sized unsigned and signed values */

typedef unsigned long uval;
typedef long sval;

typedef enum{
    ERR_NOOP            = -20,
    ERR_NO_PLAYER       = -21,
    ERR_NO_OBJECT       = -22,
    ERR_CELL_HOLDING    = -40,
    ERR_CELL_OCCUPIED   = -41,
    ERR_BAD_PLAYER_ID   = -60,
    ERR_BAD_CURRENT_CEL = -61,
    ERR_BAD_NEXT_CELL   = -62,
    ERR_BAD_ACTION      = -63,
    ERR_JAIL_FULL       = -80
}
Game_Error_Types;

typedef enum{
    GAME_STATE_UNCHANGED =  0,
    GAME_STATE_WAITING   =  1,
    GAME_STATE_ACTIVE    =  2,
    GAME_STATE_RED_WIN   =  3,
    GAME_STATE_BLUE_WIN  =  4,
    GAME_STATE_ERROR     =  5
} Game_State_Types;

typedef enum{
    PLAYER_UNCHANGED =  0,
    PLAYER_ADDED     =  1,
    PLAYER_DROPPED   =  2
} Player_Update_Types;

/* GAME TYPES */
typedef enum{
    ACTION_NOOP,
    ACTION_MOVE,
    ACTION_DROP_FLAG,
    ACTION_DROP_SHOVEL,
    ACTION_PICKUP_FLAG,
    ACTION_PICKUP_SHOVEL
} Action_Types;

typedef enum{
    TEAM_RED,
    TEAM_BLUE
} Team_Types;

typedef enum{
    OBJECT_VISIBLE,
    OBJECT_INVISIBLE
} Visible_Types;

typedef enum{
    OBJECT_FLAG,
    OBJECT_SHOVEL
} Object_Types; 

typedef enum{
    PLAYER_FREE,
    PLAYER_JAILED
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

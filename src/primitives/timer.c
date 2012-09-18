/* Copyright (C) 2009 Papavasileiou Dimitris                             
 *                                                                      
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or    
 * (at your option) any later version.                                  
 *                                                                      
 * This program is distributed in the hope that it will be useful,      
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
 * GNU General Public License for more details.                         
 *                                                                      
 * You should have received a copy of the GNU General Public License    
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "timer.h"

@implementation Timer

-(Timer *) init
{
    self = [super init];

    self->tick = LUA_REFNIL;    
    self->period = 1;
    
    return self;
}

-(void) toggle
{
    [super toggle];

    if (self->linked) {
	clock_gettime (CLOCK_REALTIME, &self->checkpoint);
	self->elapsed = 0;
	self->delta = 0;
	self->count = 0;
    }
}

-(void) free
{
     luaL_unref (_L, LUA_REGISTRYINDEX, self->tick);

     [super free];
}

-(void) tick
{
    struct timespec time;

    clock_gettime (CLOCK_REALTIME, &time);   

    self->delta = time.tv_sec - self->checkpoint.tv_sec +
	         (time.tv_nsec - self->checkpoint.tv_nsec) / 1e9;

    if (self->delta > self->period) {
	self->elapsed += self->delta;
	self->checkpoint = time;
	self->count += 1;

	t_push_userdata (_L, 1, self);
	lua_pushnumber (_L, self->count);
	lua_pushnumber (_L, self->delta);
	lua_pushnumber (_L, self->elapsed);

	t_call_hook (_L, self->tick, 4, 0);
    }
}

-(void) begin
{
    [self tick];
    [super begin];
}

-(void) stepBy: (double) h at: (double) t
{
    [self tick];
    [super stepBy: h at: t];
}

-(void) prepare
{
    [self tick];
    [super prepare];
}

-(void) traverse
{
    [self tick];
    [super traverse];
}

-(void) finish
{
    [self tick];
    [super finish];
}

-(int) _get_period
{
    lua_pushnumber (_L, self->period);

    return 1;
}

-(int) _get_tick
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->tick);

    return 1;
}

-(int) _get_state
{
    lua_newtable (_L);

    lua_pushnumber (_L, self->count);
    lua_rawseti (_L, -2, 1);
    lua_pushnumber (_L, self->elapsed);
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(void) _set_period
{
    self->period = lua_tonumber (_L, 3);
}

-(void) _set_tick
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->tick);
    self->tick = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_state
{
    T_WARN_READONLY;
}

@end

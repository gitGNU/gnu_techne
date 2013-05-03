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

#ifndef _TIMER_H_
#define _TIMER_H_

#include <lua.h>
#include <time.h>
#include "dynamic.h"

@interface Timer: Dynamic {
    struct timespec checkpoint;

    double period, elapsed, delta, count;
    int tick, shortcircuit;
}

-(void) tick;

-(int) _get_period;
-(int) _get_tick;

-(void) _set_period;
-(void) _set_tick;

-(int) _get_count;
-(int) _get_elapsed;

-(void) _set_count;
-(void) _set_elapsed;

@end

#endif

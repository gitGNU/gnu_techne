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

#ifndef _DOUBLEHINGE_H_
#define _DOUBLEHINGE_H_

#include <lua.h>
#include <ode/ode.h>
#include "joint.h"

@interface Doublehinge: Joint {
@public    
    double axis[3], anchors[2][3];
    double tolerance;
}

-(int) _get_anchors;
-(int) _get_axis;
-(int) _get_tolerance;

-(void) _set_axis;
-(void) _set_anchors;
-(void) _set_tolerance;

@end

#endif

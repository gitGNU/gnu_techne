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

#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

#include "profiling.h"
#include "transform.h"

@interface Graphic: Transform {
@public
    int draw;

    t_GPUProfilingInterval graphics;
}

-(void) draw: (int)frame;
-(int) _get_graphics;
-(void) _set_graphics;
-(int) _get_draw;
-(void) _set_draw;

@end

#endif

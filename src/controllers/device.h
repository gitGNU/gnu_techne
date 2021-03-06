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

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "event.h"

@interface Device: Event {
    int buttonpress, buttonrelease, absolute, relative;
}

-(int) _get_buttonpress;
-(int) _get_buttonrelease;
-(int) _get_absolute;
-(int) _get_relative;

-(void) _set_buttonpress;
-(void) _set_buttonrelease;
-(void) _set_absolute;
-(void) _set_relative;

@end

#endif

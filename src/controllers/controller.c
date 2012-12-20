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
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include "techne.h"
#include "controller.h"

@implementation Controller

-(void) initWithDevice: (const char *)name
{
    [super init];

    self->device = open(name, O_NONBLOCK | O_RDONLY);
}

-(void) input
{
    struct input_event events[64];
    int i, n;
    
    n = read(self->device, events, 64 * sizeof(struct input_event));

    if (n > 0) {
        assert (n % sizeof(struct input_event) == 0);

        /* Fire the bindings based on the event type. */
    
        for (i = 0 ; i < n / sizeof(struct input_event) ; i += 1) {
            switch(events[i].type) {
            case EV_KEY:
                t_pushuserdata (_L, 1, self);
                lua_pushnumber (_L, events[i].code);
        
                if (events[i].value == 1 || events[i].value == 2) {
                    t_callhook (_L, self->buttonpress, 2, 0);
                } else {
                    t_callhook (_L, self->buttonrelease, 2, 0);
                }

                break;                  
            case EV_REL:
            case EV_ABS:
                t_pushuserdata (_L, 1, self);
		
                lua_pushnumber (_L, events[i].code);
                lua_pushnumber (_L, events[i].value);

                t_callhook (_L, self->motion, 3, 0);

                break;
            }
        }
    }

    [super input];
}

-(void) free
{
    close(self->device);

    [super free];
}

@end
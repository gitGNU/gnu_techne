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

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "grass.h"

static const char *sources[2];

@implementation Grass

+(void)initialize
{
#include "glsl/grass_tesselation_control.h"
#include "glsl/grass_tesselation_evaluation.h"

    sources[0] = glsl_grass_tesselation_control;
    sources[1] = glsl_grass_tesselation_evaluation;
}

-(void)init
{
    float texels[16 * 16 * 2];
    int i, j;

    for (i = 0 ; i < 16 ; i += 1) {
        for (j = 0 ; j < 16 ; j += 1) {
            texels[2 * ((16 * i) + j) + 0] = (i / 16.0) * (j / 16.0);
            texels[2 * ((16 * i) + j) + 1] = (j / 16.0);
        }
    }
    
    [super init];

    glGenTextures (1, &self->deflections);
    glBindTexture(GL_TEXTURE_RECTANGLE, self->deflections);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RG, 16, 16, 0, GL_RG,
                 GL_FLOAT, texels);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

-(void)free
{
    glDeleteTextures (1, &self->deflections);

    [super free];
}
    
-(const char **)implementation
{
    return sources;
}

-(void) updateWithProgram: (unsigned int)name andIndex: (int)i
{
    unsigned int l;
    
    l = glGetUniformLocation (name, "deflections");
    
    [super updateWithProgram: name andIndex: i];
}

@end

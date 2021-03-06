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

#include <ode/ode.h>
#include "gl.h"

#include "techne.h"
#include "polyhedron.h"

@implementation Polyhedron

-(void) init
{
    self->references[0] = LUA_REFNIL;
    self->references[1] = LUA_REFNIL;

    self->data = dGeomTriMeshDataCreate();
    self->geom = dCreateTriMesh (NULL, self->data, NULL, NULL, NULL);
    dGeomSetData (self->geom, self);

    [super init];
}

-(void) update
{
    if (self->vertices && self->indices) {
        if (self->vertices->type == ARRAY_TFLOAT) {
            dGeomTriMeshDataBuildSingle (self->data,
                                         self->vertices->values.floats,
                                         3 * sizeof(float),
                                         self->vertices->size[0],
                                         self->indices->values.ints,
                                         self->indices->size[0] *
                                         self->indices->size[1],
                                         3 * sizeof(int));
        } else {
            dGeomTriMeshDataBuildDouble (self->data,
                                         self->vertices->values.doubles,
                                         3 * sizeof(double),
                                         self->vertices->size[0],
                                         self->indices->values.ints,
                                         self->indices->size[0] *
                                         self->indices->size[1],
                                         3 * sizeof(int));
        }
    } else {
        dGeomTriMeshDataBuildSingle (self->data, NULL, 0, 0, NULL, 0, 0);
    }
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[0]);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[1]);

    dGeomTriMeshDataDestroy (self->data);
}

-(int) _get_vertices
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->references[0]);

    return 1;
}

-(void) _set_vertices
{
    array_Array *array;

    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[0]);
    self->vertices = NULL;

    array = array_testarray (_L, 3);

    if (array) {
        if (array->type != ARRAY_TFLOAT && array->type != ARRAY_TDOUBLE) {
            t_print_error("Array specified for vertex data is of "
                          "unsuitable type.\n");
            abort();
        }

        if (array->rank != 2 || array->size[1] == 3) {
            t_print_error("Array specified for vertex data is of "
                          "unsuitable rank or size.\n");
            abort();
        }

        self->vertices = array;

        [self update];
    } else if (!lua_isnil (_L, 3)) {
        lua_pushnil (_L);
    }

    self->references[0] = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(int) _get_indices
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->references[1]);

    return 1;
}

-(void) _set_indices
{
    array_Array *array;

    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[1]);
    self->indices = NULL;

    array = array_testcompatible (_L, 3, ARRAY_TYPE, ARRAY_TINT);

    if (array) {
        if (array->rank != 2 || array->size[1] != 3) {
            t_print_error("Array specified for index data is of "
                          "unsuitable rank or size.\n");
            abort();
        }

        self->indices = array;

        [self update];
    } else if (!lua_isnil (_L, 3)) {
        /* If array was incompatible push nil for the reference
         * below. */

        lua_pushnil (_L);
    }

    self->references[1] = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end

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

#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "graphics.h"
#include "shape.h"
#include "shader.h"

static int next_attribute(lua_State *L)
{
    Shape *shape;
    shape_Buffer *b;

    shape = t_check_node (L, 1, [Shape class]);

    if (lua_isnil (L, 2)) {
	b = shape->buffers;
    } else {
	const char *k;

	/* Look for the current buffer and return the next. */
	
	k = lua_tostring (L, 2);

	for (b = shape->buffers;
	     b && strcmp (b->key, k);
	     b = b->next);

	b = b->next;
    }

    lua_pop (L, 1);

    /* If we're not done return the key - value pair, otherwise
     * nil. */
	
    if (b) {
	lua_pushstring (L, b->key);
	lua_pushvalue (L, 2);
	lua_gettable (L, 1);

	return 2;
    } else {
	lua_pushnil(L);

	return 1;
    }
}

static int attributes_iterator(lua_State *L)
{
    lua_pushcfunction (L, next_attribute);
    lua_pushvalue(L, 1);
    lua_pushnil (L);

    return 3;
}

@implementation Shape

+ (void)initialize
{
    if(self == [Shape class]) {
	lua_pushcfunction (_L, attributes_iterator);
	lua_setglobal (_L, "attributes");
    }
}

-(Shape *)initWithMode: (GLenum) m
{
    self = [super init];

    self->mode = m;
    self->buffers = NULL;
    self->indices = NULL;

    /* Create the vertex array object. */
    
    glGenVertexArrays (1, &self->name);

    return self;
}

-(void) free
{
    shape_Buffer *b, *next;

    /* Free the attribute buffers. */
    
    for (b = self->buffers ; b ; b = next) {
	next = b->next;

	glDeleteBuffers(1, &b->name);
	free(b->key);
	free(b);
    }

    /* Free the index buffer. */
    
    if (self->indices) {
	b = self->indices;
	
	glDeleteBuffers(1, &b->name);
	free(b->key);
	free(b);
    }

    /* And finally free the vertex array. */
    
    glDeleteVertexArrays (1, &self->name);

    [super free];
}

-(int) _get_
{
    array_Array array;
    shape_Buffer *b;
    const char *k;
    int size;

    k = lua_tostring (_L, 2);

    /* Check if the key corresponds to a buffer. */
    
    if (!strcmp (k, "indices")) {
	b = self->indices;
    } else {
	for (b = self->buffers;
	     b && strcmp (b->key, k);
	     b = b->next);
    }
    
    if (!b) {
	return [super _get_];
    }

    /* If so recreate the array. */
    
    glBindBuffer(GL_ARRAY_BUFFER, b->name);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

    array.type = b->type;
    array.free = FREE_BOTH;
    array.length = size;
    array.rank = b->size == 1 ? 1 : 2;
    array.size = malloc (array.rank * sizeof(int));

    array.size[0] = b->length;
    
    if (array.rank > 1) {
	array.size[1] = b->size;
    }

    /* Read back the data from the buffer object. */
    
    array.values.any = malloc (size);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, array.values.any);

    array_pusharray(_L, &array);
    
    return 1;
}

-(int) _set_
{
    array_Array *array;
    shape_Buffer *b;
    const char *k;
    int isindices;

    k = lua_tostring (_L, 2);
    isindices = !strcmp(k, "indices");

    /* Check whether the given key already points to a buffer. */

    if (isindices) {
	b = self->indices;
    } else {
	for (b = self->buffers;
	     b && strcmp (b->key, k);
	     b = b->next);
    }
    
    /* If it does free the buffer in question. */
    
    if (b) {
	if (isindices) {
	    self->indices = NULL;
	} else {
	    if (self->buffers == b) {
		self->buffers = b->next;
	    } else {
		shape_Buffer *s;

		for (s = self->buffers ; s->next != b ; s = s->next);
		s->next = b->next;
	    }
	}
    
	glDeleteBuffers(1, &b->name);
	free(b->key);
	free(b);	
    }

    assert (!isindices || self->indices == NULL);

    /* If index data is provided as a normal Lua table attempt to
     * promote it to and unsigned integer array.  For vertex attribute
     * buffers promote to the default (double) type. */
    
    if (isindices && lua_type(_L, 3) == LUA_TTABLE) {
	array = array_testtyped (_L, 3, ARRAY_TUINT);
    } else {
	array = array_testarray (_L, 3);
    }

    /* Now if an array has beens supplied assume it defines a new
     * buffer for the shape, otherwise treat it like a normal key. */

    if (array) {
	GLenum target;
	
	if (array->rank > 2) {
	    t_print_error("Array used for vertex attribute data is of "
			  "unsuitable rank.\n");
	    abort();
	} else if (array->rank == 2 && array->size[1] < 2) {
	    t_print_error("Scalar vertex attributes should be specified "
			  "as arrays of rank 1.\n");
	    abort();
	}	    
	
	b = malloc (sizeof (shape_Buffer));
	b->key = strdup(k);

	/* Check if supplied data is sane. */
	
	if (isindices) {
	    /* Expect only unsigned integral types for index data. */
	    
	    switch (b->type) {
	    case ARRAY_TUINT:
	    case ARRAY_TUSHORT:
	    case ARRAY_TUCHAR:
		break;
	    default:
		t_print_error("Array used for index data must be of "
			      "unsigned integral type.\n");
		abort();	    
		break;
	    }
	} else {
	    /* Check length consistency for vertex data. */
	    
	    if (self->buffers &&
		self->buffers->length != array->size[0]) {
		t_print_error ("Shape vertex buffers have inconsistent "
			       "lengths.\n");
		abort();
	    }
	}

	/* Create the buffer object and initialize it. */

	target = isindices ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;

	glGenBuffers(1, &b->name);
	glBindBuffer(target, b->name);
	glBufferData(target, array->length, array->values.any,
		     GL_STATIC_DRAW);

	b->type = array->type;
	b->length = array->size[0];
	b->size = array->rank == 1 ? 1 : array->size[1];

	/* Link in the buffer. */
	
	if (isindices) {
	    self->indices = b;
	} else {
	    b->next = self->buffers;
	    self->buffers = b;
	}
	
	return 1;
    } else {
	return [super _set_];
    }
}

-(void) meetParent: (Shader *)parent
{
    shape_Buffer *b;
    int j, l, n;

    if (![parent isKindOf: [Shader class]]) {
	t_print_warning("%s node has no shader parent.\n",
			[self name]);
	
	return;
    }

    /* Once we're linked to a shader parent we can interrogate it to
     * find out what kind of data we're supposed to have in our
     * buffers. */

    glGetProgramiv(parent->name, GL_LINK_STATUS, &j);

    if (j != GL_TRUE) {
	return;
    }
    
    glGetProgramiv(parent->name, GL_ACTIVE_ATTRIBUTES, &n);
    glGetProgramiv(parent->name, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &l);
    
    glBindVertexArray(self->name);

    for (j = 0 ; j < n ; j += 1) {
	char attribute[l];
	GLenum type;
	GLboolean normalized;	
	int i, size, integral, precision;

	glGetActiveAttrib (parent->name, i, l, NULL, &size, &type,
			   attribute);

	for (b = self->buffers;
	     b && strcmp(b->key, attribute);
	     b = b->next);
	
	if (!b) {
	    t_print_error("%s node does not speicify vertex attribute "
			  "'%s'.\n",
			  [self name], attribute);

	    abort();
	}

	
	switch (type) {
	case GL_DOUBLE_MAT2: case GL_DOUBLE_MAT3: case GL_DOUBLE_MAT4:
	case GL_DOUBLE_MAT2x3: case GL_DOUBLE_MAT2x4:
	case GL_DOUBLE_MAT3x2: case GL_DOUBLE_MAT3x4:
	case GL_DOUBLE_MAT4x2: case GL_DOUBLE_MAT4x3:
	    t_print_error("Matrix type vertex attributes not "
			  "supported.\n");
	    
	    abort();
	    break;

	case GL_DOUBLE_VEC2: case GL_DOUBLE_VEC3: case GL_DOUBLE_VEC4:
	case GL_DOUBLE:
	    if (b->type != ARRAY_TDOUBLE) {
		t_print_error("Double precision floating point data "
			      "expected for vertex attribute '%s' of "
			      "%s node.\n", attribute, [self name]);

		abort();
	    }

	    precision = 1;
	    break;

	case GL_FLOAT_MAT2: case GL_FLOAT_MAT3: case GL_FLOAT_MAT4:
	case GL_FLOAT_MAT2x3: case GL_FLOAT_MAT2x4:
	case GL_FLOAT_MAT3x2: case GL_FLOAT_MAT3x4:
	case GL_FLOAT_MAT4x2: case GL_FLOAT_MAT4x3:
	    t_print_error("Matrix type vertex attributes not "
			  "supported.\n");
	    
	    abort();
	    break;

	case GL_FLOAT_VEC2: case GL_FLOAT_VEC3: case GL_FLOAT_VEC4:
	case GL_FLOAT:
	    precision = 2;
	    break;

	case GL_INT:
	case GL_INT_VEC2: case GL_INT_VEC3: case GL_INT_VEC4:

	case GL_UNSIGNED_INT:
	case GL_UNSIGNED_INT_VEC2: case GL_UNSIGNED_INT_VEC3:
	case GL_UNSIGNED_INT_VEC4:
	    if (b->type == ARRAY_TDOUBLE ||
		b->type == ARRAY_TFLOAT) {
		t_print_error("Integral data expected for vertex attribute "
			      "'%s' of %s node.\n", attribute, [self name]);

		abort();
	    }

	    precision = 3;
	    break;
	}

	/* Try to map the supplied array type to a GL type. */
	
	switch(abs(b->type)) {
	case ARRAY_TDOUBLE:
	    type = GL_DOUBLE;
	    normalized = GL_FALSE;
	    integral = 0;
	    break;
	case ARRAY_TFLOAT:
	    type = GL_FLOAT;
	    normalized = GL_FALSE;
	    integral = 0;
	    break;
	case ARRAY_TULONG:
	case ARRAY_TLONG:
	    t_print_error("Array used for vertex attribute data is of "
			  "unsuitable type.\n");
	    abort();	    
	    break;
	case ARRAY_TUINT:
	    type = GL_UNSIGNED_INT;
	    integral = b->type > 0;
	    normalized = !integral ? GL_TRUE : GL_FALSE;
	    break;
	case ARRAY_TINT:
	    type = GL_INT;
	    integral = b->type > 0;
	    normalized = !integral ? GL_TRUE : GL_FALSE;
	    break;
	case ARRAY_TUSHORT:
	    type = GL_UNSIGNED_SHORT;
	    integral = b->type > 0;
	    normalized = !integral ? GL_TRUE : GL_FALSE;
	    break;
	case ARRAY_TSHORT:
	    type = GL_SHORT;
	    integral = b->type > 0;
	    normalized = !integral ? GL_TRUE : GL_FALSE;
	    break;
	case ARRAY_TUCHAR:
	    type = GL_UNSIGNED_BYTE;
	    integral = b->type > 0;
	    normalized = !integral ? GL_TRUE : GL_FALSE;
	    break;
	case ARRAY_TCHAR:
	    type = GL_BYTE;
	    integral = b->type > 0;
	    normalized = !integral ? GL_TRUE : GL_FALSE;
	    break;
	}

	/* Bind the data into the vertex array object's state. */
	
	glBindBuffer(GL_ARRAY_BUFFER, b->name);
	i = glGetAttribLocation(parent->name, b->key);	

	switch (precision) {
	case 1:
	    glVertexAttribIPointer(i, b->size, type, 0, (void *)0);
	    break;
	case 2:
	    glVertexAttribPointer(i, b->size, type, normalized,
				  0, (void *)0);
	    break;
	case 3:
	    glVertexAttribIPointer(i, b->size, type, 0, (void *)0);
	    break;
	}

	glEnableVertexAttribArray(i);
    }

    glBindVertexArray(0);
}

-(void) traverse
{
    /* Set the transform. */
    
    t_set_modelview (self->matrix);

    /* Bind the vertex array and draw the supplied indices or the
     * arrays if no indices we're supplied. */
    
    glBindVertexArray(self->name);

    if (self->indices) {
	shape_Buffer *i;
	GLenum type;

	i = self->indices;
	
	switch(i->type) {
	case ARRAY_TUINT: type = GL_UNSIGNED_INT; break;
	case ARRAY_TUSHORT: type = GL_UNSIGNED_SHORT; break;
	case ARRAY_TUCHAR: type = GL_UNSIGNED_BYTE; break;
	default: assert(0);
	}
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i->name);
	glDrawRangeElements (self->mode, 0, self->buffers->length - 1,
			     i->size * i->length,
			     type, (void *)0);
    } else {
	glDrawArrays (self->mode, 0, self->buffers->length);
    }
    
    [super traverse];
}

@end
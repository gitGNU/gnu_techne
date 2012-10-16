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
#include <string.h>
#include <search.h>
#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "array/array.h"
#include "techne.h"
#include "shader.h"

#include "glsl/preamble.h"

static char *declarations[3];
static struct {
    const char *name;
    unsigned int index;
} *globals;

static int globals_n;

#define TYPE_ERROR()							\
    {									\
	t_print_error("Value assigned to uniform variable '%s' of "	\
		      "shader '%s' is of unsuitable type.\n",		\
		      k, [self name]);					\
	abort();							\
    }

#define UPDATE_SINGLE(value, length)					\
    {									\
	glBufferSubData (GL_UNIFORM_BUFFER, u->offset, length, value);	\
    }

#define UPDATE_ARRAY(size_0, stride_0, value, length)			\
    {									\
	void *b;							\
	int i;								\
									\
	b = glMapBuffer (GL_UNIFORM_BUFFER, GL_WRITE_ONLY);		\
									\
	for(i = 0 ; i < size_0 ; i += 1) {				\
	    memcpy(b + u->offset + i * stride_0,			\
		   (void *)value + i * length,				\
		   length);						\
	}								\
									\
	glUnmapBuffer (GL_UNIFORM_BUFFER);				\
    }

#define UPDATE_MATRIX_ARRAY(size_0, stride_0, size_1, stride_1,		\
			    value, length)				\
    {									\
	void *b;							\
	int i, j;							\
									\
	b = glMapBuffer (GL_UNIFORM_BUFFER, GL_WRITE_ONLY);		\
									\
	for(j = 0 ; j < size_1 ; j += 1) {				\
	    for(i = 0 ; i < size_0 ; i += 1) {				\
		memcpy(b + u->offset + i * stride_0 + j * stride_1,	\
		       (void *)value + (j * size_0 + i) * length,	\
		       length);						\
	    }								\
	}								\
									\
	glUnmapBuffer (GL_UNIFORM_BUFFER);				\
    }

#define UPDATE_VALUE(size_0, stride_0, size_1, stride_1, elements,	\
		     ctype, arraytype)					\
    {									\
	array_Array *array;						\
	ctype n;							\
									\
	if (size_0 == 1 && size_1 == 1) {				\
	    if (elements == 1) {					\
		if (lua_type (_L, 3) != LUA_TNUMBER ) {			\
		    TYPE_ERROR();					\
		}							\
									\
		n = (ctype)lua_tonumber (_L, 3);			\
		UPDATE_SINGLE (&n, elements * sizeof(ctype));		\
	    } else {							\
		array = array_testcompatible (_L, 3,                    \
                                              ARRAY_TYPE | ARRAY_RANK | \
                                              ARRAY_SIZE, arraytype,    \
                                              1, elements);		\
		if(!array) {						\
		    TYPE_ERROR();					\
		}							\
									\
		UPDATE_SINGLE (array->values.any,			\
			       elements * sizeof(ctype));		\
	    }								\
	} else if (size_1 == 1) {					\
	    if (elements == 1) {					\
		array = array_testcompatible (_L, 3,                    \
                                              ARRAY_TYPE | ARRAY_RANK | \
                                              ARRAY_SIZE, arraytype, 1, \
					      size_0);			\
	    } else {							\
		array = array_testcompatible (_L, 3,                    \
                                              ARRAY_TYPE | ARRAY_RANK | \
                                              ARRAY_SIZE, arraytype, 2, \
					      size_0, elements);	\
	    }								\
									\
	    if(!array) {						\
		TYPE_ERROR();						\
	    }								\
									\
	    UPDATE_ARRAY (size_0, stride_0, array->values.any,		\
			  elements * sizeof(ctype));			\
	} else {							\
	    array = array_testcompatible (_L, 3,                        \
                                          ARRAY_TYPE | ARRAY_RANK |     \
                                          ARRAY_SIZE, arraytype, 3,     \
					  size_1, size_0, elements);	\
									\
	    if(!array) {						\
		TYPE_ERROR();						\
	    }								\
									\
	    UPDATE_MATRIX_ARRAY (size_0, stride_0, size_1, stride_1, 	\
				 array->values.any,			\
				 elements * sizeof(ctype));		\
	}								\
    }	    

#define CASE_BLOCK_S_V(ctype, gltype, arraytype)	\
    case gltype:					\
        UPDATE_VALUE(u->size, u->arraystride, 1, 0, 1,	\
		       ctype, arraytype); break;	\
    case gltype##_VEC2:					\
	UPDATE_VALUE(u->size, u->arraystride, 1, 0, 2,	\
		       ctype, arraytype); break;	\
    case gltype##_VEC3:					\
	UPDATE_VALUE(u->size, u->arraystride, 1, 0, 3,	\
		       ctype, arraytype); break;	\
    case gltype##_VEC4:					\
	UPDATE_VALUE(u->size, u->arraystride, 1, 0, 4,	\
		       ctype, arraytype); break;	\

#define CASE_BLOCK_M(ctype, gltype, arraytype)				\
    case gltype##_MAT2:							\
    UPDATE_VALUE(2, u->matrixstride, u->size, u->arraystride, 2,	\
		 ctype, arraytype); break;				\
    case gltype##_MAT3:							\
    UPDATE_VALUE(3, u->matrixstride, u->size, u->arraystride, 3,	\
		 ctype, arraytype); break;				\
    case gltype##_MAT4:							\
    UPDATE_VALUE(4, u->matrixstride, u->size, u->arraystride, 4,	\
		 ctype, arraytype); break;				\
    									\
    case gltype##_MAT2x3:						\
    UPDATE_VALUE(2, u->matrixstride, u->size, u->arraystride, 3,	\
		 ctype, arraytype); break;				\
    case gltype##_MAT2x4:						\
    UPDATE_VALUE(2, u->matrixstride, u->size, u->arraystride, 4,	\
		 ctype, arraytype); break;				\
    case gltype##_MAT3x2:						\
    UPDATE_VALUE(3, u->matrixstride, u->size, u->arraystride, 2,	\
		 ctype, arraytype); break;				\
    case gltype##_MAT3x4:						\
    UPDATE_VALUE(3, u->matrixstride, u->size, u->arraystride, 4,	\
		 ctype, arraytype); break;				\
									\
    case gltype##_MAT4x2:						\
    UPDATE_VALUE(4, u->matrixstride, u->size, u->arraystride, 2,	\
		 ctype, arraytype); break;				\
    case gltype##_MAT4x3:						\
    UPDATE_VALUE(4, u->matrixstride, u->size, u->arraystride, 3,	\
		 ctype, arraytype); break;				\

#define UPDATE_UNIFORM(uniform)						\
    {									\
	shader_Uniform *u;						\
									\
	u = uniform;							\
	glBindBuffer(GL_UNIFORM_BUFFER, self->blocks[u->block]);	\
									\
	switch (u->type) {						\
	    CASE_BLOCK_S_V(double, GL_DOUBLE, ARRAY_TDOUBLE);		\
	    CASE_BLOCK_M(double, GL_DOUBLE, ARRAY_TDOUBLE);		\
									\
	    CASE_BLOCK_S_V(float, GL_FLOAT, ARRAY_TFLOAT);		\
	    CASE_BLOCK_M(float, GL_FLOAT, ARRAY_TFLOAT);		\
									\
	    CASE_BLOCK_S_V(int, GL_INT, ARRAY_TINT);			\
	    CASE_BLOCK_S_V(unsigned int, GL_UNSIGNED_INT, ARRAY_TUINT);	\
	    CASE_BLOCK_S_V(unsigned char, GL_BOOL, ARRAY_TUCHAR);	\
	default: abort();						\
	}								\
    }

static int next_uniform(lua_State *L)
{
    Shader *shader;
    unsigned int i;

    shader = t_checknode (L, 1, [Shader class]);

    if (lua_isnil (L, 2)) {
	/* We need the first uniform so return 0. */
	
	if (shader->blocks_n > 0) {
	    i = 0;
	} else {
	    i = GL_INVALID_INDEX;
	}
    } else {
	const char *k;

	/* Look up the index of the current uniform and return the
	 * next. */
	
	k = lua_tostring (L, 2);
	
	glGetUniformIndices(shader->name, 1, &k, &i);

	if (i != shader->blocks_n - 1) {
	    i += 1;
	} else {
	    i = GL_INVALID_INDEX;
	}
    }

    lua_pop (L, 1);

    if (i != GL_INVALID_INDEX) {
	GLenum type;
	int l, size;

	/* Get the uniforms name and return the key - value pair. */
	
	glGetActiveUniformsiv(shader->name, 1, &i,
			      GL_UNIFORM_NAME_LENGTH, &l);

	{
	    char name[l];
	    
	    glGetActiveUniform(shader->name, i, l, NULL,
			       &size, &type, name);

	    lua_pushstring (L, name);
	}

	lua_pushvalue (L, 2);
	lua_gettable (L, 1);

	return 2;
    } else {
	lua_pushnil(L);

	return 1;
    }
}

static int uniforms_iterator(lua_State *L)
{
    lua_pushcfunction (L, next_uniform);
    lua_pushvalue(L, 1);
    lua_pushnil (L);

    return 3;
}

@implementation Shader

+(void)initialize
{
    if(self == [Shader class]) {
	/* Export the uniforms iterator. */
	
	lua_pushcfunction (_L, uniforms_iterator);
	lua_setglobal (_L, "uniforms");	
    }
}

+(int) addUniformBlockNamed: (const char *)name
                   forStage: (shader_Stage)stage
                 withSource: (const char *)declaration
{
    unsigned int i;
    int l;

    /* Allocate a buffer object and add it to the globals list. */

    glGenBuffers(1, &i);
    
    globals_n += 1;
    globals = realloc (globals, globals_n * sizeof (globals[0]));

    globals[globals_n - 1].name = name;
    globals[globals_n - 1].index = i;

    /* Add the declaration for the global block to the declaractions
       string for the stage. */
    
    if (declarations[stage]) {
	l = strlen(declarations[stage]) + strlen(declaration) + 1;
	declarations[stage] = realloc(declarations[stage], l * sizeof (char));
	strcat(declarations[stage], declaration);
    } else {
	l = strlen(declaration) + 1;
	declarations[stage] = malloc(l * sizeof (char));
	strcpy(declarations[stage], declaration);
    }

    return i;
}

-(id) init
{
    self = [super init];
    self->name = glCreateProgram();
    self->ismold = 1;
    
    return self;
}

-(void) addSource: (const char *) source for: (shader_Stage)stage
{
    unsigned int shader;

    switch(stage) {
    case VERTEX_STAGE:
	shader = glCreateShader(GL_VERTEX_SHADER);break;
    case GEOMETRY_STAGE:
	shader = glCreateShader(GL_GEOMETRY_SHADER);break;
    case FRAGMENT_STAGE:
	shader = glCreateShader(GL_FRAGMENT_SHADER);break;
    }
    
    if (declarations[stage]) {
	const char *sources[3] = {glsl_preamble,
				  declarations[stage],
				  source};

	glShaderSource(shader, 3, sources, NULL);
    } else {
	const char *sources[2] = {glsl_preamble, source};

	glShaderSource(shader, 2, sources, NULL);
    }
    glCompileShader(shader);
    glAttachShader(self->name, shader);
    glDeleteShader(shader);
}

-(void) link
{
    int i, j, k, n;
    int u;
    
    glLinkProgram(self->name);

    /* Validate the shader and print the info logs if necessary. */
    
    glGetProgramiv(self->name, GL_LINK_STATUS, &i);
    glGetProgramiv(self->name, GL_ATTACHED_SHADERS, &n);
    glGetProgramiv(self->name, GL_ACTIVE_ATTRIBUTES, &j);
    glGetProgramiv(self->name, GL_ACTIVE_UNIFORMS, &k);

    if (i == GL_TRUE) {
	t_print_message("Program for %s nodes linked successfully (%d shaders, %d uniforms and %d attributes).\n",
			[self name],
			n, k, j);
    } else {
	t_print_warning("Program for %s nodes did not link successfully.\n",
			[self name]);

	glGetProgramiv(self->name, GL_INFO_LOG_LENGTH, &j);

	if (j > 1) {
	    char buffer[j];

	    glGetProgramInfoLog (self->name, j, NULL, buffer);
	    t_print_warning("Linker output follows:\n%s\n", buffer);
	}

	if (n > 0) {
	    unsigned int shaders[n];

	    glGetAttachedShaders (self->name, n, NULL, shaders);

	    for (i = 0 ; i < n ; i += 1) {
		char *type;
	    
		glGetShaderiv(shaders[i], GL_SHADER_TYPE, &j);

		switch (j) {
		case GL_VERTEX_SHADER: type = "Vertex"; break;
		case GL_FRAGMENT_SHADER: type = "Fragment"; break;
		case GL_GEOMETRY_SHADER: type = "Geometry"; break;
		default:assert(0);
		}
	    
		glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &j);

		if (j == GL_TRUE) {
		    continue;
		}
	    
		t_print_warning("%s shader %d did not compile "
				"successfully.\n",
				type, i + 1);

		glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &j);

		if (j > 1) {
		    char buffer[j];

		    glGetShaderInfoLog (shaders[i], j, NULL, buffer);
		    t_print_warning("Compiler output follows:\n%s\n",
				    buffer);
		}
	    }
	}
    }
    
    /* Create a map of active uniforms. */
    
    glGetProgramiv (self->name, GL_ACTIVE_UNIFORMS, &u);
    
    if (u > 0) {
	unsigned int list[u];
	int types[u], sizes[u], offsets[u];
	int arraystrides[u], matrixstrides[u], indices[u];
	int i;

	self->uniforms = malloc (u * sizeof (shader_Uniform));

	for (i = 0 ; i < u ; i += 1) {
	    list[i] = i;
	}	

	/* Read interesting parameters for all uniforms. */
	
	glGetActiveUniformsiv(self->name, u, list,
			      GL_UNIFORM_BLOCK_INDEX, indices);
	glGetActiveUniformsiv(self->name, u, list,
			      GL_UNIFORM_TYPE, types);
	glGetActiveUniformsiv(self->name, u, list,
			      GL_UNIFORM_SIZE, sizes);
	glGetActiveUniformsiv(self->name, u, list,
			      GL_UNIFORM_OFFSET, offsets);
	glGetActiveUniformsiv(self->name, u, list,
			      GL_UNIFORM_ARRAY_STRIDE, arraystrides);
	glGetActiveUniformsiv(self->name, u, list,
			      GL_UNIFORM_MATRIX_STRIDE, matrixstrides);

	for (i = 0 ; i < u ; i += 1) {
	    if (indices[i] == GL_INVALID_INDEX) {
		t_print_error("Encountered uniform in the default block "
			      "of shader '%s'.\n", [self name]);
		abort();
	    }
	    
	    self->uniforms[i].block = indices[i];
	    self->uniforms[i].type = types[i];
	    self->uniforms[i].size = sizes[i];
	    self->uniforms[i].offset = offsets[i];
	    self->uniforms[i].arraystride = arraystrides[i];
	    self->uniforms[i].matrixstride = matrixstrides[i];

	    /* if (self->blocks[self->uniforms[i].block] == context) { */
	    /* 	_TRACE ("offset: %d\n", self->uniforms[i].offset); */
	    /* } */
	}	
    } else {
	self->uniforms = NULL;
    }
}

-(id)initFrom: (Shader *) mold
{
    int i;

    assert (mold->ismold);

    self = [super init];
    self->name = mold->name;
    self->uniforms = mold->uniforms;
    self->ismold = 0;

    /* Allocate uniform buffer objects. */
    
    glGetProgramiv (self->name, GL_ACTIVE_UNIFORM_BLOCKS, &self->blocks_n);

    if (self->blocks_n > 0) {
	self->blocks = malloc (self->blocks_n * sizeof (unsigned int));

	for (i = 0 ; i < self->blocks_n ; i += 1) {
	    int s, l;

	    glUniformBlockBinding (self->name, i, i);
	    glGetActiveUniformBlockiv(self->name, i,
				      GL_UNIFORM_BLOCK_DATA_SIZE,
				      &s);
	    glGetActiveUniformBlockiv(self->name, i,
				      GL_UNIFORM_BLOCK_NAME_LENGTH,
				      &l);

	    /* Create a new uniform buffer object and allocate enough
	     * space for it. */
	    
	    {
		char blockname[l];

		glGetActiveUniformBlockName(self->name, i, l, NULL,
					    blockname);

		/* If the block name begins with two underscores it's a
		 * global block so it'll have already been created and
		 * bound. */

		if (!strncmp(blockname, "__", 2)) {
                    int j;

                    for (j = 0;
                         j < globals_n && strcmp (globals[j].name, blockname);
                         j += 1);

                    assert (j != globals_n);
		    self->blocks[i] = globals[j].index;
		} else {
		    glGenBuffers(1, &self->blocks[i]);
		    glBindBuffer(GL_UNIFORM_BUFFER, self->blocks[i]);
		    glBufferData(GL_UNIFORM_BUFFER, s, NULL,
				 GL_DYNAMIC_DRAW);
		}
	    }
	}
    } else {
	self->blocks = NULL;
    }
    
    return self;
}

-(id) free
{
    int i, j;
    
    /* Free the uniform buffer objects. */

    if (self->blocks_n > 0) {
        /* Remove any global blocks from the list.  We don't want to
           free those. */
        
        for (i = 0 ; i < self->blocks_n ; i += 1) {
            for (j = 0 ; j < globals_n ; j += 1) {
                if (self->blocks[i] == globals[j].index) {
                    self->blocks[i] = GL_INVALID_INDEX;
                }
            }
        }
        
	glDeleteBuffers (self->blocks_n, self->blocks);
	free(self->blocks);
    }

    if (self->ismold) {
	/* Free the uniform information and delete the program if this is
	 * a mold. */

	free(self->uniforms);
	glDeleteProgram (self->name);
	
	_TRACE ("Deleting %s program and associated shaders.\n", [self name]);
    }

    [super free];
    
    return self;
}

-(int) _get_
{
    const char *k;
    unsigned int i;

    /* Skip this if the shader has no uniforms. */

    if (self->blocks_n == 0) {
        return [super _get_];
    }

    k = lua_tostring (_L, 2);

    /* Check if the key refers to a uniform and return the stored
     * reference. */
    
    glGetUniformIndices(self->name, 1, &k, &i);
    
    if (i != GL_INVALID_INDEX) {
	lua_getuservalue (_L, 1);
	lua_replace (_L, 1);
	lua_gettable (_L, 1);

	return 1;
    } else {
	return [super _get_];
    }
}

-(int) _set_
{
    const char *k;
    unsigned int i;

    /* Skip this if the shader has no uniforms. */

    if (self->blocks_n == 0) {
        return [super _set_];
    }

    k = lua_tostring (_L, 2);

    /* Check if the key refers to a uniform and if so update the
     * uniform buffer object it's stored in.  Also keep a reference to
     * allow for a shortcut on the way back. */
    
    glGetUniformIndices(self->name, 1, &k, &i);

    if (i != GL_INVALID_INDEX) {
	UPDATE_UNIFORM (&self->uniforms[i]);

	lua_getuservalue (_L, 1);
	lua_replace (_L, 1);
	lua_settable (_L, 1);
	
	return 1;
    } else {
	return [super _set_];
    }
}

-(void) traverse
{
    int i;

    /* _TRACE ("%f, %f, %f, %f,\n" */
    /* 	    "%f, %f, %f, %f,\n" */
    /* 	    "%f, %f, %f, %f,\n" */
    /* 	    "%f, %f, %f, %f\n", */
    /* 	    self->matrix[0], self->matrix[1], self->matrix[2], self->matrix[3], self->matrix[4], self->matrix[5], self->matrix[6], self->matrix[7], self->matrix[8], self->matrix[9], self->matrix[10], self->matrix[11], self->matrix[12], self->matrix[13], self->matrix[14], self->matrix[15]); */

    /* Bind the program and all uniform buffers and proceed to draw
     * the meshes. */
    
    glUseProgram(self->name);
    
    for (i = 0 ; i < self->blocks_n ; i += 1) {
	if (self->blocks[i] != GL_INVALID_INDEX) {
	    glBindBufferBase(GL_UNIFORM_BUFFER, i, self->blocks[i]);
	}
    }
    
    [super traverse];
}

@end

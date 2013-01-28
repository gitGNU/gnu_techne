/* Copyright (C) 2012 Papavasileiou Dimitris
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
    
#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>

#include "array.h"

#if LUA_VERSION_NUM == 501
#define lua_rawlen lua_objlen
#endif

#define absolute(_L, _i) (_i < 0 ? lua_gettop (_L) + _i + 1 : _i)

static int metatable = LUA_REFNIL;
static const void *signature;

static int __index (lua_State *L);
static int __newindex (lua_State *L);
static int __gc (lua_State *L);
static int __len (lua_State *L);
static int __ipairs (lua_State *L);
static int __tostring (lua_State *L);

static array_Array *testarray (lua_State *L, int index);

static size_t sizeof_element (array_Type type)
{
    switch (abs(type)) {
    case ARRAY_TDOUBLE:
	return sizeof (double);
    case ARRAY_TFLOAT:
	return sizeof (float);
    case ARRAY_TULONG:
	return sizeof (unsigned long);
    case ARRAY_TLONG:
	return sizeof (long);
    case ARRAY_TUINT:
	return sizeof (unsigned int);
    case ARRAY_TINT:
	return sizeof (int);
    case ARRAY_TUSHORT:
	return sizeof (unsigned short);
    case ARRAY_TSHORT:
	return sizeof (short);
    case ARRAY_TUCHAR:
	return sizeof (unsigned char);
    case ARRAY_TCHAR:
	return sizeof (char);
    }

    return 0;
}

static void write_element (array_Array *array, int i, lua_Number value)
{
    switch (array->type) {
    case ARRAY_TDOUBLE:
	array->values.doubles[i] = value;
	break;
    case ARRAY_TFLOAT:
	array->values.floats[i] = value;
	break;
    case ARRAY_TULONG:
	array->values.ulongs[i] = value;
	break;
    case ARRAY_TNULONG:
	array->values.ulongs[i] = value * ULONG_MAX;
	break;
    case ARRAY_TLONG:
	array->values.longs[i] = value;
	break;
    case ARRAY_TNLONG:
	array->values.longs[i] = value * LONG_MAX;
	break;
    case ARRAY_TUINT:
	array->values.uints[i] = value;
	break;
    case ARRAY_TNUINT:
	array->values.uints[i] = value * UINT_MAX;
	break;
    case ARRAY_TINT:
	array->values.ints[i] = value;
	break;
    case ARRAY_TNINT:
	array->values.ints[i] = value * INT_MAX;
	break;
    case ARRAY_TUSHORT:
	array->values.ushorts[i] = value;
	break;
    case ARRAY_TNUSHORT:
	array->values.ushorts[i] = value * USHRT_MAX;
	break;
    case ARRAY_TSHORT:
	array->values.shorts[i] = value;
	break;
    case ARRAY_TNSHORT:
	array->values.shorts[i] = value * SHRT_MAX;
	break;
    case ARRAY_TUCHAR:
	array->values.uchars[i] = value;
	break;
    case ARRAY_TNUCHAR:
	array->values.uchars[i] = value * UCHAR_MAX;
	break;
    case ARRAY_TCHAR:
	array->values.chars[i] = value;
	break;
    case ARRAY_TNCHAR:
	array->values.chars[i] = value * CHAR_MAX;
	break;
    }
}

static lua_Number read_element (array_Array *array, int i)
{
    switch (array->type) {
    case ARRAY_TDOUBLE:
	return (lua_Number)array->values.doubles[i];
    case ARRAY_TFLOAT:
	return (lua_Number)array->values.floats[i];
    case ARRAY_TULONG:
	return (lua_Number)array->values.ulongs[i];
    case ARRAY_TNULONG:
	return (lua_Number)array->values.ulongs[i] / ULONG_MAX;
    case ARRAY_TLONG:
	return (lua_Number)array->values.longs[i];
    case ARRAY_TNLONG:
	return (lua_Number)array->values.longs[i] / LONG_MAX;
    case ARRAY_TUINT:
	return (lua_Number)array->values.uints[i];
    case ARRAY_TNUINT:
	return (lua_Number)array->values.uints[i] / UINT_MAX;
    case ARRAY_TINT:
	return (lua_Number)array->values.ints[i];
    case ARRAY_TNINT:
	return (lua_Number)array->values.ints[i] / INT_MAX;
    case ARRAY_TUSHORT:
	return (lua_Number)array->values.ushorts[i];
    case ARRAY_TNUSHORT:
	return (lua_Number)array->values.ushorts[i] / USHRT_MAX;
    case ARRAY_TSHORT:
	return (lua_Number)array->values.shorts[i];
    case ARRAY_TNSHORT:
	return (lua_Number)array->values.shorts[i] / SHRT_MAX;
    case ARRAY_TUCHAR:
	return (lua_Number)array->values.uchars[i];
    case ARRAY_TNUCHAR:
	return (lua_Number)array->values.uchars[i] / UCHAR_MAX;
    case ARRAY_TCHAR:
	return (lua_Number)array->values.chars[i];
    case ARRAY_TNCHAR:
	return (lua_Number)array->values.chars[i] / CHAR_MAX;
    }

    return 0;
}

static void *reference_element (array_Array *array, int i)
{
    switch (abs(array->type)) {
    case ARRAY_TDOUBLE:
	return &array->values.doubles[i];
    case ARRAY_TFLOAT:
	return &array->values.floats[i];
    case ARRAY_TULONG:
	return &array->values.ulongs[i];
    case ARRAY_TLONG:
	return &array->values.longs[i];
    case ARRAY_TUINT:
	return &array->values.uints[i];
    case ARRAY_TINT:
	return &array->values.ints[i];
    case ARRAY_TUSHORT:
	return &array->values.ushorts[i];
    case ARRAY_TSHORT:
	return &array->values.shorts[i];
    case ARRAY_TUCHAR:
	return &array->values.uchars[i];
    case ARRAY_TCHAR:
	return &array->values.chars[i];
    }

    return NULL;
}

static void copy_values (array_Array *array, void *values, int i, int j, int n)
{
    int w;

    w = sizeof_element(array->type);
    memcpy(((char *)array->values.any) + i * w, ((char *)values) + j * w, n * w);
}

static void copy_elements (array_Array *to, array_Array *from, int i, int j, int n)
{
    assert(from->type == to->type);
    copy_values (to, from->values.any, i, j, n);
}

static void zero_elements (array_Array *array, int i, int n)
{
    int w;

    w = sizeof_element(array->type);
    memset(((char *)array->values.any) + i * w, 0, n * w);
}

static void dump (lua_State *L, int index, array_Array *array,
		  int l, int b)
{
    array_Array *subarray;
    int i, d;

    index = absolute (L, index);
    subarray = testarray(L, index);

    if (subarray) {
	if (subarray->type != array->type) {
	    lua_pushfstring (L,
			     "Inconsistent array structure (subarray "
			     "at depth %d has unsuitable type).", l);
	    lua_error (L);
	}

	if (subarray->rank != array->rank - l) {
	    lua_pushfstring (L,
			     "Inconsistent array structure (subarray "
			     "at depth %d should have rank %d but "
			     "has %d).",
			     l, array->rank - l, subarray->rank);
	    lua_error (L);
	}

	for (i = 0, d = 1 ; i < array->rank - l ; d *= subarray->size[i], i += 1) {
	    if (subarray->size[i] != array->size[l + i]) {
		lua_pushfstring (L,
				 "Inconsistent array structure (subarray "
				 "at depth %d should have size %d along "
				 "dimension %d but has %d).",
				 l, array->size[l + i], i + 1, subarray->size[i]);
		lua_error (L);
	    }
	}

	copy_elements(array, subarray, b, 0, d);
    } else {
	if (lua_rawlen (L, index) != array->size[l]) {
	    lua_pushfstring (L,
			     "Inconsistent array structure (subarray "
			     "at depth %d should have %d elements but "
			     "has %d).",
			     l, array->size[l], lua_rawlen (L, -1));
	    lua_error (L);
	}	
	
	for (i = l + 1, d = 1;
	     i < array->rank;
	     d *= array->size[i], i += 1);

	for (i = 0 ; i < array->size[l] ; i += 1) {
	    lua_rawgeti (L, index, i + 1);
	
	    if (lua_type (L, -1) == LUA_TNUMBER) {
		write_element(array, b + i, lua_tonumber (L, -1));
	    } else {
		dump (L, -1, array, l + 1, b + i * d);
	    }
	
	    lua_pop (L, 1);
	}
    }
}

static array_Array *construct (lua_State *L, array_Array *array)
{
    array_Array *new;

    new = (array_Array *)lua_newuserdata(L, sizeof(array_Array));
    *new = *array;

    if (metatable == LUA_REFNIL) {
	lua_newtable (L);
    
	lua_pushstring(L, "__ipairs");
	lua_pushcfunction(L, (lua_CFunction)__ipairs);
	lua_settable(L, -3);
	lua_pushstring(L, "__pairs");
	lua_pushcfunction(L, (lua_CFunction)__ipairs);
	lua_settable(L, -3);
	lua_pushstring(L, "__len");
	lua_pushcfunction(L, (lua_CFunction)__len);
	lua_settable(L, -3);
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, (lua_CFunction)__index);
	lua_settable(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, (lua_CFunction)__newindex);
	lua_settable(L, -3);
	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, (lua_CFunction)__tostring);
	lua_settable(L, -3);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, (lua_CFunction)__gc);
	lua_settable(L, -3);
	lua_pushstring(L, "__array");
	lua_pushboolean(L, 1);
	lua_settable(L, -3);

	signature = lua_topointer (L, -1);
	metatable = luaL_ref (L, LUA_REGISTRYINDEX);
    }

    lua_rawgeti (L, LUA_REGISTRYINDEX, metatable);
    lua_setmetatable(L, -2);

    return new;
}

static int __gc (lua_State *L)
{
    array_Array *array;
    
    array = lua_touserdata(L, 1);

    if (array->free == FREE_SIZE || array->free == FREE_BOTH) {
	free(array->size);
    }

    if (array->free == FREE_VALUES || array->free == FREE_BOTH) {
	free(array->values.any);
    }

    return 0;
}

static int __tostring (lua_State *L)
{
    array_Array *array;
    int i;
    
    array = lua_touserdata(L, 1);

    lua_pushfstring (L, "<array[%d", array->size[0]);

    for (i = 1 ; i < array->rank ; i += 1) {
	lua_pushfstring (L, ", %d", array->size[i]);
    }

    lua_pushstring (L, "]>");

    lua_concat (L, i + 1);
    
    return 1;
}

static int __len (lua_State *L)
{
    array_Array *array;
    
    array = lua_touserdata(L, 1);

    lua_pushinteger (L, array->size[0]);
    
    return 1;
}

static int __next (lua_State *L)
{
    array_Array *array;
    int k;
    
    array = lua_touserdata(L, 1);
    k = lua_tointeger (L, 2);

    lua_pop (L, 1);
    
    if (k < array->size[0]) {
	lua_pushinteger (L, k + 1);
	lua_pushinteger (L, k + 1);
	lua_gettable (L, 1);

	return 2;
    } else {
	lua_pushnil (L);

	return 1;
    }
}

static int __ipairs (lua_State *L)
{
    lua_pushcfunction (L, __next);
    lua_insert (L, 1);
    lua_pushnil (L);

    return 3;
}

static int __index (lua_State *L)
{
    array_Array *array;
    int i;
    
    array = lua_touserdata(L, 1);
    i = lua_tointeger (L, 2);

    if (i > 0 && i <= array->size[0]) {
	if (array->rank == 1) {
	    lua_pushnumber (L, read_element(array,
					    lua_tointeger (L, 2) - 1));
	} else {
	    array_Array subarray;
	    int j, d;

	    for (j = 1, d = 1;
		 j < array->rank;
		 d *= array->size[j], j += 1);

	    subarray.type = array->type;
	    subarray.free = FREE_NOTHING;
	    subarray.rank = array->rank - 1;
	    subarray.size = &array->size[1];
	    subarray.values.any = reference_element(array, (i - 1) * d);

	    construct (L, &subarray);

            /* Make a reference to the superarray since we're pointing
             * to its data. */
            
            lua_createtable(L, 1, 0);
            lua_pushvalue(L, 1);
            lua_rawseti(L, -2, 1);
            lua_setuservalue(L, -2);
	}
    } else {
	lua_pushnil (L);
    }

    return 1;
}

static int __newindex (lua_State *L)
{
    array_Array *array;
    int i;
    
    array = lua_touserdata(L, 1);
    i = lua_tointeger (L, 2);

    if (i > 0 && i <= array->size[0]) {
	if (array->rank == 1) {
	    write_element (array,
			   lua_tointeger (L, 2) - 1,
			   lua_tonumber (L, 3));
	} else if (lua_istable (L, 3)) {
	    array_Array subarray;
	    int j, r, d;
	    
	    for (r = 0;
		 lua_type (L, -1) == LUA_TTABLE;
		 r += 1, lua_rawgeti (L, -1, 1)) {
		if (array->size[r + 1] != lua_rawlen (L, -1)) {
		    lua_pushstring (L, "Array sizes don't match.");
		    lua_error (L);
		}
	    }

	    if (r != array->rank - 1) {
		lua_pushstring (L, "Array dimensions don't match.");
		lua_error (L);		
	    }

	    lua_settop (L, 3);

	    for (j = 1, d = 1;
		 j < array->rank;
		 d *= array->size[j], j += 1);

	    subarray.type = array->type;
	    subarray.free = FREE_NOTHING;
	    subarray.rank = array->rank - 1;
	    subarray.size = &array->size[1];
	    subarray.values.any = reference_element(array, (i - 1) * d);

	    dump (L, -1, &subarray, 0, 0);
	} else {
	    array_Array *subarray;
	    int j, d;

	    subarray = lua_touserdata(L, 3);

	    for (j = 1, d = 1;
		 j < array->rank;
		 d *= array->size[j], j += 1);

	    if (subarray->rank != array->rank - 1) {
		lua_pushstring (L, "Array dimensions don't match.");
		lua_error (L);
	    }

	    if (subarray->type != array->type) {
		lua_pushstring (L, "Array types don't match.");
		lua_error (L);
	    }

	    for (j = 1 ; j < array->rank ; j += 1) {
		if (subarray->size[j - 1] != array->size[j]) {
		    lua_pushstring (L, "Array sizes don't match.");
		    lua_error (L);
		}
	    }
	    
	    copy_elements(array, subarray, (i - 1) * d, 0, d);
	}
    } else {
	lua_pushstring (L, "Index out of array bounds.");
	lua_error (L);
    }

    return 0;
}

static int fromtable (lua_State *L, int index, array_Array *array)
{
    array_Array *subarray;
    int i, l, r, h;

    luaL_checktype (L, index, LUA_TTABLE);

    h = lua_gettop (L);

    lua_pushvalue (L, index);
    
    for (r = 0, subarray = NULL;
	 lua_type (L, -1) == LUA_TTABLE || (subarray = testarray(L, -1));
	 lua_rawgeti (L, -1, 1)) {
	if (subarray) {
	    /* If we've hit a subarray we can happily skip parsing the
	     * rest of the table structure. */
	    
	    r += subarray->rank;
	    break;
	} else {
	    r += 1;
	}
    }

    lua_settop (L, h);

    if (array->rank == 0) {
	array->rank = r;
	array->size = calloc (r, sizeof(int));
    } else if (r != array->rank) {
	lua_pushstring (L, "Initialization from table of incompatible rank.");
	lua_error (L);
    }
    
    lua_pushvalue (L, index);
	
    for (i = 0, l = 1 ; i < r ; i += 1) {
	int j, l_0;

	subarray = testarray(L, -1);

	if (subarray) {
	    /* If we've hit a subarray we can happily skip parsing the
	     * rest of the table structure. */
	    
	    l *= subarray->length / sizeof_element (subarray->type);

	    for (j = i ; j < r ; j += 1) {
		if (array->size[j] == 0) {
		    array->size[j] = subarray->size[j - i];
		} else if (subarray->size[j - i] != array->size[j]) {
		    lua_pushstring (L, "Initialization from table of incompatible size.");
		    lua_error (L);
		}
	    }
	    
	    break;
	} else {
	    l_0 = lua_rawlen (L, -1);
	    l *= l_0;

	    if (array->size[i] == 0) {
		array->size[i] = l_0;
	    } else if (l_0 != array->size[i]) {
		lua_pushstring (L, "Initialization from table of incompatible size.");
		lua_error (L);
	    }
	
	    lua_rawgeti (L, -1, 1);
	}
    }
    
    lua_settop (L, h);

    array->free = FREE_BOTH;
    array->length = l * sizeof_element (array->type);
    array->values.any = malloc (array->length);

    dump (L, index, array, 0, 0);
    construct (L, array);

    return 1;
}

static void fromstring (lua_State *L, int index, array_Array *array)
{
    int i, l;

    for (i = 0, l = 1 ; i < array->rank ; l *= array->size[i], i += 1);

    l *= sizeof_element (array->type);
    array->free = FREE_SIZE;
    array->length = lua_rawlen (L, index);
    array->values.any = (void *)lua_tostring (L, index);

    if (l != array->length) {
	lua_pushfstring (L, "Invalid array data length (should be %d bytes but is %d bytes).", l, lua_rawlen (L, index));
	lua_error (L);
    }

    construct (L, array);
}

static void fromuserdata (lua_State *L, int index, array_Array *array)
{
    array->free = FREE_SIZE;
    array->length = lua_rawlen(L, index);
    array->values.any = (void *)lua_touserdata (L, index);

    construct (L, array);
}

static void fromzeros (lua_State *L, array_Array *array)
{
    int i, l;

    for (i = 0, l = 1 ; i < array->rank ; l *= array->size[i], i += 1);

    array->free = FREE_BOTH;
    array->length = l * sizeof_element (array->type);
    array->values.any = malloc (array->length);

    zero_elements (array, 0, l);
    construct (L, array);
}

static int typeerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

static array_Array *testarray (lua_State *L, int index)
{
    index = absolute (L, index);

    if (!lua_type (L, index) == LUA_TUSERDATA || !lua_getmetatable (L, index)) {
	return NULL;
    } else {
	if (lua_topointer (L, -1) == signature) {
	    lua_pop (L, 1);
	    return lua_touserdata (L, index);
	} else if (lua_getfield (L, -1, "__array"),
		   lua_toboolean (L, -1)) {
	    lua_pop (L, 2);
	    return lua_touserdata (L, index);
	} else {
	    lua_pop (L, 1);
	    return NULL;
	}
    }
}

array_Array *array_testarray (lua_State *L, int index)
{
    index = absolute (L, index);

    if (lua_type (L, index) == LUA_TTABLE) {
	array_toarray (L, index, ARRAY_TDOUBLE, 0);
	lua_replace (L, index);
    }

    return testarray (L, index);
}

array_Array *array_checkarray (lua_State *L, int index)
{
    array_Array *array;
    
    index = absolute (L, index);

    if (!(array = array_testarray (L, index))) {
	typeerror(L, index, "array");
    }

    return array;
}

array_Array *array_checkcompatible (lua_State *L, int index, int what, ...)
{        
    va_list ap;
    array_Array *array;
    array_Type type;
    int rank;

    index = absolute (L, index);

    va_start (ap, what);
	    
    if (what & ARRAY_TYPE) {
        type = va_arg (ap, array_Type);
    } else {
        type = ARRAY_TDOUBLE;
    }
    
    if (lua_type (L, index) == LUA_TTABLE) {
	array_toarray (L, index, type, 0);
	lua_replace (L, index);
    }
    
    array = array_checkarray (L, index);

    if (what & ARRAY_TYPE) {
        if (array->type != type) {
            luaL_argerror (L, index, "expected array of different type");
        }
    }

    if (what & ARRAY_RANK) {
        rank = va_arg (ap, int);

        if (array->rank != rank) {
            luaL_argerror (L, index,
                           "expected array of different dimensionality");
        }
    }

    if (what & ARRAY_SIZE) {
	int i, l;

        assert (what & ARRAY_RANK);
        
	for (i = 0 ; i < rank ; i += 1) {
	    l = va_arg (ap, int);
	    
	    if (l > 0 && array->size[i] != l) {
		luaL_argerror (L, index,
			       "expected array of different size");
	    }
	}
    }

    va_end (ap);

    return array;
}

array_Array *array_testcompatible (lua_State *L, int index, int what, ...)
{
    va_list ap;
    array_Array *array;
    array_Type type;
    int rank;

    index = absolute (L, index);

    va_start (ap, what);
	    
    if (what & ARRAY_TYPE){
        type = va_arg (ap, array_Type);
    } else {
        type = ARRAY_TDOUBLE;
    }
    
    if (lua_type (L, index) == LUA_TTABLE) {
	array_toarray (L, index, type, 0);
	lua_replace (L, index);
    }
    
    array = array_testarray (L, index);

    if (!array) {
        return NULL;
    }
    
    if (what & ARRAY_TYPE){
        if (array->type != type) {
            return NULL;
        }
    }

    if (what & ARRAY_RANK) {
        rank = va_arg (ap, int);

        if (array->rank != rank) {
            return NULL;
        }
    }

    if (what & ARRAY_SIZE) {
        int i;
        
        assert (what & ARRAY_RANK);
	    
        for (i = 0 ; i < rank ; i += 1) {
            if (array->size[i] != va_arg (ap, int)) {
                return NULL;
            }
        }
    }
    
    va_end (ap);

    return array;
}

array_Array *array_pusharray (lua_State *L, array_Array *array)
{
    return construct (L, array);
}

void array_initializev (array_Array *array, array_Type type,
                        void *values, int rank, int *size)
{
    int j, l;

    array->type = type;
    array->rank = rank;
    array->size = malloc (rank * sizeof(int));

    for (j = 0, l = 1 ; j < rank ; j += 1) {
	array->size[j] = size[j];
	l *= array->size[j];
    }

    array->free = FREE_BOTH;
    array->length = l * sizeof_element (type);
    array->values.any = malloc (array->length);

    if (values) {
	copy_values (array, values, 0, 0, l);
    }    
}

void array_initialize (array_Array *array, array_Type type, void *values,
                       int rank, ...)
{
    va_list ap;
    int j, size[rank];
    
    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
	size[j] = va_arg(ap, int);
    }
   
    va_end(ap);

    array_initializev (array, type, values, rank, size);
}

array_Array *array_createarrayv (lua_State *L, array_Type type,
                                 void *values, int rank, int *size)
{
    array_Array array;

    array_initializev (&array, type, values, rank, size);
    return construct (L, &array);
}

array_Array *array_createarray (lua_State *L, array_Type type,
                                void *values, int rank, ...)
{
    va_list ap;
    int j, size[rank];
    
    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
	size[j] = va_arg(ap, int);
    }
   
    va_end(ap);

    return array_createarrayv (L, type, values, rank, size);
}

void array_toarrayv (lua_State *L, int index, array_Type type,
		     int rank, int *size)
{
    array_Array array;
    int j;

    index = absolute (L, index);

    if (lua_type (L, index) != LUA_TNIL &&
	lua_type (L, index) != LUA_TSTRING &&
	lua_type (L, index) != LUA_TUSERDATA &&
	lua_type (L, index) != LUA_TLIGHTUSERDATA &&
	lua_type (L, index) != LUA_TTABLE) {
	lua_pushfstring (L,
			 "Initialization from incompatible value "
			 "(expected string or table, got %s).",
			 lua_typename (L, lua_type (L, index)));
	lua_error (L);
    }

    array.type = type;

    if (rank < 1) {
	if (lua_type(L, index) == LUA_TSTRING) {
	    array.rank = 1;
	    
	    array.size = malloc (sizeof (int));
	    array.size[0] = lua_rawlen(L, index) / sizeof_element (array.type);
	} else if (lua_type(L, index) == LUA_TTABLE) {
	    array.rank = 0;
	    array.size = NULL;
	} else {
	    lua_pushstring (L, "Array dimensions undefined.");
	    lua_error (L);
	}
    } else {
	array.rank = rank;
	array.size = malloc (rank * sizeof (int));
	
	for (j = 0 ; j < rank ; j += 1) {
	    array.size[j] = size[j];
	}
    }

    if (lua_type(L, index) == LUA_TSTRING) {
	fromstring (L, index, &array);

        /* Make a reference to the string since we're pointing
         * into its memory. */
            
        lua_createtable(L, 1, 0);
        lua_pushvalue(L, index);
        lua_rawseti(L, -2, 1);
        lua_setuservalue(L, -2);
    } else if (lua_type(L, index) == LUA_TTABLE) {
	fromtable (L, index, &array);
    } else if (lua_type(L, index) == LUA_TUSERDATA) {
	fromuserdata (L, index, &array);

        /* Make a reference to the userdata since we're pointing
         * into its memory. */
            
        lua_createtable(L, 1, 0);
        lua_pushvalue(L, index);
        lua_rawseti(L, -2, 1);
        lua_setuservalue(L, -2);
    } else {
	fromzeros (L, &array);
    }
}

void array_toarray (lua_State *L, int index, array_Type type, int rank, ...)
{
    va_list ap;
    int j, size[rank];
    
    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
	size[j] = va_arg(ap, int);
    }
   
    va_end(ap);

    array_toarrayv (L, index, type, rank, size);
}

void array_castv (lua_State *L, int index, int rank, int *size)
{
    array_Array cast, *array;
    int j, l, m;

    array = array_testarray (L, index);
    cast.size = malloc (rank * sizeof (int));
    
    for (j = 0, l = 1 ; j < rank ; j += 1) {
	cast.size[j] = size[j];
	l *= size[j];
    }

    for (j = 0, m = 1 ; j < array->rank ; j += 1) {
	m *= array->size[j];
    }

    if (l != m) {
	lua_pushstring (L, "Incompatible cast dimensions.");
	lua_error (L);
    }
	
    cast.type = array->type;
    cast.length = array->length;
    cast.rank = rank;
    cast.values.any = array->values.any;
    cast.free = FREE_SIZE;
    
    /* Make a reference to the original array since we're pointing
     * into its memory. */
            
    lua_createtable(L, 1, 0);
    lua_pushvalue(L, index);
    lua_rawseti(L, -2, 1);
    lua_setuservalue(L, -2);

    construct (L, &cast);
}

void array_cast (lua_State *L, int index, int rank, ...)
{
    va_list ap;
    int j, size[rank];
    
    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
	size[j] = va_arg(ap, int);
    }
   
    va_end(ap);

    array_castv (L, index, rank, size);
}

void array_copy (lua_State *L, int index)
{
    array_Array *array, copy;
    int i, d;
    
    array = array_testarray(L, index);

    copy.type = array->type;
    copy.length = array->length;
    copy.rank = array->rank;
    copy.free = FREE_BOTH;

    copy.size = malloc (array->rank * sizeof (int));
    memcpy(copy.size, array->size,array->rank * sizeof (int));
    
    for (i = 0, d = 1;
	 i < array->rank;
	 d *= array->size[i], i += 1);

    copy.values.any = malloc (copy.length);
    copy_elements(&copy, array, 0, 0, d);

    construct (L, &copy);
}

void array_set (lua_State *L, int index, lua_Number c)
{
    array_Array *array;
    int i, d;
    
    array = array_testarray(L, index);
    
    for (i = 0, d = 1;
	 i < array->rank;
	 d *= array->size[i], i += 1);

    for (i = 0 ; i < d ; i += 1) {
	write_element (array, i, c);
    }
}

static void cut (array_Array *source, array_Array *sink, int from, int to,
                 int l, int m, int d, int *range)
{
    int i;
	
    if (d < source->rank - 1) {
	int s, t;

	s = l / source->size[d];
	t = m / sink->size[d];
	
	for (i = 0 ; i < range[1] - range[0] + 1 ; i += 1) {
	    cut (source, sink,
		 from + (range[0] - 1 + i) * s,
		 to + i * t,
		 s, t, d + 1, range + 2);
	}
    } else {
	copy_elements (sink, source, to, from + (range[0] - 1),
                       (range[1] - range[0] + 1));
    }
}

void array_slicev (lua_State *L, int index, int *slices)
{
    array_Array slice, *array;
    int i, j, l, m;

    array = array_testarray (L, index);
    
    slice.size = malloc (array->rank * sizeof (int));
    memcpy (slice.size, array->size, array->rank * sizeof (int));
    
    for (j = 0, l = 1, m = 1 ; j < array->rank ; j += 1) {
	int a, b;

	/* Check slice ranges against source array bounds. */
	
	for (i = 0 ; i < 2 ; i += 1) {
	    a = slices[2 * j + i];
	    
	    if (a < 1 || a > array->size[j]) {
		lua_pushfstring (L,
				 "Invalid slice range specified "
				 "(%d is out of bounds).", a); 
	    
		lua_error (L);
	    }
	}

	/* Check slice range ordering. */

	a = slices[2 * j];
	b = slices[2 * j + 1];

	if (b < a) {
	    lua_pushfstring (L,
			     "Invalid slice range specified (%d < %d).",
			     b, a);
	    
	    lua_error (L);
	}
	
	slice.size[j] = b - a + 1;

	l *= array->size[j];
	m *= slice.size[j];
    }

    slice.length = m * sizeof_element (array->type);
    slice.values.any = malloc (slice.length);

    cut (array, &slice, 0, 0, l, m, 0, slices);

    slice.type = array->type;
    slice.rank = array->rank;
    slice.free = FREE_BOTH;

    construct (L, &slice);
}

static void adjust(array_Array *source, array_Array *sink, void *defaults,
                   int offset_s, int stride_s, int offset_k, int stride_k,
                   int level, int use_defaults)
{
    int j;
    
    if (level <= sink->rank - 1) {
        stride_s /= source->size[level];
        stride_k /= sink->size[level];

        for (j = 0 ; j < sink->size[level] ; j += 1) {
            adjust (source, sink, defaults,
                    offset_s + j * stride_s, stride_s,
                    offset_k + j * stride_k, stride_k,
                    level + 1, use_defaults || j >= source->size[level]);
        }
    } else {
        assert(level <= source->rank);
        assert(offset_k + stride_k <= sink->length);
        assert(stride_k == stride_s);

        if (use_defaults) {
            if (!defaults) {
                zero_elements (sink, offset_k, stride_k);
            } else {
                copy_values(sink, defaults, offset_k, offset_k, stride_k);
            }
        } else {
            copy_elements(sink, source, offset_k, offset_s, stride_k);
        }
    }
}

array_Array *array_adjustv (lua_State *L, int index, void *defaults, int rank, int *size)
{
    array_Array sink, *source, *array;
    int j, l, m;

    index = absolute (L, index);
    source = array_testcompatible(L, index, ARRAY_RANK, rank);

    if (!source) {
        return NULL;
    }

    /* If no adjustment is required return the array as-is. */
    
    if (!memcmp(source->size, size, rank * sizeof(int))) {
        return source;
    }

    /* Initialize the adjusted array. */

    sink.type = source->type;
    sink.rank = rank;
    sink.size = malloc (rank * sizeof(int));

    for (j = 0, l = 1 ; j < rank ; j += 1) {
	sink.size[j] = size[j];
	l *= sink.size[j];
    }

    sink.free = FREE_BOTH;
    sink.length = l * sizeof_element(sink.type);
    sink.values.any = malloc (sink.length);

    for (j = 0, m = 1;
         j < source->rank;
         m *= source->size[j], j += 1);

    adjust (source, &sink, defaults, 0, m, 0, l, 0, 0);

    array = construct (L, &sink);
    lua_replace (L, index);
    
    return array;
}

array_Array *array_adjust (lua_State *L, int index, void *defaults, int rank, ...)
{
    va_list ap;
    int j, size[rank];
    
    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
	size[j] = va_arg(ap, int);
    }
   
    va_end(ap);

    return array_adjustv (L, index, defaults, rank, size);
}

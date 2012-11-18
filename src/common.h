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


/* Debugging convenience macros. */

#define _TRACE printf ("\033[0;31m%s: %d: \033[0m", __FILE__, __LINE__), printf

#define _PRINTV(_N, _F, _V)                     \
    {                                           \
	int i;                                  \
                                                \
	printf ("(");                           \
                                                \
	for (i = 0 ; i < _N ; i += 1) {         \
	    if (i != _N - 1) {                  \
		printf ("%"_F", ", _V[i]);      \
	    } else {                            \
		printf ("%"_F, _V[i]);          \
	    }                                   \
	}                                       \
                                                \
	printf (")\n");                         \
    }

#define _TRACEV(_N, _F, _V)                     \
    {                                           \
	printf ("\033[0;31m%s: %d: \033[0m",    \
                __FILE__, __LINE__);            \
	_PRINTV(_N, _F, _V);                    \
    }                                           \

#define _TRACEM(_N_0, _N_1, _F, _V)                     \
    {                                                   \
	int j;                                          \
                                                        \
	printf ("\033[0;31m%s: %d: \033[0m(\n",		\
                __FILE__, __LINE__);                    \
                                                        \
	for (j = 0 ; j < _N_0 ; j += 1) {               \
	    printf ("  ");                              \
	    _PRINTV (_N_1, _F, (_V + j * _N_1));        \
	}                                               \
                                                        \
	printf (")\n");					\
    }

#define _TOP(L) _TRACE ("@%d\n", lua_gettop(L))
#define _TYPE(L, i) _TRACE ("%s@%d\n", lua_typename(L, lua_type(L, (i))), i)
#define _STACK(L) {                                                     \
        int i;                                                          \
                                                                        \
        _TRACE ("\033[0;31m\n  >>>>>>>>>>\033[0m\n");                   \
                                                                        \
        for (i = 1 ; i <= lua_gettop(L) ; i += 1) {                     \
            if (lua_type(L, (i)) == LUA_TNUMBER) {                      \
                printf ("\033[0;31m  %d: %s \033[0;32m[%f]\033[0m\n",   \
                        i, lua_typename(L, lua_type(L, (i))),           \
                        lua_tonumber (L, (i)));                         \
            } else if (lua_type(L, (i)) == LUA_TSTRING) {               \
                printf ("\033[0;31m  %d: %s \033[0;32m[%s]\033[0m\n",   \
                        i, lua_typename(L, lua_type(L, (i))),           \
                        lua_tostring (L, (i)));                         \
            } else if (lua_type(L, (i)) == LUA_TUSERDATA) {             \
                printf ("\033[0;31m  %d: %s \033[0;32m[%p]\033[0m\n",   \
                        i, lua_typename(L, lua_type(L, (i))),           \
                        lua_touserdata (L, (i)));                        \
            } else {                                                    \
                printf ("\033[0;31m  %d: %s\033[0m\n",                  \
                        i, lua_typename(L, lua_type(L, (i))));          \
            }                                                           \
        }                                                               \
                                                                        \
        printf ("\033[0;31m  <<<<<<<<<<\n\033[0m\n");                   \
    }
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
#include <lualib.h>
#include <lauxlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "opengl.h"

#include "array/array.h"
#include "techne.h"
#include "graphics.h"
#include "transform.h"
#include "event.h"

static GdkDisplay *display;
static GdkScreen *screen;
static GdkWindow *window;

static enum {
    ORTHOGRAPHIC, PERSPECTIVE, FRUSTUM
} projection;

static int width, height;
static int hide = 1, cursor = 1, decorate = 1;
static double planes[6], frustum[3];
static int focus = LUA_REFNIL, defocus = LUA_REFNIL;
static int configure = LUA_REFNIL, delete = LUA_REFNIL;
static char *title;

static unsigned int *keys;
static int allocated;

static int debugcontext = -1;

static void debug_callback (GLenum source,
                            GLenum type,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar *message,
                            GLvoid *userParam)
{
    int c;
    const char *s;

    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB:
	c = 31;
	s = "error";
	break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
	c = 35;
	s = "deprecated behavior";
	break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
	c = 33;
	s = "undefined behavior";
	break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
	c = 36;
	s = "performance warning";
	break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
	c = 34;
	s = "portability warning";
	break;
    case GL_DEBUG_TYPE_OTHER_ARB:default:
	c = 37;
	s = "";
	break;
    }
    
    t_print_message("%sARB_debug_output (%s %d):%s %s\n",
		    t_ansi_color(c, 1),
		    s, id,
		    t_ansi_color(0, 0),
		    message);

    /* Ignore this kind of message from now on. */
    
    glDebugMessageControlARB (source, type, GL_DONT_CARE,
                              1, &id, GL_FALSE);
}

static void recurse (Node *root, GdkEvent *event)
{
    Node *child;
    
    if ([root respondsTo: @selector(inputWithEvent:)]) {
	[(id)root inputWithEvent: event];
    }
    
    for (child = root->down ; child ; child = child->right) {
	t_begin_interval (child, T_INPUT_PHASE);
	recurse (child, event);
	t_end_interval (child, T_INPUT_PHASE);
    }
}

@implementation Graphics

-(id) initWithName: (char *)name andClass: (char *)class
{
    GdkWindowAttr window_attributes;
    GdkVisual *visual;
    GLXFBConfig *configurations, configuration;
    GLXContext context;
    XVisualInfo *info;
        
    int context_attributes[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB, (GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |
                                GLX_CONTEXT_DEBUG_BIT_ARB),
        None
    };
 
    int visual_attributes[] = {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_RED_SIZE        , 8,
        GLX_GREEN_SIZE      , 8,
        GLX_BLUE_SIZE       , 8,
        GLX_ALPHA_SIZE      , 8,
        GLX_DEPTH_SIZE      , 24,
        GLX_STENCIL_SIZE    , 8,
        GLX_DOUBLEBUFFER    , True,
        /* GLX_SAMPLE_BUFFERS  , 1, */
        /* GLX_SAMPLES         , 4, */
        None
    };

    int argc = 0;
    char **argv = NULL;

    int i, j, r, g, b, a, z, s;

    if (debugcontext < 0) {
	/* Get the configuration. */
    
	lua_getglobal (_L, "options");

	lua_getfield (_L, -1, "debugcontext");
	if (lua_type(_L, -1) == LUA_TBOOLEAN) {
	    debugcontext = lua_toboolean (_L, -1);
	} else {
	    debugcontext = (int)lua_tonumber (_L, -1);
	}
	
	lua_pop (_L, 2);
    }

    self = [super init];
    
    /* Create the rendering context and associated visual. */

    gdk_init(&argc, &argv);

    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);
    
    /* Check the version. */
	
    if (!glXQueryVersion(GDK_DISPLAY_XDISPLAY(display), &i, &j) ||
        (i == 1 && j < 4) || i < 1) {
        t_print_error ("This GLX version is not supported.\n");
        exit(1);
    }

    /* Query the framebuffer configurations. */

    configurations = glXChooseFBConfig(GDK_DISPLAY_XDISPLAY(display),
                                       GDK_SCREEN_XNUMBER(screen), 
                                       visual_attributes, &j);
    if (!configurations) {
        t_print_error("Failed to retrieve any framebuffer configurations.\n");
        exit(1);
    }
        
    t_print_message("Found %d matching framebuffer configurations.\n", j);

    /* Choose a configuration at random and create a GLX context. */
	
    configuration = configurations[0];

    if (debugcontext) {
        context = glXCreateContextAttribsARB(GDK_DISPLAY_XDISPLAY(display),
                                             configuration, 0,
                                             True, context_attributes);

        t_print_message("Created a core profile, forward-compatible, debugging "
                        "GLX context.\n");
    } else {
        context = glXCreateNewContext(GDK_DISPLAY_XDISPLAY(display),
                                      configuration, GLX_RGBA_TYPE, 0, True);
    }

    gdk_display_sync(display);
 
    if (!context) {
        t_print_error ("I could not create a rendering context.\n");
        exit(1);
    }
        
    /* Print useful debug information. */
        
    t_print_message ("The graphics renderer is: '%s %s'.\n",
                     glGetString(GL_RENDERER), glGetString(GL_VERSION));
    t_print_message ("The shading language version is: '%s'.\n",
                     glGetString(GL_SHADING_LANGUAGE_VERSION));
        
    glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                          GLX_RED_SIZE, &r);
        
    glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                          GLX_GREEN_SIZE, &g);
        
    glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                          GLX_BLUE_SIZE, &b);
        
    glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                          GLX_ALPHA_SIZE, &a);
        
    glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                          GLX_DEPTH_SIZE, &z);
        
    glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                          GLX_STENCIL_SIZE, &s);

    t_print_message ("The configuration of the framebuffer is "
                     "[%d, %d, %d, %d, %d, %d].\n", r, g, b, a, z, s);

    t_print_message ("The rendering context is%sdirect.\n",
                     glXIsDirect (GDK_DISPLAY_XDISPLAY(display), context) ?
                     " " : " *not* ");

    /* Get the visual. */
	
    info = glXGetVisualFromFBConfig(GDK_DISPLAY_XDISPLAY(display), configuration);
    visual = gdk_x11_screen_lookup_visual (screen, info->visualid);

    /* Clean up. */
	
    XFree(configurations);
    XFree(info);
    
    /* Create the window. */

    window_attributes.window_type = GDK_WINDOW_TOPLEVEL;
    window_attributes.wclass = GDK_INPUT_OUTPUT;
    window_attributes.wmclass_name = name;
    window_attributes.wmclass_class = class;
    window_attributes.colormap = gdk_colormap_new(visual, FALSE);
    window_attributes.visual = visual;
    window_attributes.width = 640;
    window_attributes.height = 480;
    window_attributes.event_mask = (GDK_STRUCTURE_MASK |
			     GDK_FOCUS_CHANGE_MASK |
			     GDK_BUTTON_PRESS_MASK |
			     GDK_BUTTON_RELEASE_MASK |
			     GDK_KEY_PRESS_MASK |
			     GDK_KEY_RELEASE_MASK |
			     GDK_POINTER_MOTION_MASK |
			     GDK_SCROLL_MASK);

    window = gdk_window_new(gdk_get_default_root_window(),
			    &window_attributes, 
			    GDK_WA_COLORMAP | GDK_WA_VISUAL |
			    GDK_WA_WMCLASS);

    glXMakeCurrent(GDK_DISPLAY_XDISPLAY(display),
                   GDK_WINDOW_XWINDOW(window),
                   context);
	
    if (debugcontext) {
        glDebugMessageCallbackARB (debug_callback, NULL);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

        glDebugMessageControlARB (GL_DONT_CARE,
        			  GL_DONT_CARE,
        			  GL_DEBUG_SEVERITY_HIGH_ARB,
        			  0, NULL,
        			  GL_TRUE);

        glDebugMessageControlARB (GL_DONT_CARE,
        			  GL_DONT_CARE,
        			  GL_DEBUG_SEVERITY_MEDIUM_ARB,
        			  0, NULL,
        			  debugcontext > 1 ? GL_TRUE : GL_FALSE);

        glDebugMessageControlARB (GL_DONT_CARE,
        			  GL_DONT_CARE,
        			  GL_DEBUG_SEVERITY_LOW_ARB,
        			  0, NULL,
        			  debugcontext > 2 ? GL_TRUE : GL_FALSE);
    }
    
    return self;
}

-(void) iterate: (Transform *)root
{
    GdkEvent *event;
    
    t_begin_interval (root, T_INPUT_PHASE);    

    while ((event = gdk_event_get()) != NULL) {
	assert(event);

	if(event->type == GDK_CONFIGURE) {
	    width = ((GdkEventConfigure *)event)->width;
	    height = ((GdkEventConfigure *)event)->height;
	    
	    glViewport (0, 0, width, height);

	    /* If a projection frustum has been specified update the
	     * planes as the viewport aspect ratio has probably
	     * changed. */
	    
	    if (projection == FRUSTUM) {
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity();
		gluPerspective (frustum[0],
				(double)width / height,
				frustum[1],
				frustum[2]);
	    }		

	    t_push_userdata (_L, 1, self);
	    t_call_hook (_L, configure, 1, 0);
	} else if (event->type == GDK_FOCUS_CHANGE) {
	    t_push_userdata (_L, 1, self);

	    if (((GdkEventFocus *)event)->in == TRUE) {
		t_call_hook (_L, focus, 1, 0);
	    } else {
		t_call_hook (_L, defocus, 1, 0);
	    }
	} else if (event->type == GDK_DELETE) {
	    t_call_hook (_L, delete, 0, 0);
	} else {
	    /* Ignore consecutive keypresses for the same key.  These are
	     * always a result of key autorepeat. */
    
	    if (event->type == GDK_KEY_PRESS) {
		unsigned int k;
		int i, j = -1;

		k = ((GdkEventKey *)event)->keyval;
	
		for (i = 0 ; i < allocated ; i += 1) {
		    if (keys[i] == 0) {
			j = i;
		    }
	    
		    if (keys[i] == k) {
			return;
		    }
		}

		if (j >= 0) {
		    keys[j] = k;
		} else {
		    if (i == allocated) {
			allocated += 1;
			keys = realloc (keys, allocated * sizeof(unsigned int));
		    }

		    keys[i] = k;
		}
	    } else if (event->type == GDK_KEY_RELEASE) {
		unsigned int k;
		int i;

		k = ((GdkEventKey *)event)->keyval;
	
		for (i = 0 ; i < allocated ; i += 1) {
		    if (keys[i] == k) {
			keys[i] = 0;
			break;
		    }
		}
	    }
	    
	    /* Pass it on to the node tree. */
		    
	    recurse(root, event);
	}
		
	gdk_event_free (event);
    }

    t_end_interval (root, T_INPUT_PHASE);

    /* Traverse the scene. */

    t_begin_interval (root, T_PREPARE_PHASE);    
    [root prepare];
    t_end_interval (root, T_PREPARE_PHASE);

    t_begin_interval (root, T_TRAVERSE_PHASE);    
    glClear(GL_DEPTH_BUFFER_BIT |
	    GL_COLOR_BUFFER_BIT |
	    GL_STENCIL_BUFFER_BIT);
    
    [root traverse];

    glXSwapBuffers (GDK_DISPLAY_XDISPLAY(display),
                    GDK_WINDOW_XWINDOW(window));
    
    t_end_interval (root, T_TRAVERSE_PHASE);

    t_begin_interval (root, T_FINISH_PHASE);    
    [root finish];
    t_end_interval (root, T_FINISH_PHASE);
}

-(int) _get_window
{
    lua_newtable (_L);
    lua_pushinteger (_L, width);
    lua_rawseti (_L, -2, 1);
    lua_pushinteger (_L, height);
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_hide
{
    lua_pushboolean (_L, hide);

    return 1;
}

-(int) _get_title
{
    lua_pushstring (_L, title);

    return 1;
}

-(int) _get_decorate
{
    lua_pushboolean (_L, decorate);

    return 1;
}

-(int) _get_cursor
{
    lua_pushboolean (_L, cursor);

    return 1;
}

-(int) _get_grabinput
{
    lua_pushboolean (_L, gdk_pointer_is_grabbed ());

    return 1;
}

-(int) _get_canvas
{
    double canvas[4];

    glGetDoublev (GL_COLOR_CLEAR_VALUE, canvas);
    array_createarray (_L, ARRAY_TDOUBLE, canvas, 1, 3);

    return 1;
}

-(int) _get_frustum
{
    int i;
    
    if (projection == FRUSTUM) {
	lua_newtable(_L);
        
	for(i = 0; i < 3; i += 1) {
	    lua_pushnumber(_L, frustum[i]);
	    lua_rawseti(_L, -2, i + 1);
	}
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_pointer
{
    /* Implement this if needed. */
    
    return 0;
}

-(int) _get_perspective
{
    int i;
    
    if (projection == PERSPECTIVE) {
	lua_newtable(_L);
        
	for(i = 0; i < 6; i += 1) {
	    lua_pushnumber(_L, planes[i]);
	    lua_rawseti(_L, -2, i + 1);
	}
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_orthographic
{
    int i;
    
    if (projection == ORTHOGRAPHIC) {
	lua_newtable(_L);
        
	for(i = 0; i < 6; i += 1) {
	    lua_pushnumber(_L, planes[i]);
	    lua_rawseti(_L, -2, i + 1);
	}
    } else {
	lua_pushnil (_L);
    }

    return 1;
}
	
-(int) _get_configure
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, configure);

    return 1;
}
	
-(int) _get_focus
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, focus);

    return 1;
}
	
-(int) _get_defocus
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, defocus);

    return 1;
}
	
-(int) _get_close
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, delete);

    return 1;
}

-(int) _get_renderer
{
    lua_pushstring (_L, (const char *)glGetString(GL_RENDERER));
    lua_pushstring (_L, " ");
    lua_pushstring (_L, (const char *)glGetString(GL_VERSION));
    lua_concat (_L, 3);

    return 1;
}

-(int) _get_screen
{
    lua_newtable (_L);
    lua_pushinteger (_L, gdk_screen_width());
    lua_rawseti (_L, -2, 1);
    lua_pushinteger (_L, gdk_screen_height());
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(void) _set_window
{
    GdkGeometry geometry;
	
    if (lua_istable (_L, 3)) {
	gdk_window_hide(window);
	gdk_window_unfullscreen (window);
	gdk_window_set_geometry_hints (window, NULL, 0);
	    
	lua_pushinteger (_L, 1);
	lua_gettable (_L, 3);
	width = lua_tointeger(_L, -1);

	lua_pushinteger (_L, 2);
	lua_gettable (_L, 3);
	height = lua_tointeger(_L, -1);

	lua_pop (_L, 2);

	gdk_window_resize (window, width, height);
	    
	geometry.min_width = width;
	geometry.min_height = height;
	geometry.max_width = width;
	geometry.max_height = height;

	gdk_window_set_geometry_hints(window, &geometry,
				      GDK_HINT_MIN_SIZE |
				      GDK_HINT_MAX_SIZE);

	if (!hide) {
	    gdk_window_clear (window);
	    gdk_window_show (window);
	    gdk_window_raise (window);
	    /* gdk_window_focus (window, GDK_CURRENT_TIME); */

	    if (width == gdk_screen_width() &&
		height == gdk_screen_height()) {
		gdk_window_fullscreen (window);
	    }
	}
	    
	gdk_window_flush (window);
    }

    /* Always flush when you're done. */
	
    gdk_flush ();

    /* Clear the window's color buffers to the
       canvas color. */
		
    glDrawBuffer (GL_FRONT_AND_BACK);
    glClear (GL_COLOR_BUFFER_BIT);
    glFlush();
    glDrawBuffer (GL_BACK);
}

-(void) _set_hide
{
    /* Update the window if the state has toggled. */

    if (window) {
	if (!hide && lua_toboolean (_L, 3)) {
	    gdk_window_hide (window);
	    gdk_flush();
	}

	if (hide && !lua_toboolean (_L, 3)) {
	    gdk_window_show (window);
	    gdk_window_raise (window);
	    /* gdk_window_focus (window, GDK_CURRENT_TIME); */

	    if (width == gdk_screen_width() &&
		height == gdk_screen_height()) {
		gdk_window_fullscreen (window);
	    }

	    gdk_flush();

	    /* Clear the window's color buffers to the
	       canvas color. */
		
	    glDrawBuffer (GL_FRONT_AND_BACK);
	    glClear (GL_COLOR_BUFFER_BIT);
	    glFlush();
	    glDrawBuffer (GL_BACK);
	}
	    
	gdk_window_flush (window);
    }

    hide = lua_toboolean (_L, 3);
}

-(void) _set_title
{
    gdk_window_set_title (window, (char *) lua_tostring (_L, 3));
}

-(void) _set_decorate
{
    decorate = lua_toboolean (_L, 3);

    gdk_window_set_override_redirect (window, !decorate);
}

-(void) _set_grabinput
{
    if (lua_toboolean (_L, 3)) {
	gdk_pointer_grab (window, TRUE,
			  GDK_BUTTON_PRESS_MASK |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_POINTER_MOTION_MASK,
			  NULL, NULL, GDK_CURRENT_TIME);
    } else {
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
    }
}

-(void) _set_cursor
{
    GdkCursor *new;

    cursor = lua_toboolean (_L, 3);

    new = gdk_cursor_new (cursor ? GDK_ARROW : GDK_BLANK_CURSOR);
    gdk_window_set_cursor (window, new);
    gdk_cursor_destroy(new);
}

-(void) _set_pointer
{
    int x_w, y_w, x_p, y_p;
	
    lua_pushinteger (_L, 1);
    lua_gettable (_L, 3);
    x_p = lua_tointeger (_L, -1);
	
    lua_pushinteger (_L, 2);
    lua_gettable (_L, 3);
    y_p = lua_tointeger (_L, -1);

    lua_pop (_L, 2);
    gdk_window_get_origin (window, &x_w, &y_w);

    gdk_display_warp_pointer (display, screen, x_p + x_w, y_p + y_w);
}

-(void) _set_perspective
{
    int i;

    for (i = 0 ; i < 6 ; i += 1) {
	lua_pushinteger (_L, i + 1);
	lua_gettable (_L, 3);
	planes[i] = lua_tonumber(_L, -1);
	lua_pop (_L, 1);
    }

    projection = PERSPECTIVE;

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glFrustum (planes[0],
	       planes[1],
	       planes[2],
	       planes[3],
	       planes[4],
	       planes[5]);
}

-(void) _set_frustum
{
    int i;

    for (i = 0 ; i < 3 ; i += 1) {
	lua_pushinteger (_L, i + 1);
	lua_gettable (_L, 3);
	frustum[i] = lua_tonumber(_L, -1);
	lua_pop (_L, 1);
    }
    
    projection = FRUSTUM;

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluPerspective (frustum[0],
		    (double)width / height,
		    frustum[1], frustum[2]);
}

-(void) _set_orthographic
{
    int i;

    for (i = 0 ; i < 6 ; i += 1) {
	lua_pushinteger (_L, i + 1);
	lua_gettable (_L, 3);
	planes[i] = lua_tonumber(_L, -1);
	lua_pop (_L, 1);
    }

    projection = ORTHOGRAPHIC;

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (planes[0],
	     planes[1],
	     planes[2],
	     planes[3],
	     planes[4],
	     planes[5]);
}

-(void) _set_canvas
{
    array_Array *array;
    
    array = array_checkcompatible (_L, 3, ARRAY_TDOUBLE, 1, 3);

    glClearColor (array->values.doubles[0],
		  array->values.doubles[1],
		  array->values.doubles[2],
		  0);
}

-(void) _set_configure
{
    luaL_unref (_L, LUA_REGISTRYINDEX, configure);
    configure = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_focus
{
    luaL_unref (_L, LUA_REGISTRYINDEX, focus);
    focus = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_defocus
{
    luaL_unref (_L, LUA_REGISTRYINDEX, defocus);
    defocus = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_close
{
    luaL_unref (_L, LUA_REGISTRYINDEX, delete);
    delete = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_renderer
{
    /* Do nothing. */
}

-(void) _set_screen
{
    /* Do nothing. */
}
@end

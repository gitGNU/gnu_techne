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
#include "layout.h"

static PangoContext *context;
static ShaderMold *handle;

@implementation Layout

+(void) initialize
{
    PangoFontMap *fontmap;

    fontmap = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(fontmap);
}

-(void)init
{
    /* Initialize the node. */

    [super init];

    self->layout = pango_layout_new (context);

    self->texels[0] = 0;
    self->texels[1] = 0;

    self->allocation[0] = 0;
    self->allocation[1] = 0;

    self->wrap = 0;
    self->gravity = -1;
    self->justify = 0;
    self->indent = 0;
    self->spacing = 0;
    self->scale = -1;

    glGenTextures(1, &self->texture);

    /* Create the shader node. */

    lua_pushstring(_L, "shader");
    lua_pushvalue (_L, -2);
    [[LayoutShader alloc] init];

    /* Create the shape node. */

    lua_pushstring(_L, "shape");
    lua_pushvalue (_L, -4);
    [[LayoutShape alloc] init];
    lua_settable (_L, -3);
    lua_settable (_L, -3);
}

-(void) free
{
    glDeleteTextures(1, &self->texture);

    [super free];
}

-(void) measure
{
    GLint v[4];

    if (self->scale > 0) {
        self->content[0] = self->scale * (double)self->texels[0] /
                                         (double)self->texels[1];

        self->content[1] = self->scale;
    } else {
        glGetIntegerv(GL_VIEWPORT, v);

        self->content[0] = (double)self->texels[0] / v[3];
        self->content[1] = (double)self->texels[1] / v[3];
    }
}

-(void) update
{
    cairo_t *cairo;
    cairo_surface_t *surface;

    unsigned char *pixels, *source;
    int i, j, n;

    /* Lay out the text. */

    pango_layout_set_markup (self->layout, self->text, -1);
    pango_layout_get_pixel_size (self->layout,
                                 &self->texels[0],
                                 &self->texels[1]);

    /* Create a Cairo image surface. */

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                          self->texels[0],
                                          self->texels[1]);

    cairo = cairo_create (surface);
    cairo_set_source_rgba (cairo, 0, 0, 0, 0);
    cairo_paint (cairo);

    /* Draw the layout. */

    pango_cairo_update_context (cairo, context);
    pango_cairo_show_layout (cairo, self->layout);

    /* Convert from pre-multiplied alpha BGRA to RGBA. */

    source = cairo_image_surface_get_data (surface);
    n = cairo_image_surface_get_stride (surface);
    pixels = (unsigned char *) malloc (self->texels[0] * self->texels[1] *
                                       4 * sizeof (unsigned char));

    for (j = 0 ; j < self->texels[1] ; j += 1) {
        for (i = 0 ; i < self->texels[0] ; i += 1) {
            unsigned char *p, *q;

            p = source + (self->texels[1] - j - 1) * n + 4 * i;
            q = pixels + 4 * (j * self->texels[0] + i);

            q[0] = p[3] > 0 ? ((double)p[2] / p[3]) * 255 : 0;
            q[1] = p[3] > 0 ? ((double)p[1] / p[3]) * 255 : 0;
            q[2] = p[3] > 0 ? ((double)p[0] / p[3]) * 255 : 0;
            q[3] = p[3];
        }
    }

    /* Load the texture. */

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, self->texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_RGBA,
                 self->texels[0],
                 self->texels[1],
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    /* Clean up. */

    cairo_destroy (cairo);
    cairo_surface_destroy (surface);

    free (pixels);
}

-(int) _get_text
{
    lua_pushstring (_L, self->text);

    return 1;
}

-(int) _get_width
{
    if (self->wrap) {
        lua_pushnumber (_L, self->width);
    } else {
        lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_justify
{
    lua_pushboolean (_L, self->justify);

    return 1;
}

-(int) _get_gravity
{
    lua_pushnumber (_L, self->gravity);

    return 1;
}

-(int) _get_indent
{
    lua_pushnumber (_L, self->indent);

    return 1;
}

-(int) _get_spacing
{
    lua_pushnumber (_L, self->spacing);

    return 1;
}

-(int) _get_tabs
{
    lua_getmetatable (_L, 1);
    lua_replace (_L, 1);
    lua_gettable (_L, 1);

    return 1;
}

-(int) _get_scale
{
    if (self->scale > 0) {
        lua_pushnumber (_L, self->scale);
    } else {
        lua_pushnil (_L);
    }

    return 1;
}

-(void) _set_text
{
    self->text = lua_tostring (_L, 3);

    [self update];
}

-(void) _set_width
{
    if (lua_isnumber (_L, 3)) {
        self->width = lua_tonumber (_L, 3);

        pango_layout_set_width (self->layout, self->width * PANGO_SCALE);
        /* pango_layout_set_wrap (self->layout, PANGO_WRAP_WORD_CHAR); */
        pango_layout_set_ellipsize (self->layout, PANGO_ELLIPSIZE_START);

        self->wrap = 1;
    } else {
        pango_layout_set_width (self->layout, -1);

        self->wrap = 0;
    }

    [self update];
}

-(void) _set_justify
{
    self->justify = lua_toboolean (_L, 3);

    pango_layout_set_justify (self->layout, self->justify ? TRUE : FALSE);

    [self update];
}

-(void) _set_gravity
{
    self->gravity = lua_tonumber (_L, 3);

    if (self->gravity > 0) {
        pango_layout_set_alignment (self->layout, PANGO_ALIGN_RIGHT);
    } else if (self->gravity < 0) {
        pango_layout_set_alignment (self->layout, PANGO_ALIGN_LEFT);
    } else {
        pango_layout_set_alignment (self->layout, PANGO_ALIGN_CENTER);
    }

    [self update];
}

-(void) _set_indent
{
    self->indent = lua_tonumber (_L, 3);

    pango_layout_set_indent (self->layout, self->indent * PANGO_SCALE);

    [self update];
}

-(void) _set_spacing
{
    self->spacing = lua_tonumber (_L, 3);

    pango_layout_set_spacing (self->layout, self->spacing * PANGO_SCALE);

    [self update];
}

-(void) _set_tabs
{
    PangoTabArray *array;
    int i, n;

    if(lua_istable (_L, 3)) {
        lua_len (_L, 3);
        n = lua_tointeger (_L, -1);
        lua_pop (_L, 1);

        array = pango_tab_array_new (n, TRUE);

        for(i = 0 ; i < n ; i += 1) {
            lua_rawgeti (_L, 3, i + 1);
            pango_tab_array_set_tab (array, i, PANGO_TAB_LEFT,
                                     lua_tointeger (_L, -1));

            lua_pop (_L, 1);
        }

        pango_layout_set_tabs (self->layout, array);
        pango_tab_array_free (array);
    } else {
        pango_layout_set_tabs (self->layout, NULL);
    }

    [self update];

    lua_getmetatable (_L, 1);
    lua_replace (_L, 1);
    lua_settable (_L, 1);
}

-(void) _set_scale
{
    if (lua_isnumber(_L, 3)) {
        self->scale = lua_tonumber (_L, 3);
    } else {
        self->scale = -1;
    }
}

@end

@implementation LayoutShader

-(void)init
{
#include "glsl/textured_vertex.h"
#include "glsl/textured_fragment.h"

    const char *private[1] = {"texture"};

    /* Make a reference to the mold to make sure it's not
     * collected. */

    self->layout = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super init];

    /* If this is the first instance create the program. */

    if (!handle) {
        ShaderMold *shader;

        shader = [ShaderMold alloc];

        [shader initWithHandle: &handle];
        [shader declare: 1 privateUniforms: private];
        [shader addSourceString: glsl_textured_vertex for: T_VERTEX_STAGE];
        [shader addSourceString: glsl_textured_fragment for: T_FRAGMENT_STAGE];
        [shader link];
    } else {
        t_pushuserdata(_L, 1, handle);
    }

    [self load];

    t_set_sampler (self, "texture", self->layout->texture);
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference_1);

    [super free];
}

-(void) draw: (int)frame
{
    glDepthMask (GL_FALSE);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glUseProgram (self->name);

    [self bind];
    [super draw: frame];

    glDisable(GL_BLEND);
    glDepthMask (GL_TRUE);
}

@end

@implementation LayoutShape

-(void)init
{
    float uv[4 * 2] = {1, 0,  1, 1,  0, 0,  0, 1};
    float vertices[4 * 2] = {0.5, -0.5,  0.5, 0.5,  -0.5, -0.5,  -0.5, 0.5};

    /* Make a reference to the mold to make sure it's not
     * collected. */

    self->layout = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super initWithMode: GL_TRIANGLE_STRIP];

    /* Create the VBOs.  Positions. */

    glGenBuffers(1, &self->positions);
    glBindBuffer (GL_ARRAY_BUFFER, self->positions);
    glBufferData (GL_ARRAY_BUFFER, 2 * 4 * sizeof(float),
                  vertices, GL_STATIC_DRAW);

    /* Texture coordinates. */

    glGenBuffers(1, &self->mapping);
    glBindBuffer (GL_ARRAY_BUFFER, self->mapping);
    glBufferData (GL_ARRAY_BUFFER, 2 * 4 * sizeof(float),
                  uv, GL_STATIC_DRAW);
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);

    glDeleteBuffers (1, &self->positions);
    glDeleteBuffers (1, &self->mapping);

    [super free];
}

-(void) meetParent: (Shader *)parent
{
    int i;

    [super meetParent: parent];

    /* Bind the VBOs into the VAO. */

    glBindVertexArray(self->name);

    i = glGetAttribLocation(parent->name, "positions");
    glBindBuffer(GL_ARRAY_BUFFER, self->positions);
    glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "mapping");
    glBindBuffer(GL_ARRAY_BUFFER, self->mapping);
    glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(i);
}

-(void) draw: (int)frame
{
    double M[16];

    [super draw: frame];

    t_push_modelview (self->matrix, T_MULTIPLY);
    t_copy_modelview (M);

    /* Stretch the vertices as needed. */

    M[0] *= self->layout->content[0];
    M[4] *= self->layout->content[0];
    M[8] *= self->layout->content[0];

    M[1] *= self->layout->content[1];
    M[5] *= self->layout->content[1];
    M[9] *= self->layout->content[1];

    t_load_modelview (M, T_LOAD);

    glBindVertexArray(self->name);
    glDrawArrays (self->mode, 0, 4);

    t_pop_modelview ();
}
@end

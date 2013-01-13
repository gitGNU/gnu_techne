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

#ifndef _ROAM_H_
#define _ROAM_H_

#define BLOCKING (512)
#define TRIANGLE_POOL (0)
#define DIAMOND_POOL (1)

#define QUEUE_SIZE (65536)
#define TREE_HEIGHT ((context->depth << 1) + 1)

/* Triangle cull flags */

#define RIGHT       (1 << 0)
#define LEFT        (1 << 1)
#define TOP         (1 << 2)
#define BOTTOM      (1 << 3)
#define NEAR        (1 << 4)
#define FAR         (1 << 5)
#define OUT         (1 << 6)
#define ALL_IN      (RIGHT | LEFT | TOP | BOTTOM | NEAR | FAR)

/* Triangle flags */

/* ... */

/* Diamond flags */

#define FLIPPED     (1 << 0)

#define is_locked(n) ((n)->diamond->level >=                \
                      (context->orders[(n)->index] << 1))

#define is_leaf(n) (!(n)->children[0])
#define is_out(n) ((n)->cullbits & OUT)
#define is_primary(n) (n->diamond->flags & FLIPPED ?        \
                       (n)->diamond->triangle != (n) :      \
                       (n)->diamond->triangle == (n))

#define is_pair(n) ((n)->neighbors[2] &&                    \
                    (n)->neighbors[2]->neighbors[2] == (n))

#define is_flagged(d, mask) (((d)->flags & (mask)) != 0)
#define is_fine(d) ((d)->level >= TREE_HEIGHT - 1 || (d)->error == 0.0)
#define is_coarse(d) ((d)->level <= 0 || isinf((d)->error))
#define is_queued(d) ((d) && (d)->queue)
#define is_visible(d) \
    (is_pair((d)->triangle) ?                                                \
     !((d)->triangle->cullbits &                                             \
       (d)->triangle->neighbors[2]->cullbits & OUT) :                        \
     !((d)->triangle->cullbits & OUT))

#define is_splittable(d) ((d) && !is_queued(d) && !is_fine(d) && is_visible(d))
#define is_mergeable(d) (d && !is_queued(d) && !is_coarse(d) &&               \
                         is_leaf((d)->triangle->children[0]) &&               \
                         is_leaf((d)->triangle->children[1]) &&               \
                         is_leaf((d)->triangle->neighbors[2]->children[0]) && \
                         is_leaf((d)->triangle->neighbors[2]->children[1]))

struct block {
    struct block *next;
    
    void *chunks;
};

struct chunk {
    struct chunk *next;
};

struct triangle {
    struct diamond *diamond;
    struct triangle *neighbors[3], *children[2], *parent;
    unsigned char cullbits, flags;
    unsigned short index;
};

struct diamond {
    struct diamond *queue, *left, *right;
    struct triangle *triangle;

    float vertices[2][3], center[3];

    float error;
    unsigned short priority;
    char level, flags;
};

typedef struct context Context;
typedef struct chunk Chunk;
typedef struct block Block;
typedef struct triangle Triangle;
typedef struct diamond Diamond;
typedef struct environment Environment;

#endif

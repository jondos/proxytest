/*
  tre-mem.h - TRE memory allocator interface

  Copyright (C) 2001-2003 Ville Laurikari <vl@iki.fi>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 (June
  1991) as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
//#ifdef LOG_CRIME
#ifndef TRE_MEM_H
#define TRE_MEM_H 1

//#include <stdlib.h>

#define TRE_MEM_BLOCK_SIZE 1024

typedef struct tre_list {
  void *data;
  struct tre_list *next;
} tre_list_t;

typedef struct tre_mem_struct {
  tre_list_t *blocks;
  tre_list_t *current;
  char *ptr;
  size_t n;
  int failed;
  void **provided;
} *tre_mem_t;


tre_mem_t tre_mem_new_impl(int provided, void *provided_block);
void *tre_mem_alloc_impl(tre_mem_t mem, int provided, void *provided_block,
			 size_t size);

/* Returns a new memory allocator or NULL if out of memory. */
#define tre_mem_new()  tre_mem_new_impl(0, NULL)
#define tre_mem_newa() \
  tre_mem_new_impl(1, alloca(sizeof(struct tre_mem_struct)))

/* Allocates a block of `size' bytes from `mem'.  Returns a pointer to the
   allocated block or NULL if an underlying malloc() failed. */
#define tre_mem_alloc(mem, size) tre_mem_alloc_impl(mem, 0, NULL, size)

/* Like tre_mem_alloc but memory is allocated with alloca() instead of
   malloc(). */
#define tre_mem_alloca(mem, size)                                             \
  ((mem)->n >= (size)                                                         \
   ? tre_mem_alloc_impl((mem), 1, NULL, (size))				      \
   : tre_mem_alloc_impl((mem), 1, alloca(TRE_MEM_BLOCK_SIZE), (size)))

/* Frees the memory allocator and all memory allocated with it. */
void tre_mem_destroy(tre_mem_t mem);

#endif /* TRE_MEM_H */

//#endif //LOG_CRIME
/* EOF */

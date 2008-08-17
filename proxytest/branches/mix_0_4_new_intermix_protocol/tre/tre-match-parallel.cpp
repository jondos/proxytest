/*
  tre-match-parallel.c - TRE parallel regex matching engine

  Copyright (C) 2001-2003 Ville Laurikari <vl@iki.fi>.

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

/*
  This algorithm searches for matches basically by reading characters
  in the searched string one by one, starting at the beginning.  All
  matching paths in the TNFA are traversed in parallel.  When two or
  more paths reach the same state, exactly one is chosen according to
  tag ordering rules; if returning submatches is not required it does
  not matter which path is chosen.

  The worst case time required for finding the leftmost and longest
  match, or determining that there is no match, is always linearly
  dependent on the length of the text being searched.

  This algorithm cannot handle TNFAs with back referencing nodes.
  See `tre-match-backtrack.c'.
*/
#include "../StdAfx.h"
#ifdef LOG_CRIME
#include "tre-config.h"
//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif /* HAVE_CONFIG_H */

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

//#include <assert.h>
//#include <stdlib.h>
//#include <string.h>
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif /* HAVE_WCHAR_H */
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif /* HAVE_WCTYPE_H */
#ifndef TRE_WCHAR
#include <ctype.h>
#endif /* !TRE_WCHAR */
//#ifdef HAVE_MALLOC_H
//#include <malloc.h>
//#endif /* HAVE_MALLOC_H */

#include "tre-internal.h"
#include "tre-match-utils.h"
#include "regex.h"
#include "xmalloc.h"



typedef struct {
  tre_tnfa_transition_t *state;
  int *tags;
} tre_tnfa_reach_t;

typedef struct {
  int pos;
  int **tags;
} tre_reach_pos_t;


#ifdef TRE_DEBUG
static void
print_reach(const tre_tnfa_t *tnfa, tre_tnfa_reach_t *reach)
{
  int i;

  while (reach->state != NULL)
    {
      DPRINT((" %p", (void *)reach->state));
      if (tnfa->num_tags > 0)
	{
	  DPRINT(("/"));
	  for (i = 0; i < tnfa->num_tags; i++)
	    {
	      DPRINT(("%d:%d", i, reach->tags[i]));
	      if (i < (tnfa->num_tags-1))
		DPRINT((","));
	    }
	}
      reach++;
    }
  DPRINT(("\n"));

}
#endif /* TRE_DEBUG */

/* XXX - Make sure that this does not refer past the end of very short
         strings (length 0 - 4 chars).
   XXX - Make sure assertions work when len < 0.
   XXX - Use a type for `len' which can hold the maximum value of a `size_t'.
   XXX - Go through the code with a fine comb to spot bugs and optimization
         possibilities.  Fix/implement/document them. */
reg_errcode_t
tre_tnfa_run_parallel(const tre_tnfa_t *tnfa, const void *string, int len,
		      tre_str_type_t type, int *match_tags, int eflags,
		      int *match_end_ofs)
{
  tre_char_t prev_c = 0, next_c = 0;
  const char *str_byte =(const char*) string;
  char *buf;
  int pos = -1, pos_add_next = 1;
  tre_tnfa_transition_t *trans_i;
  tre_tnfa_reach_t *reach, *reach_next, *reach_i, *reach_next_i;
  tre_reach_pos_t *reach_pos;
  int *tag_i;
  int num_tags, i;
  int cflags = tnfa->cflags;
  int reg_notbol = eflags & REG_NOTBOL;
  int reg_noteol = eflags & REG_NOTEOL;
  int reg_newline = cflags & REG_NEWLINE;
  int match_eo = -1;       /* end offset of match (-1 if no match found yet) */
  int new_match = 0;
  int *tmp_tags = NULL;
  int *tmp_iptr;
#ifdef TRE_WCHAR
  const wchar_t *str_wide = string;
#ifdef TRE_MBSTATE
  mbstate_t mbstate;
  memset(&mbstate, '\0', sizeof(mbstate));
#endif /* TRE_MBSTATE */
#endif /* TRE_WCHAR */

  DPRINT(("tre_tnfa_run_parallel, input type %d\n", type));

  if (eflags & REG_NOTAGS)
    num_tags = 0;
  else
    num_tags = tnfa->num_tags;

  /* Allocate memory for temporary data required for matching.  This needs to
     be done for every matching operation to be thread safe.  This allocates
     everything in a single large block from the stack frame using alloca(). */
  {
    int tbytes, rbytes, pbytes, xbytes, total_bytes;
    char *tmp_buf;
    /* Compute the length of the block we need. */
    tbytes = sizeof(*tmp_tags) * num_tags;
    rbytes = sizeof(*reach_next) * (tnfa->num_states + 1);
    pbytes = sizeof(*reach_pos) * tnfa->num_states;
    xbytes = sizeof(int) * num_tags;
    total_bytes =
      (sizeof(long) - 1) * 4 /* for alignment paddings */
      + (rbytes + xbytes * tnfa->num_states) * 2 + tbytes + pbytes;

    /* Allocate the memory. */
    buf = (char*)alloca(total_bytes);
    if (buf == NULL)
      return REG_ESPACE;
    memset(buf, 0, total_bytes);

    /* Get the various pointers within tmp_buf (properly aligned). */
    tmp_tags = (int *)buf;
    tmp_buf = buf + tbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    reach_next = (tre_tnfa_reach_t *)tmp_buf;
    tmp_buf += rbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    reach = (tre_tnfa_reach_t *)tmp_buf;
    tmp_buf += rbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    reach_pos = (tre_reach_pos_t *)tmp_buf;
    tmp_buf += pbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    for (i = 0; i < tnfa->num_states; i++)
      {
	reach[i].tags = (int *)tmp_buf;
	tmp_buf += xbytes;
	reach_next[i].tags = (int *)tmp_buf;
	tmp_buf += xbytes;
      }
  }

  for (i = 0; i < tnfa->num_states; i++)
    reach_pos[i].pos = -1;

  reach_next_i = reach_next;
  GET_NEXT_WCHAR();

#if 1
  /* If only one character can start a match, find it first. */
  if (tnfa->first_char >= 0)
    {
      const char *orig_str = str_byte;
      int first = tnfa->first_char;
      /* This uses strchr() hoping it might be optimized for the
	 target architecture. */
      /* XXX - use memchr(), wcschr() and wmemchr() as well! */
      str_byte = strchr(orig_str, first);
      if (str_byte == NULL)
	return REG_NOMATCH;
      prev_c = *(str_byte - 2);
      next_c = *(str_byte - 1);
      pos += str_byte - orig_str;
      DPRINT(("skipped %d chars\n", str_byte - orig_str));
    }
#endif

#if 0
  /* Skip over characters that cannot possibly be the first character
     of a match. */
  if (tnfa->firstpos_chars != NULL)
    {
      char *chars = tnfa->firstpos_chars;

      if (len < 0)
	{
	  const char *orig_str = str_byte;
   	  /* XXX - use strpbrk() and wcspbrk() because they might be
	     optimized for the target architecture.  Try also strcspn()
	     and wcscspn() and compare the speeds. */
	  while (next_c != L'\0' && !chars[next_c])
	    {
	      next_c = *str_byte++;
	    }
	  prev_c = *(str_byte - 2);
	  pos += str_byte - orig_str;
	  DPRINT(("skipped %d chars\n", str_byte - orig_str));
	}
      else
	{
	  while (pos <= len && !chars[next_c])
	    {
	      prev_c = next_c;
	      next_c = (unsigned char)(*str_byte++);
	      pos++;
	    }
	}
    }
#endif

  DPRINT(("length: %d\n", len));
  DPRINT(("pos:chr/code | states and tags\n"));
  DPRINT(("-------------+------------------------------------------------\n"));

  while (1)
    {
      /* If no match found yet, add the initial states to `reach_next'. */
      if (match_eo < 0)
	{
	  DPRINT((" init >"));
	  trans_i = tnfa->initial;
	  while (trans_i->state != NULL)
	    {
	      if (reach_pos[trans_i->state_id].pos < pos)
		{
		  if (trans_i->assertions
		      && CHECK_ASSERTIONS(trans_i->assertions))
		    {
		      DPRINT(("assertion failed\n"));
		      trans_i++;
		      continue;
		    }

		  DPRINT((" %p", (void *)trans_i->state));
		  reach_next_i->state = trans_i->state;
		  for (i = 0; i < num_tags; i++)
		    reach_next_i->tags[i] = -1;
		  tag_i = trans_i->tags;
		  if (tag_i)
		    while (*tag_i >= 0)
		      {
			reach_next_i->tags[*tag_i] = pos;
			tag_i++;
		      }
		  if (reach_next_i->state == tnfa->final)
		    {
		      DPRINT(("  found empty match\n"));
		      match_eo = pos;
		      new_match = 1;
		      for (i = 0; i < num_tags; i++)
			match_tags[i] = reach_next_i->tags[i];
		    }
		  reach_pos[trans_i->state_id].pos = pos;
		  reach_pos[trans_i->state_id].tags = &reach_next_i->tags;
		  reach_next_i++;
		}
	      trans_i++;
	    }
	  DPRINT(("\n"));
	  reach_next_i->state = NULL;
	}
      else
	{
	  if (num_tags == 0 || reach_next_i == reach_next)
	    /*�We have found a match. */
	    break;
	}

      GET_NEXT_WCHAR();

#ifdef TRE_DEBUG
      DPRINT(("%3d:%2lc/%05d |", pos - 1, (tre_cint_t)prev_c, (int)prev_c));
      print_reach(tnfa, reach_next);
#endif /* TRE_DEBUG */

      /* Check for end of string. */
      if (len < 0)
	{
	  if (prev_c == L'\0')
	    break;
	}
      else
	{
	  if (pos > len)
	    break;
	}

      /* Swap `reach' and `reach_next'. */
      reach_i = reach;
      reach = reach_next;
      reach_next = reach_i;

      /* For each state in `reach', weed out states that don't fulfill the
	 minimal matching conditions. */
      if (tnfa->num_minimals && new_match)
	{
	  new_match = 0;
	  reach_next_i = reach_next;
	  for (reach_i = reach; reach_i->state; reach_i++)
	    {
	      int i;
	      int skip = 0;
	      for (i = 0; tnfa->minimal_tags[i] >= 0; i += 2)
		{
		  int end = tnfa->minimal_tags[i];
		  int start = tnfa->minimal_tags[i + 1];
		  DPRINT(("  Minimal start %d, end %d\n", start, end));
		  if (end >= num_tags)
		    {
		      DPRINT(("  Throwing %p out.\n", reach_i->state));
		      skip = 1;
		      break;
		    }
		  else if (reach_i->tags[start] == match_tags[start]
			   && reach_i->tags[end] < match_tags[end])
		    {
		      DPRINT(("  Throwing %p out because t%d < %d\n",
			      reach_i->state, end, match_tags[end]));
		      skip = 1;
		      break;
		    }
		}
	      if (!skip)
		{
		  int *tmp_iptr;
		  reach_next_i->state = reach_i->state;
		  tmp_iptr = reach_next_i->tags;
		  reach_next_i->tags = reach_i->tags;
		  reach_i->tags = tmp_iptr;
		  reach_next_i++;
		}
	    }
	  reach_next_i->state = NULL;

	  /* Swap `reach' and `reach_next'. */
	  reach_i = reach;
	  reach = reach_next;
	  reach_next = reach_i;
	}

      /* For each state in `reach' see if there is a transition leaving with
	 the current input symbol to a state not yet in `reach_next', and
	 add the destination states to `reach_next'. */
      reach_i = reach;
      reach_next_i = reach_next;
      while (reach_i->state != NULL)
	{
	  trans_i = reach_i->state;
	  while (trans_i->state != NULL)
	    {
	      /* Does this transition match the input symbol? */
	      if (trans_i->code_min <= prev_c &&
		  trans_i->code_max >= prev_c)
		{
		  if (trans_i->assertions
		      && (CHECK_ASSERTIONS(trans_i->assertions)
			  /* Handle character class transitions. */
			  || ((trans_i->assertions & ASSERT_CHAR_CLASS)
			      && !(cflags & REG_ICASE)
			      && !tre_isctype((tre_cint_t)prev_c, trans_i->u.o_class))
			  || ((trans_i->assertions & ASSERT_CHAR_CLASS)
			      && (cflags & REG_ICASE)
			      && (!tre_isctype(tre_tolower((tre_cint_t)prev_c),
					    trans_i->u.o_class)
				  && !tre_isctype(tre_toupper((tre_cint_t)prev_c),
					       trans_i->u.o_class)))
			  || ((trans_i->assertions & ASSERT_CHAR_CLASS_NEG)
			      && neg_char_classes_match(trans_i->neg_classes,
							(tre_cint_t)prev_c,
							cflags & REG_ICASE))))
		    {
		      DPRINT(("assertion failed\n"));
		      trans_i++;
		      continue;
		    }

		  /* Compute the tags after this transition. */
		  for (i = 0; i < num_tags; i++)
		    tmp_tags[i] = reach_i->tags[i];
		  tag_i = trans_i->tags;
		  if (tag_i != NULL)
		    while (*tag_i >= 0)
		      {
			tmp_tags[*tag_i] = pos;
			tag_i++;
		      }

		  if (reach_pos[trans_i->state_id].pos < pos)
		    {
		      /* Found an unvisited node. */
		      reach_next_i->state = trans_i->state;
		      tmp_iptr = reach_next_i->tags;
		      reach_next_i->tags = tmp_tags;
		      tmp_tags = tmp_iptr;
		      reach_pos[trans_i->state_id].pos = pos;
		      reach_pos[trans_i->state_id].tags = &reach_next_i->tags;

		      if (reach_next_i->state == tnfa->final
			  && (match_eo == -1
			      || (num_tags > 0
				  && reach_next_i->tags[0] <= match_tags[0])))
			{
			  DPRINT(("  found match %p\n", trans_i->state));
			  match_eo = pos;
			  new_match = 1;
			  for (i = 0; i < num_tags; i++)
			    match_tags[i] = reach_next_i->tags[i];
			}
		      reach_next_i++;

		    }
		  else
		    {
		      assert(reach_pos[trans_i->state_id].pos == pos);
		      /* Another path has also reached this state.  We choose
			 the winner by examining the tag values for both
			 paths. */
		      if (tag_order(num_tags, tnfa->tag_directions, tmp_tags,
				    *reach_pos[trans_i->state_id].tags))
			{
			  /* The new path wins. */
			  tmp_iptr = *reach_pos[trans_i->state_id].tags;
			  *reach_pos[trans_i->state_id].tags = tmp_tags;
			  if (trans_i->state == tnfa->final)
			    {
			      DPRINT(("  found better match\n"));
			      match_eo = pos;
			      new_match = 1;
			      for (i = 0; i < num_tags; i++)
				match_tags[i] = tmp_tags[i];
			    }
			  tmp_tags = tmp_iptr;
			}
		    }
		}
	      trans_i++;
	    }
	  reach_i++;
	}
      reach_next_i->state = NULL;
    }

  DPRINT(("match end offset = %d\n", match_eo));

  *match_end_ofs = match_eo;
  return match_eo >= 0 ? REG_OK : REG_NOMATCH;
}

/* EOF */
#endif //LOG_CRIME
/*
  regexec.c - TRE POSIX compatible matching functions (and more).

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

#include "regex.h"
#include "tre-internal.h"
#include "xmalloc.h"


/* Fills the POSIX.2 regmatch_t array according to the TNFA tag and match
   endpoint values. */
void
tre_fill_pmatch(size_t nmatch, regmatch_t pmatch[],
		const tre_tnfa_t *tnfa, int *tags, int match_eo)
{
  tre_submatch_data_t *submatch_data;
  unsigned int i, j;
  int *parents;

  i = 0;
  if (match_eo >= 0 && !(tnfa->cflags & REG_NOSUB))
    {
      /* Construct submatch offsets from the tags. */
      DPRINT(("end tag = t%d = %d\n", tnfa->end_tag, match_eo));
      submatch_data = tnfa->submatch_data;
      while (i < tnfa->num_submatches && i < nmatch)
	{
	  if (submatch_data[i].so_tag == tnfa->end_tag)
	    pmatch[i].rm_so = match_eo;
	  else
	    pmatch[i].rm_so = tags[submatch_data[i].so_tag];

	  if (submatch_data[i].eo_tag == tnfa->end_tag)
	    pmatch[i].rm_eo = match_eo;
	  else
	    pmatch[i].rm_eo = tags[submatch_data[i].eo_tag];

	  /* If either of the endpoints were not used, this submatch
	     was not part of the match. */
	  if (pmatch[i].rm_so == -1 || pmatch[i].rm_eo == -1)
	    pmatch[i].rm_so = pmatch[i].rm_eo = -1;

	  DPRINT(("pmatch[%d] = {t%d = %d, t%d = %d}\n", i,
		  submatch_data[i].so_tag, pmatch[i].rm_so,
		  submatch_data[i].eo_tag, pmatch[i].rm_eo));
	  i++;
	}
      /* Reset all submatches that are not within all of their parent
	 submatches. */
      i = 0;
      while (i < tnfa->num_submatches && i < nmatch)
	{
	  if (pmatch[i].rm_eo == -1)
	    assert(pmatch[i].rm_so == -1);
	  assert(pmatch[i].rm_so <= pmatch[i].rm_eo);

	  parents = submatch_data[i].parents;
	  if (parents != NULL)
	    for (j = 0; parents[j] >= 0; j++)
	      {
		DPRINT(("pmatch[%d] parent %d\n", i, parents[j]));
		if (pmatch[i].rm_so < pmatch[parents[j]].rm_so
		    || pmatch[i].rm_eo > pmatch[parents[j]].rm_eo)
		  pmatch[i].rm_so = pmatch[i].rm_eo = -1;
	      }
	  i++;
	}
    }

  while (i < nmatch)
    {
      pmatch[i].rm_so = -1;
      pmatch[i].rm_eo = -1;
      i++;
    }
}


/*
  Wrapper functions for POSIX compatible exact regexp matching.
*/


static int
tre_match(const tre_tnfa_t *tnfa, const void *string, size_t len,
	  tre_str_type_t type, size_t nmatch, regmatch_t pmatch[],
	  int eflags)
{
  reg_errcode_t status;
  int *tags = NULL, eo;
  if (nmatch == 0)
    eflags |= REG_NOTAGS;
  if (tnfa->num_tags > 0 && !(eflags & REG_NOTAGS))
    {
      tags =(int*) alloca(sizeof(*tags) * tnfa->num_tags);
      if (tags == NULL)
	return REG_ESPACE;
    }
  if (tnfa->have_backrefs)
    status = tre_tnfa_run_backtrack(tnfa, string, len, type, tags, eflags, &eo);
  else
    status = tre_tnfa_run_parallel(tnfa, string, len, type, tags, eflags, &eo);

  if (status == REG_OK)
    tre_fill_pmatch(nmatch, pmatch, tnfa, tags, eo);
  return status;
}

int
regnexec(const regex_t *preg, const char *str, size_t len,
	 size_t nmatch, regmatch_t pmatch[], int eflags)
{
  tre_tnfa_t *tnfa = (tre_tnfa_t *)preg->TRE_REGEX_T_FIELD;
#ifdef TRE_MULTIBYTE
  if (MB_CUR_MAX == 1)
    return tre_match(tnfa, str, len, STR_BYTE, nmatch, pmatch, eflags);
  else
    return tre_match(tnfa, str, len, STR_MBS, nmatch, pmatch, eflags);
#else /* !TRE_MULTIBYTE */
  return tre_match(tnfa, str, len, STR_BYTE, nmatch, pmatch, eflags);
#endif /* !TRE_MULTIBYTE */
}

int
regexec(const regex_t *preg, const char *str,
	size_t nmatch, regmatch_t pmatch[], int eflags)
{
  return regnexec(preg, str, -1, nmatch, pmatch, eflags);
}


#ifdef TRE_WCHAR

int
regwnexec(const regex_t *preg, const wchar_t *str, size_t len,
	  size_t nmatch, regmatch_t pmatch[], int eflags)
{
  tre_tnfa_t *tnfa = (void *)preg->TRE_REGEX_T_FIELD;
  return tre_match(tnfa, str, len, STR_WIDE, nmatch, pmatch, eflags);
}

int
regwexec(const regex_t *preg, const wchar_t *str,
	 size_t nmatch, regmatch_t pmatch[], int eflags)
{
  return regwnexec(preg, str, -1, nmatch, pmatch, eflags);
}

#endif /* TRE_WCHAR */


#ifdef TRE_APPROX

/*
  Wrapper functions for approximate regexp matching.
*/

static int
tre_match_approx(const tre_tnfa_t *tnfa, const void *string, size_t len,
		 tre_str_type_t type, regamatch_t *match, regaparams_t params,
		 int eflags)
{
  reg_errcode_t status;
  int *tags = NULL, eo;
  if (match->nmatch == 0)
    eflags |= REG_NOTAGS;
  if (tnfa->num_tags > 0 && !(eflags & REG_NOTAGS))
    {
      tags = alloca(sizeof(*tags) * tnfa->num_tags);
      if (tags == NULL)
	return REG_ESPACE;
    }
  status = tre_tnfa_run_approx(tnfa, string, len, type, tags,
			       match, params, eflags, &eo);
  if (status == REG_OK)
    tre_fill_pmatch(match->nmatch, match->pmatch, tnfa, tags, eo);
  return status;
}

int
reganexec(const regex_t *preg, const char *str, size_t len,
	  regamatch_t *match, regaparams_t params, int eflags)
{
  tre_tnfa_t *tnfa = (void *)preg->TRE_REGEX_T_FIELD;
#ifdef TRE_MULTIBYTE
  if (MB_CUR_MAX == 1)
    return tre_match_approx(tnfa, str, len, STR_BYTE,
			    match, params, eflags);
  else
    return tre_match_approx(tnfa, str, len, STR_MBS,
			    match, params, eflags);
#else /* !TRE_MULTIBYTE */
  return tre_match_approx(tnfa, str, len, STR_BYTE,
			  match, params, eflags);
#endif /* !TRE_MULTIBYTE */
}

int
regaexec(const regex_t *preg, const char *str,
	 regamatch_t *match, regaparams_t params, int eflags)
{
  return reganexec(preg, str, -1, match, params, eflags);
}

#ifdef TRE_WCHAR

int
regawnexec(const regex_t *preg, const wchar_t *str, size_t len,
	   regamatch_t *match, regaparams_t params, int eflags)
{
  tre_tnfa_t *tnfa = (void *)preg->TRE_REGEX_T_FIELD;
  return tre_match_approx(tnfa, str, len, STR_WIDE,
			  match, params, eflags);
}

int
regawexec(const regex_t *preg, const wchar_t *str,
	  regamatch_t *match, regaparams_t params, int eflags)
{
  return regawnexec(preg, str, -1, match, params, eflags);
}

#endif /* TRE_WCHAR */

#endif /* TRE_APPROX */


#endif //LOG_CRIME
/* EOF */

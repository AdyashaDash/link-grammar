/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright (c) 2012, 2014 Linas Vepstas                                */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/


#include <stdarg.h>
#include "analyze-linkage.h"
#include "api-structures.h"
#include "post-process.h"
#include "string-set.h"
#include "structures.h"
#include "word-utils.h"

static Linkage x_create_sublinkage(unsigned int N_links)
{
	Linkage s = (Linkage) xalloc (sizeof(struct Linkage_s));
	memset(&s->pp_data, 0, sizeof(PP_data));

	s->num_links = N_links;
	s->link_array = (Link *) xalloc(N_links * sizeof(Link));
	memset(s->link_array, 0, N_links * sizeof(Link));

	s->pp_info = NULL;
	s->pp_violation = NULL;

	return s;
}

static void free_sublinkage(Linkage s)
{
	size_t i;
	for (i = 0; i < s->num_links; i++) {
		Link* l = &s->link_array[i];
		if (l->link_name != NULL)
		{
			free((void*)l->link_name);
			exfree_connectors(l->rc);
			exfree_connectors(l->lc);
		}
	}
	xfree(s->link_array, s->num_links * sizeof(Link));
	xfree(s, sizeof(struct Linkage_s));
}

/* XXX this should go away */
static void copy_full_link(Link *dst, Link *src)
{
	/* XXX FIXME use stringset here ??? */
	dst->link_name = strdup(src->link_name);
	dst->lw = src->lw;
	dst->rw = src->rw;
	dst->lc = excopy_connectors(src->lc);
	dst->rc = excopy_connectors(src->rc);
}

/**
 * This function defines the cost of a link as a function of its length.
 */
static inline int cost_for_length(int length)
{
	return length-1;
}

/**
 * Computes the cost of the current parse of the current sentence,
 * due to the length of the links.
 */
static size_t compute_link_cost(Linkage lkg)
{
	size_t lcost, i;
	lcost =  0;
	for (i = 0; i < lkg->num_links; i++)
	{
		lcost += cost_for_length(lkg->link_array[i].rw - lkg->link_array[i].lw);
	}
	return lcost;
}

static int unused_word_cost(Linkage lkg)
{
	int lcost;
	size_t i;
	lcost =  0;
	for (i = 0; i < lkg->num_words; i++)
		lcost += (lkg->chosen_disjuncts[i] == NULL);
	return lcost;
}

/**
 * Computes the cost of the current parse of the current sentence
 * due to the cost of the chosen disjuncts.
 */
static double compute_disjunct_cost(Linkage lkg)
{
	size_t i;
	double lcost;
	lcost =  0.0;
	for (i = 0; i < lkg->num_words; i++)
	{
		if (lkg->chosen_disjuncts[i] != NULL)
			lcost += lkg->chosen_disjuncts[i]->cost;
	}
	return lcost;
}

/**
 * This returns a string that is the the GCD of the two given strings.
 * If the GCD is equal to one of them, a pointer to it is returned.
 * Otherwise a new string for the GCD is xalloced and put on the
 * "free later" list.
 */
const char * intersect_strings(Sentence sent, const char * s, const char * t)
{
	int i, j, d;
	const char *w, *s0;
	char u0[MAX_TOKEN_LENGTH]; /* Links are *always* less than 10 chars long */
	char *u;

	if (islower((int) *s)) s++;  /* skip the head-dependent indicator */
	if (islower((int) *t)) t++;  /* skip the head-dependent indicator */

	if (strcmp(s,t) == 0) return s;  /* would work without this */
	i = strlen(s);
	j = strlen(t);
	if (j > i) {
		w = s; s = t; t = w;
	}
	/* s is now the longer (at least not the shorter) string */
	u = u0;
	d = 0;
	s0 = s;
	while (*t != '\0') {
		if ((*s == *t) || (*t == '*')) {
			*u = *s;
		} else {
			d++;
			if (*s == '*') *u = *t;
			else *u = '^';
		}
		s++; t++; u++;
	}
	if (d == 0) {
		return s0;
	} else {
		strcpy(u, s);   /* get the remainder of s */
		return string_set_add(u0, sent->string_set);
	}
}

/**
 * The name of the link is set to be the GCD of the names of
 * its two endpoints. Must be called after each extract_links(),
 * etc. since that call issues a brand-new set of links into
 * parse_info.
 */
static void compute_link_names(Sentence sent)
{
	size_t i;
	Parse_info pi = sent->parse_info;

	for (i = 0; i < pi->N_links; i++)
	{
		pi->link_array[i].link_name = intersect_strings(sent,
			connector_get_string(pi->link_array[i].lc),
			connector_get_string(pi->link_array[i].rc));
	}
}

/**
 * This uses link_array.  It post-processes
 * this linkage, and prints the appropriate thing.
 */
Linkage_info analyze_thin_linkage(Sentence sent, Parse_Options opts, int analyze_pass)
{
	size_t i;
	Linkage_info li;
	PP_node * pp;
	Postprocessor * postprocessor = sent->dict->postprocessor;
	Parse_info pi = sent->parse_info;
	Linkage sublinkage = x_create_sublinkage(pi->N_links);

	compute_link_names(sent);
	for (i=0; i<sublinkage->num_links; i++)
	{
	  copy_full_link(&(sublinkage->link_array[i]), &(pi->link_array[i]));
	}

	if (analyze_pass == PP_FIRST_PASS)
	{
		post_process_scan_linkage(postprocessor, opts, sent, sublinkage);
		free_sublinkage(sublinkage);
		memset(&li, 0, sizeof(li));
		return li;
	}

	pp = do_post_process(postprocessor, opts, sent, sublinkage, true);

	memset(&li, 0, sizeof(li));
	li.N_violations = 0;
	li.unused_word_cost = unused_word_cost(sublinkage);
	if (opts->use_sat_solver)
	{
		li.disjunct_cost = 0.0;
	}
	else
	{
		li.disjunct_cost = compute_disjunct_cost(sublinkage);
	}
	li.link_cost = compute_link_cost(sublinkage);
	li.corpus_cost = -1.0;

	if (pp == NULL)
	{
		if (postprocessor != NULL) li.N_violations = 1;
	}
	else if (pp->violation != NULL)
	{
		li.N_violations++;
		li.pp_violation_msg = pp->violation;
	}

	free_sublinkage(sublinkage);
	return li;
}

void extract_thin_linkage(Sentence sent, Linkage linkage, Parse_Options opts)
{
	size_t i;
	Parse_info pi = sent->parse_info;

	linkage->num_links = pi->N_links;
	linkage->link_array = (Link *) exalloc(linkage->num_links * sizeof(Link));

	linkage->pp_info = NULL;
	linkage->pp_violation = NULL;
	memset(&linkage->pp_data, 0, sizeof(PP_data));

	compute_link_names(sent);
	for (i=0; i<linkage->num_links; ++i)
	{
		copy_full_link(&(linkage->link_array[i]), &(pi->link_array[i]));
	}
}

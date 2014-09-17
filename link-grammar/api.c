/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2013, 2014 Linas Vepstas                        */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "analyze-linkage.h"
#include "corpus/corpus.h"
#include "count.h"
#include "dict-common.h"
#include "disjunct-utils.h"
#include "error.h"
#include "externs.h"
#include "extract-links.h"
#include "fast-match.h"
#include "post-process.h"
#include "preparation.h"
#include "print.h"
#include "prune.h"
#include "regex-morph.h"
#include "resources.h"
#include "sat-solver/sat-encoder.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "tokenize.h"
#include "utilities.h"
#include "word-utils.h"

#ifdef USE_FAT_LINKAGES
#include "and.h"
#endif /* USE_FAT_LINKAGES */

/***************************************************************
*
* Routines for setting Parse_Options
*
****************************************************************/

/**
 * For sorting the linkages in postprocessing
 */

static int VDAL_compare_parse(Linkage_info * p1, Linkage_info * p2)
{
	/* Move the discarded entries to the end of the list */
	if (p1->discarded || p2->discarded) return (p1->discarded - p2->discarded);

	if (p1->N_violations != p2->N_violations) {
		return (p1->N_violations - p2->N_violations);
	}
	else if (p1->unused_word_cost != p2->unused_word_cost) {
		return (p1->unused_word_cost - p2->unused_word_cost);
	}
#ifdef USE_FAT_LINKAGES
	else if (p1->fat != p2->fat) {
		return (p1->fat - p2->fat);
	}
#endif /* USE_FAT_LINKAGES */
	else if (p1->disjunct_cost > p2->disjunct_cost) return 1;
	else if (p1->disjunct_cost < p2->disjunct_cost) return -1;
#ifdef USE_FAT_LINKAGES
	else if (p1->and_cost != p2->and_cost) {
		return (p1->and_cost - p2->and_cost);
	}
#endif /* USE_FAT_LINKAGES */
	else {
		return (p1->link_cost - p2->link_cost);
	}
}

#ifdef USE_CORPUS
static int CORP_compare_parse(Linkage_info * p1, Linkage_info * p2)
{
	double diff = p1->corpus_cost - p2->corpus_cost;

	/* Move the discarded entries to the end of the list */
	if (p1->discarded || p2->discarded) return (p1->discarded - p2->discarded);

	if (fabs(diff) < 1.0e-5)
		return VDAL_compare_parse(p1, p2);
	if (diff < 0.0) return -1;
	return 1;
}
#endif

/**
 * Create and initialize a Parse_Options object
 */
Parse_Options parse_options_create(void)
{
	Parse_Options po;

	init_memusage();
	po = (Parse_Options) xalloc(sizeof(struct Parse_Options_s));

	/* Here's where the values are initialized */
	po->verbosity = 1;
	po->debug = (char *)"";
	po->test = (char *)"";
	po->linkage_limit = 100;

	/* A cost of 2.7 allows the usual cost-2 connectors, plus the
	 * assorted fractional costs, without going to cost 3.0, which
	 * is used only during panic-parsing.
	 */
	po->disjunct_cost = 2.7; /* 3.0 is needed for Russian dicts */
	po->min_null_count = 0;
	po->max_null_count = 0;
	po->islands_ok = false;
	po->use_spell_guess = true;
	po->use_sat_solver = false;
	po->use_viterbi = false;

#ifdef XXX_USE_CORPUS
	/* Use the corpus cost model, if available.
	 * It really does a better job at parse ranking.
	 * Err .. sometimes ...
	 */
	po->cost_model.compare_fn = &CORP_compare_parse;
	po->cost_model.type = CORPUS;
#else /* USE_CORPUS */
	po->cost_model.compare_fn = &VDAL_compare_parse;
	po->cost_model.type = VDAL;
#endif /* USE_CORPUS */
	po->short_length = 10;
	po->all_short = false;
	po->twopass_length = 30;
	po->repeatable_rand = true;
	po->resources = resources_create();
#ifdef USE_FAT_LINKAGES
	po->use_fat_links = false;
#endif /* USE_FAT_LINKAGES */
	po->use_cluster_disjuncts = false;
	po->display_morphology = false;

	return po;
}

int parse_options_delete(Parse_Options  opts)
{
	resources_delete(opts->resources);
	xfree(opts, sizeof(struct Parse_Options_s));
	return 0;
}

void parse_options_set_cost_model_type(Parse_Options opts, Cost_Model_type cm)
{
	switch(cm) {
	case VDAL:
		opts->cost_model.type = VDAL;
		opts->cost_model.compare_fn = &VDAL_compare_parse;
		break;
	case CORPUS:
#ifdef USE_CORPUS
		opts->cost_model.type = CORPUS;
		opts->cost_model.compare_fn = &CORP_compare_parse;
#else
		prt_error("Error: Source code compiled with cost model 'CORPUS' disabled.\n");
#endif
		break;
	default:
		prt_error("Error: Illegal cost model: %d\n", cm);
	}
}

Cost_Model_type parse_options_get_cost_model_type(Parse_Options opts)
{
	return opts->cost_model.type;
}

void parse_options_set_verbosity(Parse_Options opts, int dummy)
{
	opts->verbosity = dummy;
	verbosity = opts->verbosity;
	/* this is one of the only global variables. */
}

int parse_options_get_verbosity(Parse_Options opts) {
	return opts->verbosity;
}

void parse_options_set_debug(Parse_Options opts, const char * dummy)
{
	/* The comma-separated list of functions is limited to this size.
	 * Can be easily dynamically allocated. In any case it is not reentrant
	 * because the "debug" variable is static. */
	static char buff[256];
	size_t len = strlen(dummy);

	if (0 == strcmp(dummy, opts->debug)) return;

	if (0 == len)
	{
		buff[0] = '\0';
	}
	else
	{
		buff[0] = ',';
		strncpy(buff+1, dummy, sizeof(buff)-2);
		if (len < sizeof(buff)-2)
		{
			buff[len+1] = ',';
			buff[len+2] = '\0';
		}
		else
		{
			buff[sizeof(buff)-1] = '\0';
		}
	}
	opts->debug = buff;
	debug = opts->debug;
	/* this is one of the only global variables. */
}

char * parse_options_get_debug(Parse_Options opts) {
	return opts->debug;
}

void parse_options_set_test(Parse_Options opts, const char * dummy)
{
	/* The comma-separated test features is limited to this size.
	 * Can be easily dynamically allocated. In any case it is not reentrant
	 * because the "test" variable is static. */
	static char buff[256];
	size_t len = strlen(dummy);

	if (0 == strcmp(dummy, opts->test)) return;

	if (0 == len)
	{
		buff[0] = '\0';
	}
	else
	{
		buff[0] = ',';
		strncpy(buff+1, dummy, sizeof(buff)-2);
		if (len < sizeof(buff)-2)
		{
			buff[len+1] = ',';
			buff[len+2] = '\0';
		}
		else
		{
			buff[sizeof(buff)-1] = '\0';
		}
	}
	opts->test = buff;
	test = opts->test;
	/* this is one of the only global variables. */
}

char * parse_options_get_test(Parse_Options opts) {
	return opts->test;
}

void parse_options_set_use_sat_parser(Parse_Options opts, bool dummy) {
#ifdef USE_SAT_SOLVER
	opts->use_sat_solver = dummy;
#else
	prt_error("Error: cannot enable the Boolean SAT parser; this "
	          " library was built without SAT solver support.\n");
#endif
}

bool parse_options_get_use_sat_parser(Parse_Options opts) {
	return opts->use_sat_solver;
}

void parse_options_set_use_viterbi(Parse_Options opts, bool dummy) {
	opts->use_viterbi = dummy;
}

bool parse_options_get_use_viterbi(Parse_Options opts) {
	return opts->use_viterbi;
}

#ifdef USE_FAT_LINKAGES
void parse_options_set_use_fat_links(Parse_Options opts, int dummy) {
	opts->use_fat_links = dummy;
}
int parse_options_get_use_fat_links(Parse_Options opts) {
	return opts->use_fat_links;
}
#endif /* USE_FAT_LINKAGES */

void parse_options_set_linkage_limit(Parse_Options opts, int dummy)
{
	opts->linkage_limit = dummy;
}
int parse_options_get_linkage_limit(Parse_Options opts)
{
	return opts->linkage_limit;
}

void parse_options_set_disjunct_cost(Parse_Options opts, double dummy)
{
	opts->disjunct_cost = dummy;
}
double parse_options_get_disjunct_cost(Parse_Options opts)
{
	return opts->disjunct_cost;
}

void parse_options_set_min_null_count(Parse_Options opts, int val) {
	opts->min_null_count = val;
}
int parse_options_get_min_null_count(Parse_Options opts) {
	return opts->min_null_count;
}

void parse_options_set_max_null_count(Parse_Options opts, int val) {
	opts->max_null_count = val;
}
int parse_options_get_max_null_count(Parse_Options opts) {
	return opts->max_null_count;
}

void parse_options_set_islands_ok(Parse_Options opts, bool dummy) {
	opts->islands_ok = dummy;
}

bool parse_options_get_islands_ok(Parse_Options opts) {
	return opts->islands_ok;
}

void parse_options_set_spell_guess(Parse_Options opts, bool dummy) {
	opts->use_spell_guess = dummy;
}

bool parse_options_get_spell_guess(Parse_Options opts) {
	return opts->use_spell_guess;
}

void parse_options_set_short_length(Parse_Options opts, int short_length) {
	opts->short_length = short_length;
}

int parse_options_get_short_length(Parse_Options opts) {
	return opts->short_length;
}

void parse_options_set_all_short_connectors(Parse_Options opts, bool val) {
	opts->all_short = val;
}

bool parse_options_get_all_short_connectors(Parse_Options opts) {
	return opts->all_short;
}

void parse_options_set_repeatable_rand(Parse_Options opts, bool val) {
	opts->repeatable_rand = val;
}

bool parse_options_get_repeatable_rand(Parse_Options opts) {
	return opts->repeatable_rand;
}

void parse_options_set_max_parse_time(Parse_Options opts, int dummy) {
	opts->resources->max_parse_time = dummy;
}

int parse_options_get_max_parse_time(Parse_Options opts) {
	return opts->resources->max_parse_time;
}

void parse_options_set_max_memory(Parse_Options opts, int dummy) {
	opts->resources->max_memory = dummy;
}

int parse_options_get_max_memory(Parse_Options opts) {
	return opts->resources->max_memory;
}

void parse_options_set_use_cluster_disjuncts(Parse_Options opts, bool dummy) {
	opts->use_cluster_disjuncts = dummy;
}

bool parse_options_get_use_cluster_disjuncts(Parse_Options opts) {
	return opts->use_cluster_disjuncts;
}

int parse_options_get_display_morphology(Parse_Options opts) {
	return opts->display_morphology;
}

void parse_options_set_display_morphology(Parse_Options opts, int dummy) {
	opts->display_morphology = dummy;
}

bool parse_options_timer_expired(Parse_Options opts) {
	return resources_timer_expired(opts->resources);
}

bool parse_options_memory_exhausted(Parse_Options opts) {
	return resources_memory_exhausted(opts->resources);
}

bool parse_options_resources_exhausted(Parse_Options opts) {
	return (resources_timer_expired(opts->resources) || resources_memory_exhausted(opts->resources));
}

void parse_options_reset_resources(Parse_Options opts) {
	resources_reset(opts->resources);
}

/***************************************************************
*
* Routines for postprocessing
*
****************************************************************/

static Linkage_info * linkage_info_new(int num_to_alloc)
{
	Linkage_info *link_info;
	link_info = (Linkage_info *) xalloc(num_to_alloc * sizeof(Linkage_info));
	memset(link_info, 0, num_to_alloc * sizeof(Linkage_info));
	return link_info;
}

static void linkage_info_delete(Linkage_info *link_info, int sz)
{
	int i,j;

	for (i=0; i<sz; i++)
	{
		Linkage_info *lifo = &link_info[i];
		int nwords = lifo->nwords;
		for (j=0; j<nwords; j++)
		{
			if (lifo->disjunct_list_str[j])
				free(lifo->disjunct_list_str[j]);
		}
		free(lifo->disjunct_list_str);
#ifdef USE_CORPUS
		lg_sense_delete(lifo);
#endif
	}
	xfree(link_info, sz * sizeof(Linkage_info));
}

#ifdef USE_FAT_LINKAGES
static void free_andlists(Sentence sent)
{
	size_t L;
	Andlist * andlist, * next;
	for (L=0; L<sent->num_linkages_post_processed; L++) {
		/* printf("%d ", sent->link_info[L].canonical);  */
		/* if (sent->link_info[L].canonical==0) continue; */
		andlist = sent->link_info[L].andlist;
		while(1) {
			if(andlist == NULL) break;
			next = andlist->next;
			xfree((char *) andlist, sizeof(Andlist));
			andlist = next;
		}
	}
	/* printf("\n"); */
}
#endif /* USE_FAT_LINKAGES */

static void free_post_processing(Sentence sent)
{
	if (sent->link_info != NULL) {
		/* postprocessing must have been done */
#ifdef USE_FAT_LINKAGES
		free_andlists(sent);
#endif /* USE_FAT_LINKAGES */
		linkage_info_delete(sent->link_info, sent->num_linkages_alloced);
		sent->link_info = NULL;
	}
}

static void select_linkages(Sentence sent, match_context_t* mchxt, 
                            count_context_t* ctxt,
                            Parse_Options opts)
{
	size_t in;
	size_t N_linkages_found, N_linkages_alloced;
	bool overflowed;
	Linkage_info *link_info;

	free_post_processing(sent);

	overflowed = build_parse_set(sent, mchxt, ctxt, sent->null_count, opts);
	print_time(opts, "Built parse set");

	if (overflowed && (1 < opts->verbosity))
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Warn, "Warning: Count overflow.\n"
		  "Considering a random subset of %zu of an unknown and large number of linkages\n",
			opts->linkage_limit);
	}
	N_linkages_found = sent->num_linkages_found;

	if (sent->num_linkages_found == 0)
	{
		sent->num_linkages_alloced = 0;
		sent->num_linkages_post_processed = 0;
		sent->num_valid_linkages = 0;
#ifdef USE_FAT_LINKAGES
		sent->num_thin_linkages = 0;
#endif /* USE_FAT_LINKAGES */
		sent->link_info = NULL;
		return;
	}

	if (N_linkages_found > opts->linkage_limit)
	{
		N_linkages_alloced = opts->linkage_limit;
		if (opts->verbosity > 1)
		{
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Warn,
			    "Warning: Considering a random subset of %zu of %zu linkages\n",
			    N_linkages_alloced, N_linkages_found);
		}
	}
	else
	{
		N_linkages_alloced = N_linkages_found;
	}

	link_info = linkage_info_new(N_linkages_alloced);

	/* Generate an array of linkage indices to examine */
	if (overflowed)
	{
		for (in=0; in < N_linkages_alloced; in++)
		{
			link_info[in].index = -(in+1);
		}
	}
	else
	{
		if (opts->repeatable_rand)
			sent->rand_state = N_linkages_found + sent->length;

		for (in=0; in<N_linkages_alloced; in++)
		{
			size_t block_bottom, block_top;
			double frac = (double) N_linkages_found;

			frac /= (double) N_linkages_alloced;
			block_bottom = (int) (((double) in) * frac);
			block_top = (int) (((double) (in+1)) * frac);
			link_info[in].index = block_bottom +
				(rand_r(&sent->rand_state) % (block_top-block_bottom));
		}
	}

	sent->link_info = link_info;
	sent->num_linkages_alloced = N_linkages_alloced;
	/* Later we subtract the number of invalid linkages */
	sent->num_valid_linkages = N_linkages_alloced;

	/* Allow use of both old and new pp order, for tests.
	 * FIXME In release code this assignment can be removed if sane_morphism()
	 * is modified to loop over num_linkages_alloced. */
	sent->num_linkages_post_processed = N_linkages_alloced;
}

static void post_process_linkages(Sentence sent, Parse_Options opts)
{
	Linkage_info *lifo;
	size_t in;
	size_t N_linkages_post_processed = 0;
	size_t N_valid_linkages = sent->num_valid_linkages;
	size_t N_linkages_alloced = sent->num_linkages_alloced;
#ifdef USE_FAT_LINKAGES
	size_t N_thin_linkages;
	bool only_canonical_allowed;
	bool canonical;
#endif /* USE_FAT_LINKAGES */

#ifdef USE_FAT_LINKAGES
	/* When we're processing only a small subset of the linkages,
	 * don't worry about restricting the set we consider to be
	 * canonical ones.  In the extreme case where we are only
	 * generating 1 in a million linkages, it's very unlikely
	 * that we'll hit two symmetric variants of the same linkage
	 * anyway.
	 */
	only_canonical_allowed = !(overflowed || (N_linkages_found > 2*opts->linkage_limit));
#endif /* USE_FAT_LINKAGES */

	/* (optional) first pass: just visit the linkages */
	/* The purpose of these two passes is to make the post-processing
	 * more efficient.  Because (hopefully) by the time you do the
	 * real work in the 2nd pass you've pruned the relevant rule set
	 * in the first pass.
	 */
	if (sent->length >= opts->twopass_length)
	{
		for (in=0; in < N_linkages_alloced; in++)
		{
			lifo = &sent->link_info[in];
			if (lifo->discarded || lifo->N_violations) continue;
			extract_links(lifo->index, sent->parse_info);
#ifdef USE_FAT_LINKAGES
			if (set_has_fat_down(sent))
			{
				if (only_canonical_allowed && !is_canonical_linkage(sent)) continue;
				analyze_fat_linkage(sent, opts, PP_FIRST_PASS);
			}
			else
#endif /* USE_FAT_LINKAGES */
			{
				analyze_thin_linkage(sent, opts, PP_FIRST_PASS);
			}
			if ((9 == in%10) && resources_exhausted(opts->resources)) break;
		}
	}

	/* second pass: actually perform post-processing */
#ifdef USE_FAT_LINKAGES
	N_thin_linkages = 0;
#endif /* USE_FAT_LINKAGES */
	for (in=0; in < N_linkages_alloced; in++)
	{
		int index;

		lifo = &sent->link_info[in];
		if (lifo->discarded) continue;
		if (lifo->N_violations)
		{
			/* Arrange for displaying "Invalid morphism construction" sentences
			 * if they are not discarded (for debug). */
			N_linkages_post_processed++;
			continue;
		}
		index = lifo->index;
		extract_links(index, sent->parse_info);
#ifdef USE_FAT_LINKAGES
		lifo->fat = false;
		lifo->canonical = true;
		if (set_has_fat_down(sent))
		{
			canonical = is_canonical_linkage(sent);
			if (only_canonical_allowed && !canonical)
			{
				lifo->discarded = true;
				continue;
			}
			*lifo = analyze_fat_linkage(sent, opts, PP_SECOND_PASS);
			lifo->fat = true;
			lifo->canonical = canonical;
		}
		else
#endif /* USE_FAT_LINKAGES */
		{
			*lifo = analyze_thin_linkage(sent, opts, PP_SECOND_PASS);
		}
		if (0 == lifo->N_violations)
		{
#ifdef USE_FAT_LINKAGES
			if (false == lifo->fat)
				N_thin_linkages++;
#endif /* USE_FAT_LINKAGES */
		}
		else
		{
			N_valid_linkages--;
		}
		lifo->index = index;
		lg_corpus_score(sent, lifo);
		N_linkages_post_processed++;
		if ((9 == in%10) && resources_exhausted(opts->resources)) break;
	}

	print_time(opts, "Postprocessed all linkages");

#ifdef USE_FAT_LINKAGES
	if (!resources_exhausted(opts->resources))
	{
		if ((N_linkages_post_processed == 0) &&
		    (N_linkages_found > 0) &&
		    (N_linkages_found <= opts->linkage_limit))
		{
			/* With the current parser, the following sentence will elicit
			 * this error:
			 *
			 * Well, say, Joe, you can be Friar Tuck or Much the miller's
			 * son, and lam me with a quarter-staff; or I'll be the Sheriff
			 * of Nottingham and you be Robin Hood a little while and kill
			 * me.
			 */
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Error, "Error: None of the linkages is canonical\n"
			          "\tN_linkages_post_processed=%zu "
			          "N_linkages_found=%zu\n",
			          N_linkages_post_processed,
			          N_linkages_found);
		}
	}
#endif /* USE_FAT_LINKAGES */

	if (opts->verbosity > 1)
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Info, "Info: %zu of %zu linkages with no P.P. violations\n",
				N_valid_linkages, N_linkages_post_processed);
	}

	sent->num_linkages_alloced = N_linkages_alloced;
	sent->num_linkages_post_processed = N_linkages_post_processed;
	sent->num_valid_linkages = N_valid_linkages;
#ifdef USE_FAT_LINKAGES
	sent->num_thin_linkages = N_thin_linkages;
#endif /* USE_FAT_LINKAGES */
}

static void sort_linkages(Sentence sent, Parse_Options opts)
{
	qsort((void *)sent->link_info, sent->num_linkages_alloced,
	      sizeof(Linkage_info),
	      (int (*)(const void *, const void *))opts->cost_model.compare_fn);

#if 0
	/* num_linkages_post_processed sanity check (ONLY). */
	{
		size_t in;
		size_t N_linkages_post_processed = 0;
		for (in=0; in < sent->num_linkages_alloced; in++)
		{
			Linkage_info *lifo = &sent->link_info[in];
			if (lifo->discarded) break;
			N_linkages_post_processed++;
		}
		assert(sent->num_linkages_post_processed==N_linkages_post_processed,
				 "Bad num_linkages_post_processed (%zu!=%zu)",
				 sent->num_linkages_post_processed, N_linkages_post_processed);
	}
#endif

	print_time(opts, "Sorted all linkages");
}

static void old_post_process_linkages(Sentence sent, match_context_t* mchxt, 
                                  count_context_t* ctxt,
                                  Parse_Options opts)
{
	int *indices;
	size_t in, block_bottom, block_top;
	size_t N_linkages_found, N_linkages_alloced;
	size_t N_linkages_post_processed, N_valid_linkages;
	bool overflowed;
	Linkage_info *link_info;
#ifdef USE_FAT_LINKAGES
	size_t N_thin_linkages;
	bool only_canonical_allowed;
	bool canonical;
#endif /* USE_FAT_LINKAGES */

	free_post_processing(sent);

	overflowed = build_parse_set(sent, mchxt, ctxt, sent->null_count, opts);
	print_time(opts, "Built parse set");

	if (overflowed && (1 < opts->verbosity))
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Warn, "Warning: Count overflow.\n"
		  "Considering a random subset of %zu of an unknown and large number of linkages\n",
			opts->linkage_limit);
	}
	N_linkages_found = sent->num_linkages_found;

	if (sent->num_linkages_found == 0)
	{
		sent->num_linkages_alloced = 0;
		sent->num_linkages_post_processed = 0;
		sent->num_valid_linkages = 0;
#ifdef USE_FAT_LINKAGES
		sent->num_thin_linkages = 0;
#endif /* USE_FAT_LINKAGES */
		sent->link_info = NULL;
		return;
	}

	if (N_linkages_found > opts->linkage_limit)
	{
		N_linkages_alloced = opts->linkage_limit;
		if (opts->verbosity > 1)
		{
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Warn,
			    "Warning: Considering a random subset of %zu of %zu linkages\n",
			    N_linkages_alloced, N_linkages_found);
		}
	}
	else
	{
		N_linkages_alloced = N_linkages_found;
	}

	link_info = linkage_info_new(N_linkages_alloced);
	N_valid_linkages = 0;

	/* Generate an array of linkage indices to examine */
	indices = (int *) xalloc(N_linkages_alloced * sizeof(int));
	if (overflowed)
	{
		for (in=0; in < N_linkages_alloced; in++)
		{
			indices[in] = -(in+1);
		}
	}
	else
	{
		if (opts->repeatable_rand)
			sent->rand_state = N_linkages_found + sent->length;

		for (in=0; in<N_linkages_alloced; in++)
		{
			double frac = (double) N_linkages_found;
			frac /= (double) N_linkages_alloced;
			block_bottom = (int) (((double) in) * frac);
			block_top = (int) (((double) (in+1)) * frac);
			indices[in] = block_bottom +
				(rand_r(&sent->rand_state) % (block_top-block_bottom));
		}
	}

#ifdef USE_FAT_LINKAGES
	/* When we're processing only a small subset of the linkages,
	 * don't worry about restricting the set we consider to be
	 * canonical ones.  In the extreme case where we are only
	 * generating 1 in a million linkages, it's very unlikely
	 * that we'll hit two symmetric variants of the same linkage
	 * anyway.
	 */
	only_canonical_allowed = !(overflowed || (N_linkages_found > 2*opts->linkage_limit));
#endif /* USE_FAT_LINKAGES */

	/* (optional) first pass: just visit the linkages */
	/* The purpose of these two passes is to make the post-processing
	 * more efficient.  Because (hopefully) by the time you do the
	 * real work in the 2nd pass you've pruned the relevant rule set
	 * in the first pass.
	 */
	if (sent->length >= opts->twopass_length)
	{
		for (in=0; in < N_linkages_alloced; in++)
		{
			extract_links(indices[in], sent->parse_info);
#ifdef USE_FAT_LINKAGES
			if (set_has_fat_down(sent))
			{
				if (only_canonical_allowed && !is_canonical_linkage(sent)) continue;
				analyze_fat_linkage(sent, opts, PP_FIRST_PASS);
			}
			else
#endif /* USE_FAT_LINKAGES */
			{
				analyze_thin_linkage(sent, opts, PP_FIRST_PASS);
			}
			if ((9 == in%10) && resources_exhausted(opts->resources)) break;
		}
	}

	/* second pass: actually perform post-processing */
	N_linkages_post_processed = 0;
#ifdef USE_FAT_LINKAGES
	N_thin_linkages = 0;
#endif /* USE_FAT_LINKAGES */
	for (in=0; in < N_linkages_alloced; in++)
	{
		Linkage_info *lifo = &link_info[N_linkages_post_processed];
		extract_links(indices[in], sent->parse_info);
#ifdef USE_FAT_LINKAGES
		lifo->fat = false;
		lifo->canonical = true;
		if (set_has_fat_down(sent))
		{
			canonical = is_canonical_linkage(sent);
			if (only_canonical_allowed && !canonical) continue;
			*lifo = analyze_fat_linkage(sent, opts, PP_SECOND_PASS);
			lifo->fat = true;
			lifo->canonical = canonical;
		}
		else
#endif /* USE_FAT_LINKAGES */
		{
			*lifo = analyze_thin_linkage(sent, opts, PP_SECOND_PASS);
		}
		if (0 == lifo->N_violations)
		{
			N_valid_linkages++;
#ifdef USE_FAT_LINKAGES
			if (false == lifo->fat)
				N_thin_linkages++;
#endif /* USE_FAT_LINKAGES */
		}
		lifo->index = indices[in];
		lg_corpus_score(sent, lifo);
		N_linkages_post_processed++;
		if ((9 == in%10) && resources_exhausted(opts->resources)) break;
	}

	print_time(opts, "Postprocessed all linkages");
	qsort((void *)link_info, N_linkages_post_processed, sizeof(Linkage_info),
		  (int (*)(const void *, const void *)) opts->cost_model.compare_fn);

#ifdef USE_FAT_LINKAGES
	if (!resources_exhausted(opts->resources))
	{
		/* This condition can ony happen w/ fat links */
		if ((N_linkages_post_processed == 0) &&
		    (N_linkages_found > 0) &&
		    (N_linkages_found < opts->linkage_limit))
		{
			/* With the current parser, the following sentence will elicit
			 * this error:
			 *
			 * Well, say, Joe, you can be Friar Tuck or Much the miller's
			 * son, and lam me with a quarter-staff; or I'll be the Sheriff
			 * of Nottingham and you be Robin Hood a little while and kill
			 * me.
			 */
			err_ctxt ec;
			ec.sent = sent;
			err_msg(&ec, Error, "Error: None of the linkages is canonical\n"
			          "\tN_linkages_post_processed=%zu "
			          "N_linkages_found=%zu\n",
			          N_linkages_post_processed,
			          N_linkages_found);
		}
	}
#endif /* USE_FAT_LINKAGES */

	if (opts->verbosity > 1)
	{
		err_ctxt ec;
		ec.sent = sent;
		err_msg(&ec, Info, "Info: %zu of %zu linkages with no P.P. violations\n",
				N_valid_linkages, N_linkages_post_processed);
	}

	print_time(opts, "Sorted all linkages");

	sent->num_linkages_alloced = N_linkages_alloced;
	sent->num_linkages_post_processed = N_linkages_post_processed;
	sent->num_valid_linkages = N_valid_linkages;
#ifdef USE_FAT_LINKAGES
	sent->num_thin_linkages = N_thin_linkages;
#endif /* USE_FAT_LINKAGES */
	sent->link_info = link_info;

	xfree(indices, N_linkages_alloced * sizeof(int));
}

/***************************************************************
*
* Routines for creating and destroying processing Sentences
*
****************************************************************/

/* Its OK if this is racey across threads.  Any mild shuffling is enough. */
static unsigned int global_rand_state;

Sentence sentence_create(const char *input_string, Dictionary dict)
{
	Sentence sent;

	sent = (Sentence) xalloc(sizeof(struct Sentence_s));
	memset(sent, 0, sizeof(struct Sentence_s));
	sent->dict = dict;
	sent->length = 0;
	sent->word = NULL;
	sent->post_quote = NULL;
	sent->num_linkages_found = 0;
	sent->num_linkages_alloced = 0;
	sent->num_linkages_post_processed = 0;
	sent->num_valid_linkages = 0;
	sent->link_info = NULL;
	sent->null_count = 0;
	sent->parse_info = NULL;
	sent->string_set = string_set_create();
	sent->rand_state = global_rand_state;

	sent->q_pruned_rules = false;
#ifdef USE_FAT_LINKAGES
	sent->deletable = NULL;
	sent->is_conjunction = NULL;
	sent->dptr = NULL;
	sent->effective_dist = NULL;
#endif /* USE_FAT_LINKAGES */

#ifdef USE_SAT_SOLVER
	sent->hook = NULL;
#endif /* USE_SAT_SOLVER */

	sent->t_start = 0;
	sent->t_count = 0;

	/* Make a copy of the input */
	sent->orig_sentence = string_set_add (input_string, sent->string_set);

	return sent;
}

#ifdef USE_FAT_LINKAGES
/* XXX Extreme hack alert -- English-language words are used
 * completely naked in the C source code!!! FIXME !!!!
 * That's OK, this code is deprecated/obsolete, and will be
 * removed shortly.  We've added the Russian for now, but
 * really, the Russian dictionary needs to be fixed to support
 * conjunctions properly.
 */
static void set_is_conjunction(Sentence sent)
{
	size_t w;
	const char * s;
	for (w=0; w<sent->length; w++) {
		s = sent->word[w].alternatives[0];
		sent->is_conjunction[w] =
			/* English, obviously ... */
			(strcmp(s, "and") == 0) ||
			(strcmp(s, "or" ) == 0) ||
			(strcmp(s, "but") == 0) ||
			(strcmp(s, "nor") == 0) ||
			/* Russian */
			(strcmp(s, "и") == 0) ||
			(strcmp(s, "или") == 0) ||
			(strcmp(s, "но") == 0) ||
			(strcmp(s, "ни") == 0);
	}
}
#endif /* USE_FAT_LINKAGES */

int sentence_split(Sentence sent, Parse_Options opts)
{
	Dictionary dict = sent->dict;

	/* Cleanup stuff previously allocated. This is because some free
	 * routines depend on sent-length, which might change in different
	 * parse-opts settings.
	 */
#ifdef USE_FAT_LINKAGES
	free_deletable(sent);
#endif /* USE_FAT_LINKAGES */

	/* Tokenize */
	if (!separate_sentence(sent, opts))
	{
		return -1;
	}

	sent->q_pruned_rules = false; /* for post processing */
#ifdef USE_FAT_LINKAGES
	sent->is_conjunction = (char *) xalloc(sizeof(char)*sent->length);
	set_is_conjunction(sent);
	initialize_conjunction_tables(sent);
#endif /* USE_FAT_LINKAGES */

	/* If unknown_word is not defined, then no special processing
	 * will be done for e.g. capitalized words. */
	if (!(dict->unknown_word_defined && dict->use_unknown_word))
	{
		if (!sentence_in_dictionary(sent)) {
			return -2;
		}
	}

	/* Look up each word in the dictionary, collect up all
	 * plausible disjunct expressions for each word.
	 */
	build_sentence_expressions(sent, opts);

	return 0;
}

static void free_sentence_words(Sentence sent)
{
	size_t i;
	for (i = 0; i < sent->length; i++)
	{
		free_X_nodes(sent->word[i].x);
		free_disjuncts(sent->word[i].d);
		free(sent->word[i].alternatives);
	}
	free((void *) sent->word);
	sent->word = NULL;
	free(sent->post_quote);
}

void sentence_delete(Sentence sent)
{
	if (!sent) return;
	sat_sentence_delete(sent);
#ifdef USE_FAT_LINKAGES
	free_AND_tables(sent);
#endif /* USE_FAT_LINKAGES */
	free_sentence_words(sent);
	string_set_delete(sent->string_set);
	if (sent->parse_info) free_parse_info(sent->parse_info);
	free_post_processing(sent);
	post_process_close_sentence(sent->dict->postprocessor);

#ifdef USE_FAT_LINKAGES
	free_deletable(sent);
	free_effective_dist(sent);
	free_analyze(sent);
	if (sent->is_conjunction) xfree(sent->is_conjunction, sizeof(char)*sent->length);
#endif /* USE_FAT_LINKAGES */

	global_rand_state = sent->rand_state;
	xfree((char *) sent, sizeof(struct Sentence_s));
}

int sentence_length(Sentence sent)
{
	if (!sent) return 0;
	return sent->length;
}

int sentence_null_count(Sentence sent)
{
	if (!sent) return 0;
	return sent->null_count;
}

int sentence_num_thin_linkages(Sentence sent)
{
	if (!sent) return 0;
#ifdef USE_FAT_LINKAGES
	return sent->num_thin_linkages;
#else
	return sent->num_valid_linkages;
#endif /* USE_FAT_LINKAGES */
}

int sentence_num_linkages_found(Sentence sent)
{
	if (!sent) return 0;
	return sent->num_linkages_found;
}

int sentence_num_valid_linkages(Sentence sent)
{
	if (!sent) return 0;
	return sent->num_valid_linkages;
}

int sentence_num_linkages_post_processed(Sentence sent)
{
	if (!sent) return 0;
	return sent->num_linkages_post_processed;
}

int sentence_num_violations(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0;
	if (sent->num_linkages_alloced <= i) return 0; /* bounds check */
	return sent->link_info[i].N_violations;
}

int sentence_and_cost(Sentence sent, LinkageIdx i) {
#ifdef USE_FAT_LINKAGES
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0;
	if (sent->num_linkages_alloced <= i) return 0; /* bounds check */
	return sent->link_info[i].and_cost;
#else
	return 0;
#endif /* USE_FAT_LINKAGES */
}

double sentence_disjunct_cost(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0.0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0.0;
	if (sent->num_linkages_alloced <= i) return 0.0; /* bounds check */
	return sent->link_info[i].disjunct_cost;
}

int sentence_link_cost(Sentence sent, LinkageIdx i)
{
	if (!sent) return 0;

	/* The sat solver (currently) fails to fill in link_info */
	if (!sent->link_info) return 0;
	if (sent->num_linkages_alloced <= i) return 0; /* bounds check */
	return sent->link_info[i].link_cost;
}

/* ============================================================== */
/* A kind of morphism post-processing */

static inline bool
	is_AFFIXTYPE_STEM(char infix_mark, const char *a, size_t len)
	{ return (SUBSCRIPT_MARK == a[len]) && (infix_mark == a[len+1]); }
static inline bool
	is_AFFIXTYPE_PREFIX(char infix_mark, const char *a, size_t len)
	{ return infix_mark == a[len-1]; }
static inline bool
	is_AFFIXTYPE_SUFFIX(char infix_mark, const char *a)
	{ return (infix_mark == a[0] && '\0' != a[1]); }
static inline bool
	is_AFFIXTYPE_EMPTY(char infix_mark, const char *a, size_t len)
	{ return (0 == strcmp(a, EMPTY_WORD_MARK)); }

/**
 * This routine solves the problem of mis-linked alternatives,
 * i.e a morpheme in one alternative that is linked to a morpheme in
 * another alternative. This can happen due to the way in which word
 * alternatives are implemeted.
 *
 * It does so by checking that all the chosen disjuncts for each input word
 * come from the same alternative in the word.
 *
 * It also validates that there is one alternative in which all the tokens
 * are chosen. XXX This may disallow island morphemes, and may need to be
 * relaxed.
 *
 * Optionally (if SANEMORPHISM regex is defined in the affix file), it
 * also validates that the morpheme-type sequence is permitted for the
 * language. This is a sanity check of the program and the dictionary.
 *
 * TODO (if needed): support a midle morpheme type.
 */

/* These letters create a string that should be matched by a SANEMORPHISM regex,
 * given in the affix file. The empty word doesn't have a letter. E.g. for the
 * Russian dictionary: "w|ts". It is converted here to: "^((w|ts)b)+$".
 * It matches "wbtsbwbtsbwb" but not "wbtsbwsbtsb". */
#define AFFIXTYPE_PREFIX	'p'	/* prefix */
#define AFFIXTYPE_STEM		't'	/* stem */
#define AFFIXTYPE_SUFFIX	's'	/* suffix */
#define AFFIXTYPE_WORD		'w'	/* regular word */
#define AFFIXTYPE_END		'b'	/* end of input word */

static void sane_morphism(Sentence sent, Parse_Options opts)
{
	size_t N_invalid_morphism = 0;
	size_t lk, i;
	Parse_info pi = sent->parse_info;
	Dictionary afdict = sent->dict->affix_table; /* for INFIX_MARK only */
	char infix_mark = INFIX_MARK;
	int * matched_alts = NULL;       /* number of morphemes that have matched
												 * the chosen disjuncts for the
												 * unsplit_word, so far (index: ai) */
	size_t matched_alts_num = 0;     /* matched_alts number of elements */
#define MATCHED_ALTS_MIN_INC 16     /* don't allocate matched_alts often */

#if 0	/* Now it can always be enabled. Anyway, it is needed for "any". */
	/* Skip checking, if dictionary specifies neither prefixes nor sufixes. */
	if (NULL == afdict) return;
	if ((0 == AFCLASS(afdict, AFDICT_PRE)->length) &&
	    (0 == AFCLASS(afdict, AFDICT_MPRE)->length) &&
	    (0 == AFCLASS(afdict, AFDICT_SUF)->length))
	{
		return;
	}
#endif

	for (lk = 0; lk < sent->num_linkages_post_processed; lk++)
	{
		Linkage_info *lifo = &sent->link_info[lk];
		size_t numalt = 1;               /* number of alternatives */
		size_t ai;                       /* index of checked alternative */
		int unsplit_i = 0;               /* unsplit word index */
		const char * unsplit = NULL;     /* unsplit word */
		char affix_types[MAX_SENTENCE+1] = ""; /* affix types sequence */
		char * affix_types_p = affix_types;
		/* If all the words are null - behave as if everything matched. */
		bool match_found = true;        /* djw matched a morpheme */

		/* Don't bother with linkages that already failed post-processing... */
		if (0 != lifo->N_violations)
			continue;
		extract_links(lifo->index, pi);
		for (i=0; i<sent->length; i++)
		{
			const char * djw;          /* disjunct word - the chosen word */
			size_t djwlen;             /* disjunct total length */
			size_t len;                /* disjunct length w/o subscript */
			const char * mark;         /* char position of SUBSCRIPT_MARK */
			bool empty_word = false;   /* is this an empty word? */
			Disjunct * cdj = pi->chosen_disjuncts[i];

			lgdebug(+4, "Linkage %zu, word %zu/%zu\n", lk+1, i, sent->length);

			/* Ignore island words */
			if (NULL == cdj)
			{
				lgdebug(4, "%zu ignored island word\n", lk+1);
				unsplit = NULL; /* mark it as island */
				continue;
			}

			if (NULL != sent->word[i].unsplit_word)
			{
				/* This is an input word - remember its parameters */
				unsplit_i = i;
				unsplit = sent->word[i].unsplit_word;
				numalt = altlen(sent->word[i].alternatives);
				if (numalt > matched_alts_num)
				{
					matched_alts_num = numalt + MATCHED_ALTS_MIN_INC;
					matched_alts =
						realloc(matched_alts, sizeof(*matched_alts)*matched_alts_num);
				}

				lgdebug(4, "%zu unsplit word %s, alts:", lk+1, unsplit);
				for (ai = 0; ai < numalt; ai++)
				{
					matched_alts[ai] = 0;
					lgdebug(4, " %zu:%s", ai, sent->word[i].alternatives[ai]);
				}
				lgdebug(4, "\n");
			}
			if (NULL == unsplit)
			{
				lgdebug(4, "%zu ignore morphemes of an island word\n", lk+1);
				continue;
			}

			djw = cdj->string;
			djwlen = strlen(djw);
			mark = strchr(djw, SUBSCRIPT_MARK);
			len = NULL != mark ? (size_t)(mark - djw) : djwlen;

			/* Find morpheme type */
			if (is_AFFIXTYPE_EMPTY(infix_mark, djw, djwlen))
			{
				/* Ignore the empty word */;
				empty_word = true;
			}
			else
			if (is_AFFIXTYPE_SUFFIX(infix_mark, djw))
			{
				*affix_types_p = AFFIXTYPE_SUFFIX;
			}
			else
			if (is_AFFIXTYPE_STEM(infix_mark, djw, len))
			{
				*affix_types_p = AFFIXTYPE_STEM;
			}
			else
			if (is_AFFIXTYPE_PREFIX(infix_mark, djw, len))
			{
				*affix_types_p = AFFIXTYPE_PREFIX;
			}
			else
			{
				*affix_types_p = AFFIXTYPE_WORD;
			}

			lgdebug(4, "%zu djw=%s affixtype=%c\n",
			        lk+1, djw, empty_word ? 'E' : *affix_types_p);

			if (! empty_word) affix_types_p++;

			lgdebug(4, "%zu djw %s matched alt#:", lk+1, djw);
			match_found = false;

			/* Compare the chosen word djw to the alternatives */
			for (ai = 0; ai < numalt; ai++)
			{
				const char * s, * t;
				const char *a = sent->word[i].alternatives[ai];
				char downcased[MAX_WORD+1] = "";

				if (-1 == matched_alts[ai])
					continue; /* already didn't match */

				s = a;
try_again:
				t = djw;
				//lgdebug(4, "\n%d COMPARING alt%d s %s djw %s: ", lk+1, (int)ai, s, t);
				/* Rules of match:
				 * A morpheme with a subscript needs an exact match to djw.
				 * A morpheme w/o a subscript needs an exact match to djw
				 * disregarding its subscript.
				 * XXX To check: words that contain a dot as part of them. */
				while ((*s != '\0' && *s != '[') && (*s == *t)) {s++; t++;}
				/* Possibilities:
				 *** Match (the last two are for words ending with [...]):
				 * s==\0 && t==\0
				 * s==\0 && t==SUBSCRIPT_MARK
				 * s==\0 && t==[
				 * s==[ && t==[
				 *** Continue to check:
				 * s==.	&& t==SUBSCRIPT_MARK
				 */
				if (*s == SUBSCRIPT_DOT && *t == SUBSCRIPT_MARK)
				{
					s++; t++;
					while ((*s != '\0') && (*s == *t)) {s++; t++;}
				}
				if (((*s == '\0') &&
				   ((*t == '\0') || (*t == SUBSCRIPT_MARK) || (*t == '[')))
				   ||
				   ((*s == '[') && (*t == '[')))
				{
					//lgdebug(4, "EQUAL\n");
					lgdebug(4, " %zu", ai);
					match_found = true;
					/* Count matched morphemes in this alternative */
					matched_alts[ai]++;
				}
				else {
					//lgdebug(4,"NOTEQ\n");
					/* If we are here, it didn't match. Is that because of
					 * capitalization? Lets check. */
					if ((sent->word[i].firstupper) &&
						('\0' == downcased[0]) && ('\0' != a[0]))
					{
						downcase_utf8_str(downcased, a, MAX_WORD);
						lgdebug(4, "\n");
						lgdebug(4, "%zu downcasing %s>%s\n", lk+1, a, downcased);
						s = downcased;
						goto try_again;
					}
					/* No match, disregard this alternative from now on */
					matched_alts[ai] = -1;
				}
			}
			if (! match_found)
			{
				lgdebug(4, " none\n");
				break;
			}
			lgdebug(4, "\n");

			/* If this is the last morpheme of the alternatives,
			 * then make sure all of them exist in the linkage */
			if ((i+1 == sent->length) || (NULL != sent->word[i+1].unsplit_word))
			{
				int num_morphemes = i - unsplit_i + 1;

				lgdebug(4, "%zu end of input word, num_morphemes %d\n",
						lk+1, num_morphemes);
				*affix_types_p++ = AFFIXTYPE_END;

				/* Make sure that there exists an alternative in
				 * which all the morphemes have been matched.
				 * ??? This disallows island morphemes -
				 * should we allow them? */
				match_found = false;
				for (ai = 0; ai < numalt; ai++)
				{
					if (matched_alts[ai] == num_morphemes)
					{
						match_found = true;
						break;
					}
				}

				if (! match_found)
				{
					lgdebug(4, "%zu morphemes are missing in this linkage\n", lk+1);
					break;
				}
				lgdebug(4, "%zu Perfect match\n", lk+1);
			}
		}
		*affix_types_p = '\0';

		/* Check morpheme type combination.
		 * If null_count > 0, the morpheme type combination may be invalid
		 * due to morpheme islands, so skip this check. */
		if (match_found && (0 == sent->null_count) && ('\0' != affix_types[0]) &&
			(NULL != afdict) && (NULL != afdict->regex_root) &&
			(NULL == match_regex(afdict->regex_root, affix_types)))
		{
			/* Morpheme type combination not valid */
			match_found = false;
			/* XXX we should have a better way to notify */
			if (0 < verbosity)
				printf("Warning: Invalid morpheme type combination %s, "
				       "run with !bad and !verbosity=4 to debug\n", affix_types);
		}
		else
			lgdebug(4, "%zu morpheme type combination '%s'\n", lk+1, affix_types);

		if (match_found)
		{
			lgdebug(4, "%zu SUCCEEDED\n", lk+1);
		}
		else
		{
			/* Oh no ... invalid morpheme combination! */
			N_invalid_morphism++;
			lifo->N_violations++;
			lifo->pp_violation_msg = "Invalid morphism construction.";
			if (!test_enabled("display-invalid-morphism")) lifo->discarded = true;
			lgdebug(4, "%zu FAILED\n", lk+1);
		}
	}

	if (opts->verbosity > 1)
	{
		prt_error("Info: sane_morphism(): %zu of %zu linkages with "
					 "invalid morphism construction\n",
		          N_invalid_morphism, sent->num_valid_linkages);
	}
	sent->num_valid_linkages -= N_invalid_morphism;

	free(matched_alts);
}

static void chart_parse(Sentence sent, Parse_Options opts)
{
	int nl;
	match_context_t * mchxt;
	count_context_t * ctxt;

	/* Build lists of disjuncts */
	prepare_to_parse(sent, opts);
	if (resources_exhausted(opts->resources)) return;

	mchxt = alloc_fast_matcher(sent);
	ctxt = alloc_count_context(sent->length);
	print_time(opts, "Initialized fast matcher");
	if (resources_exhausted(opts->resources)) return;

	/* A parse set may have been already been built for this sentence,
	 * if it was previously parsed.  If so we free it up before
	 * building another.  */
	if (sent->parse_info) free_parse_info(sent->parse_info);
	sent->parse_info = parse_info_new(sent->length);

	for (nl = opts->min_null_count; nl <= opts->max_null_count ; ++nl)
	{
		s64 total;
		if (resources_exhausted(opts->resources)) break;
		sent->null_count = nl;
		total = do_parse(sent, mchxt, ctxt, sent->null_count, opts);

		if (verbosity > 1)
		{
			prt_error("Info: Total count with %zu null links:   %lld\n",
				sent->null_count, total);
		}

		/* total is 64-bit, num_linkages_found is 32-bit. Clamp */
		total = (total>INT_MAX) ? INT_MAX : total;
		total = (total<0) ? INT_MAX : total;

		sent->num_linkages_found = (int) total;
		print_time(opts, "Counted parses");

		if (test_enabled("old-pp-order"))
		{
			old_post_process_linkages(sent, mchxt, ctxt, opts);
			sane_morphism(sent, opts);
		}
		else
		{
			select_linkages(sent, mchxt, ctxt, opts);
			sane_morphism(sent, opts);
			post_process_linkages(sent, opts);
			sort_linkages(sent, opts);
		}
		if (sent->num_valid_linkages > 0) break;

		/* If we are here, then no valid linakges were found.
		 * If there was a parse overflow, give up now. */
		if (PARSE_NUM_OVERFLOW < total) break;
	}

	free_count_context(ctxt);
	free_fast_matcher(mchxt);
}

static void free_sentence_disjuncts(Sentence sent)
{
	size_t i;

	for (i = 0; i < sent->length; ++i)
	{
		free_disjuncts(sent->word[i].d);
		sent->word[i].d = NULL;
	}
}

int sentence_parse(Sentence sent, Parse_Options opts)
{
	int rc;

	verbosity = opts->verbosity;
	debug = opts->debug;
	test = opts->test;

	sent->num_valid_linkages = 0;

	/* If the sentence has not yet been split, do so now.
	 * This is for backwards compatibility, for existing programs
	 * that do not explicitly call the splitter.
	 */
	if (0 == sent->length)
	{
		rc = sentence_split(sent, opts);
		if (rc) return -1;
	}

	/* Check for bad sentence length */
	if (MAX_SENTENCE <= sent->length)
	{
		prt_error("Error: sentence too long, contains more than %d words\n",
			MAX_SENTENCE);
		return -2;
	}

	/* Initialize/free any leftover garbage */
	free_sentence_disjuncts(sent);  /* Is this really needed ??? */
#ifdef USE_FAT_LINKAGES
	if (sentence_contains_conjunction(sent)) free_AND_tables(sent);
#endif /* USE_FAT_LINKAGES */
	resources_reset_space(opts->resources);

	if (resources_exhausted(opts->resources))
		return 0;

#ifdef USE_FAT_LINKAGES
	init_analyze(sent);
#endif /* USE_FAT_LINKAGES */

	/* Expressions were previously set up during the tokenize stage. */
	expression_prune(sent);
	print_time(opts, "Finished expression pruning");
	if (opts->use_sat_solver)
	{
		sat_parse(sent, opts);
	}
	else
	{
		chart_parse(sent, opts);
	}
	print_time(opts, "Finished parse");

	if ((verbosity > 0) &&
	   (PARSE_NUM_OVERFLOW < sent->num_linkages_found))
	{
		prt_error("WARNING: Combinatorial explosion! nulls=%zu cnt=%d\n"
			"Consider retrying the parse with the max allowed disjunct cost set lower.\n"
			"At the command line, use !cost-max\n",
			sent->null_count, sent->num_linkages_found);
	}
	return sent->num_valid_linkages;
}


/**************************************************************************/
/* Copyright (c) 2004                                                     */
/* Daniel Sleator, David Temperley, and John Lafferty                     */
/* Copyright (c) 2014 Linas Vepstas                                       */
/* All rights reserved                                                    */
/*                                                                        */
/* Use of the link grammar parsing system is subject to the terms of the  */
/* license set forth in the LICENSE file included with this software.     */
/* This license allows free redistribution and use in source and binary   */
/* forms, with or without modification, subject to certain conditions.    */
/*                                                                        */
/**************************************************************************/

#include "api-structures.h"
#include "count.h"
#include "externs.h"
#include "fast-match.h"
#include "string-set.h"
#include "word-utils.h"

/**
 * The entire goal of this file is provide a fast lookup of all of the
 * disjuncts on a given word that might be able to connect to a given
 * connector on the left or the right. The main entry point is
 * form_match_list(), which performs this lookup.
 *
 * The lookup is fast, because it uses a precomputed lookup table to
 * find the match candidates.  The lookup table is stocked by looking
 * at all disjuncts on all words, and sorting them into bins organized
 * by connectors they could potentially connect to.  The lookup table
 * is created by calling the alloc_fast_matcher() function.
 *
 * free_fast_matcher() is used to free the matcher.
 * put_match_list() releases the memory that form_match_list returned.
 */

/**
 * returns the number of disjuncts in the list that have non-null
 * left connector lists.
 */
static int left_disjunct_list_length(const Disjunct * d)
{
	int i;
	for (i=0; d!=NULL; d=d->next) {
		if (d->left != NULL) i++;
	}
	return i;
}

static int right_disjunct_list_length(const Disjunct * d)
{
	int i;
	for (i=0; d!=NULL; d=d->next) {
		if (d->right != NULL) i++;
	}
	return i;
}

struct fast_matcher_s
{
	size_t size;
	unsigned int *l_table_size;  /* the sizes of the hash tables */
	unsigned int *r_table_size;

	/* the beginnings of the hash tables */
	Match_node *** l_table;
	Match_node *** r_table;

	/* I'll pedantically maintain my own list of these cells */
	Match_node * mn_free_list;
};


/**
 * Return a match node to be used by the caller
 */
static Match_node * get_match_node(fast_matcher_t *ctxt)
{
	Match_node * m;
	if (ctxt->mn_free_list != NULL)
	{
		m = ctxt->mn_free_list;
		ctxt->mn_free_list = m->next;
	}
	else
	{
		m = (Match_node *) xalloc(sizeof(Match_node));
	}
	return m;
}

/**
 * Put these nodes back onto my free list
 */
void put_match_list(fast_matcher_t *ctxt, Match_node *m)
{
	Match_node * xm;

	for (; m != NULL; m = xm)
	{
		xm = m->next;
		m->next = ctxt->mn_free_list;
		ctxt->mn_free_list = m;
	}
}

static void free_match_list(Match_node * t)
{
	Match_node *xt;
	for (; t!=NULL; t=xt) {
		xt = t->next;
		xfree((char *)t, sizeof(Match_node));
	}
}

/**
 * Free all of the hash tables and Match_nodes
 */
void free_fast_matcher(fast_matcher_t *mchxt)
{
	size_t w;
	unsigned int i;

	for (w = 0; w < mchxt->size; w++)
	{
		for (i = 0; i < mchxt->l_table_size[w]; i++)
		{
			free_match_list(mchxt->l_table[w][i]);
		}
		xfree((char *)mchxt->l_table[w], mchxt->l_table_size[w] * sizeof (Match_node *));
		for (i = 0; i < mchxt->r_table_size[w]; i++)
		{
			free_match_list(mchxt->r_table[w][i]);
		}
		xfree((char *)mchxt->r_table[w], mchxt->r_table_size[w] * sizeof (Match_node *));
	}
	free_match_list(mchxt->mn_free_list);
	mchxt->mn_free_list = NULL;

	xfree(mchxt->l_table_size, mchxt->size * sizeof(unsigned int));
	xfree(mchxt->l_table, mchxt->size * sizeof(Match_node **));
	xfree(mchxt, sizeof(fast_matcher_t));
}

/**
 * Adds the match node m to the sorted list of match nodes l.
 * The parameter dir determines the order of the sorting to be used.
 * Makes the list sorted from smallest to largest.
 */
static Match_node * add_to_right_table_list(Match_node * m, Match_node * l)
{
	Match_node *p, *prev;

	if (l == NULL) return m;

	/* Insert m at head of list */
	if ((m->d->right->word) <= (l->d->right->word)) {
		m->next = l;
		return m;
	}

	/* Walk list to insertion point */
	prev = l;
	p = prev->next;
	while (p != NULL && ((m->d->right->word) > (p->d->right->word))) {
		prev = p;
		p = p->next;
	}

	m->next = p;
	prev->next = m;

	return l;  /* return pointer to original head */
}

/**
 * Adds the match node m to the sorted list of match nodes l.
 * The parameter dir determines the order of the sorting to be used.
 * Makes the list sorted from largest to smallest
 */
static Match_node * add_to_left_table_list(Match_node * m, Match_node * l)
{
	Match_node *p, *prev;

	if (l == NULL) return m;

	/* Insert m at head of list */
	if ((m->d->left->word) >= (l->d->left->word)) {
		m->next = l;
		return m;
	}

	/* Walk list to insertion point */
	prev = l;
	p = prev->next;
	while (p != NULL && ((m->d->left->word) < (p->d->left->word))) {
		prev = p;
		p = p->next;
	}

	m->next = p;
	prev->next = m;

	return l;  /* return pointer to original head */
}

/**
 * Validate that the uppercase part of two connectors are the same.
 * Return true if so, false otherwise.
 * FIXME: Use connector enumeration.
 */
static bool con_uc_eq(const Connector *c1, const Connector *c2)
{

	if (string_set_cmp(c1->string, c2->string)) return true;
	if (c1->hash != c2->hash) return false;
	if (c1->uc_length != c1->uc_length) return false;

	const char *uc1 = &c1->string[c1->uc_start];
	const char *uc2 = &c2->string[c2->uc_start];
	/* We arrive here for less than 50% of the cases for "en" and
	 * less then 20% of the cases for "ru", and the following
	 * condition is always true.
	 * How come 2 different uc strings never have the same hash value? */
	if (0 == strncmp(uc1, uc2, c1->uc_length)) return true;

	return false;
}

static Match_node **get_match_table_entry(unsigned int size, Match_node **t,
                                          Connector * c, int dir)
{
		unsigned int h, s;

		s = h = connector_hash(c) & (size-1);

		if (dir == 1) {
			while (NULL != t[h])
			{
				if (con_uc_eq(t[h]->d->right, c)) break;
				h = (h + 1) & (size-1);
				if (NULL == t[h]) break;
				if (h == s) return NULL;
			}
		}
		else
		{
			while (NULL != t[h])
			{
				if (con_uc_eq(t[h]->d->left, c)) break;
				h = (h + 1) & (size-1);
				if (NULL == t[h]) break;
				if (h == s) return NULL;
			}
		}

		return &t[h];
}

/**
 * The disjunct d (whose left or right pointer points to c) is put
 * into the appropriate hash table
 * dir =  1, we're putting this into a right table.
 * dir = -1, we're putting this into a left table.
 */
static void put_into_match_table(unsigned int size, Match_node ** t,
								 Disjunct * d, Connector * c, int dir )
{
	Match_node *m, **xl;

	m = (Match_node *) xalloc (sizeof(Match_node));
	m->next = NULL;
	m->d = d;

	xl = get_match_table_entry(size, t, c, dir);
	assert(NULL != xl, "get_match_table_entry: Overflow\n");
	if (dir == 1) {
		*xl = add_to_right_table_list(m, *xl);
	}
	else
	{
		*xl = add_to_left_table_list(m, *xl);
	}
}

fast_matcher_t* alloc_fast_matcher(const Sentence sent)
{
	unsigned int size;
	size_t w;
	int len;
	Match_node ** t;
	Disjunct * d;
	fast_matcher_t *ctxt;

	ctxt = (fast_matcher_t *) xalloc(sizeof(fast_matcher_t));
	ctxt->size = sent->length;
	ctxt->l_table_size = xalloc(2 * sent->length * sizeof(unsigned int));
	ctxt->r_table_size = ctxt->l_table_size + sent->length;
	ctxt->l_table = xalloc(2 * sent->length * sizeof(Match_node **));
	ctxt->r_table = ctxt->l_table + sent->length;
	memset(ctxt->l_table, 0, 2 * sent->length * sizeof(Match_node **));
	ctxt->mn_free_list = NULL;

	for (w=0; w<sent->length; w++)
	{
		len = left_disjunct_list_length(sent->word[w].d);
		size = next_power_of_two_up(len);
		ctxt->l_table_size[w] = size;
		t = ctxt->l_table[w] = (Match_node **) xalloc(size * sizeof(Match_node *));
		memset(t, 0, size * sizeof(Match_node *));

		for (d = sent->word[w].d; d != NULL; d = d->next)
		{
			if (d->left != NULL)
			{
				put_into_match_table(size, t, d, d->left, -1);
			}
		}

		len = right_disjunct_list_length(sent->word[w].d);
		size = next_power_of_two_up(len);
		ctxt->r_table_size[w] = size;
		t = ctxt->r_table[w] = (Match_node **) xalloc(size * sizeof(Match_node *));
		memset(t, 0, size * sizeof(Match_node *));

		for (d = sent->word[w].d; d != NULL; d = d->next)
		{
			if (d->right != NULL)
			{
				put_into_match_table(size, t, d, d->right, 1);
			}
		}
	}

	return ctxt;
}

/**
 * Forms and returns a list of disjuncts coming from word w, that
 * actually matches lc or rc or both. The lw and rw are the words from
 * which lc and rc came respectively.
 *
 * The list is returned in a linked list of Match_nodes.  This list
 * contains no duplicates, because when processing the ml list, only
 * elements whose match_left is true are included, and such elements are
 * not included again when processing the mr list.
 *
 * Note that if both lc and rc match the corresponding connectors of w,
 * match_left is set to true when the ml list is processed and the
 * disjunct is then added to the result list, and match_right of the
 * same disjunct is set to true when the mr list is processed, and this
 * disjunct is not added again.
 */
Match_node *
form_match_list(fast_matcher_t *ctxt, int w,
                Connector *lc, int lw,
                Connector *rc, int rw)
{
	Match_node *mx, *my, *mr_end, *front, **mxp;
	Match_node *ml = NULL, *mr = NULL;

	/* Get the lists of candidate matching disjuncts of word w for lc and
	 * rc.  Consider each of these lists only if the length_limit of lc
	 * rc and also w, is not greater then the distance between their word
	 * and the word w. */
	if ((lc != NULL) && ((w - lw) <= lc->length_limit))
	{
		mxp = get_match_table_entry(ctxt->l_table_size[w], ctxt->l_table[w], lc, -1);
		if ((NULL != mxp) && (NULL != *mxp) &&
		    ((w - lw) <= (*mxp)->d->left->length_limit))
		{
			ml = *mxp;
		}
	}
	if ((rc != NULL) && ((rw - w) <= rc->length_limit))
	{
		mxp = get_match_table_entry(ctxt->r_table_size[w], ctxt->r_table[w], rc, 1);
		if ((NULL != mxp) && (NULL != *mxp) &&
		    ((rw - w) <= (*mxp)->d->right->length_limit))
		{
			mr = *mxp;
		}
	}

	for (mx = mr; mx != NULL; mx = mx->next)
	{
		if (mx->d->right->word > rw) break;
		mx->d->match_left = false;
	}
	mr_end = mx;

	front = NULL;
	/* Construct the list of things that could match the left. */
	for (mx = ml; mx != NULL; mx = mx->next)
	{
		if (mx->d->left->word < lw) break;

		mx->d->match_left = do_match(lc, mx->d->left, lw, w);
		if (!mx->d->match_left) continue;
		mx->d->match_right = false;

		my = get_match_node(ctxt);
		my->d = mx->d;
		my->next = front;
		front = my;
	}

	/* Append the list of things that could match the right.
	 * Note that it is important to set here match_right correctly even
	 * if we are going to skip this element here because its match_left
	 * is true, since then it means it is already included in the match
	 * list. */
	for (mx = mr; mx != mr_end; mx = mx->next)
	{
		mx->d->match_right = do_match(mx->d->right, rc, w, rw);
		if (!mx->d->match_right || mx->d->match_left) continue;

		my = get_match_node(ctxt);
		my->d = mx->d;
		my->next = front;
		front = my;
	}

	return front;
}

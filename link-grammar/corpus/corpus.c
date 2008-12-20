/*
 * corpus.c
 *
 * Data for corpus statistics, used to provide a parse ranking
 * to drive the SAT solver, as well as parse ranking with the
 * ordinary solver. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "corpus.h"
#include "../api-structures.h"
#include "../utilities.h"

typedef struct 
{
	double score;
	int found;
} DJscore;

Corpus * lg_corpus_new(void)
{
	char * dbname;
	int rc;

	Corpus *c = (Corpus *) malloc(sizeof(Corpus));
	c->errmsg = NULL;

	dbname = "/home/linas/src/novamente/src/link-grammar/trunk/data/sql/disjuncts.db";

	rc = sqlite3_open(dbname, &c->dbconn);
	if (rc)
	{
		prt_error("Warning: Can't open database: %s\n", sqlite3_errmsg(c->dbconn));
		sqlite3_close(c->dbconn);
		c->dbconn = NULL;
	}

	return c;
}

void lg_corpus_delete(Corpus *c)
{
	if (c->dbconn)
	{
		sqlite3_close(c->dbconn);
		c->dbconn = NULL;
	}
	free(c);
}

/* ========================================================= */

/* LOW_SCORE is what is assumed if a disjunct-word pair is not found
 * in the dictionary. It is meant to be -log_2(prob(d|w)) where
 * prob(d|w) is the conditional probability of seeing the disjunct d
 * given the word w. A value of 17 is about equal to 1 in 100,000.
 */
#define LOW_SCORE 17.0

static int prob_cb(void *user_data, int argc, char **argv, char **colname)
{
	DJscore *score = (DJscore *) user_data;

	if (1 < argc)
	{
		prt_error("Error: sql table entry not unique\n");
	}
	score->found = 1;
	score->score = atof(argv[0]);
	return 0;
}

double lg_corpus_score(Corpus *corp, Sentence sent, Linkage_info *li)
{
	char djstr[MAX_TOKEN_LENGTH*20]; /* no word will have more than 20 links */
	char querystr[MAX_TOKEN_LENGTH*25];
	int i, w;
	int nwords = sent->length;
	Parse_info pi = sent->parse_info;
	int nlinks = pi->N_links;
	int *djlist, *djloco, *djcount;
	DJscore score;
	int rc;
	char *errmsg;
	double tot_score = 0.0;

	djcount = (int *) malloc (sizeof(int) * (nwords + 2*nwords*nlinks));
	djlist = djcount + nwords;
	djloco = djlist + nwords*nlinks;

	for (w=0; w<nwords; w++)
	{
		djcount[w] = 0;
	}

	/* Create a table of disjuncts for each word. */
	for (i=0; i<nlinks; i++)
	{
		int lword = pi->link_array[i].l;
		int rword = pi->link_array[i].r;
		int slot = djcount[lword];
		djlist[lword*nlinks + slot] = i;
      djloco[lword*nlinks + slot] = rword;
		djcount[lword] ++;

		slot = djcount[rword];
		djlist[rword*nlinks + slot] = i;
      djloco[rword*nlinks + slot] = lword;
		djcount[rword] ++;

#ifdef DEBUG
		printf("Link: %d is %s--%s--%s\n", i, 
			sent->word[lword].string, pi->link_array[i].name,
			sent->word[rword].string);
#endif
	}

	/* Sort the table of disjuncts, left to right */
	for (w=0; w<nwords; w++)
	{
		/* Sort the disjuncts for this word. -- bubble sort */
		int slot = djcount[w];
		for (i=0; i<slot; i++)
		{
			int j;
			for (j=i+1; j<slot; j++)
			{
				if (djloco[w*nlinks + i] > djloco[w*nlinks + j])
				{
					int tmp = djloco[w*nlinks + i];
					djloco[w*nlinks + i] = djloco[w*nlinks + j];
					djloco[w*nlinks + j] = tmp;
					tmp = djlist[w*nlinks + i];
					djlist[w*nlinks + i] = djlist[w*nlinks + j];
					djlist[w*nlinks + j] = tmp;
				}
			}
		}

/* XXXX use strncat for saety ... */
		/* Create the disjunct string */
		djstr[0] = 0;
		for (i=0; i<slot; i++)
		{
			int dj = djlist[w*nlinks + i];
			strcat(djstr, pi->link_array[dj].name);
			if (djloco[w*nlinks + i] < w)
				strcat(djstr, "-");
			else
				strcat(djstr, "+");
			strcat(djstr, " ");
		}

		/* Look up the disjunct in the database */
		strcpy(querystr, 
			"SELECT log_cond_probability FROM Disjuncts WHERE inflected_word = '");
		strcat(querystr, sent->word[w].d->string);
		strcat(querystr, "' AND disjunct = '");
		strcat(querystr, djstr);
		strcat(querystr, "';");

		score.found = 0;
		rc = sqlite3_exec(corp->dbconn, querystr, prob_cb, &score, &errmsg);
		if (rc != SQLITE_OK)
		{
			prt_error("Error: SQLite: %s\n", errmsg);
			sqlite3_free(errmsg);
		}

		/* total up the score */
		if (score.found)
		{
			printf ("Word=%s dj=%s score=%f\n",
				sent->word[w].d->string, djstr, score.score);
			tot_score += score.score;
		}
		else
		{
			printf ("Word=%s dj=%s not found in dict, assume score=%f\n",
				sent->word[w].d->string, djstr, LOW_SCORE);
			tot_score += LOW_SCORE;
		}
	}
	return tot_score;
}


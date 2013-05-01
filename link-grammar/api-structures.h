/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software,    */
/* and also available at http://www.link.cs.cmu.edu/link/license.html    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/* This file is somewhat misnamed, as everything here defines the
 * link-private, internal-use-only "api", which is subject to change
 * from revision to revision. No external code should link to this
 * stuff.
 */

/*****************************************************************************
*
* NOTE: There are five basic "types" used within the link parser.  These are:
*
*       Dictionary, Parse_Options, Sentence, Linkage, PostProcessor
*
* To make the use of the API simpler, each of these is typedef'ed as a pointer
* to a data structure.  As a result, some of the code may look a little funny,
* since it uses pointers in a way that is syntactically inconsistent.  After
* working a bit with these basic types enough, this should not be confusing.
*
******************************************************************************/

#ifndef _API_STRUCTURESH_
#define _API_STRUCTURESH_

#include <wchar.h>
#include "api-types.h"
#include "structures.h" /* for definition of Link */
#include "corpus/corpus.h"
#include "error.h"

struct Cost_Model_s
{
	Cost_Model_type type;
	int (*compare_fn)(Linkage_info *, Linkage_info *);
};

struct Resources_s
{
	int    max_parse_time;  /* in seconds */
	size_t max_memory;      /* in bytes */
	double time_when_parse_started;
	size_t space_when_parse_started;
	double when_created;
	double when_last_called;
	double cumulative_time;
	int    memory_exhausted;
	int    timer_expired;
};

struct Parse_Options_s
{
	int verbosity;         /* Level of detail to give about the computation 0 */
	Boolean use_sat_solver;/* Use the Boolean SAT based parser */
	Boolean use_viterbi;   /* Use the Viterbi decoder-based parser */
	int linkage_limit;     /* The maximum number of linkages processed 100 */
	float disjunct_cost;   /* Max disjunct cost to allow */
	int min_null_count;    /* The minimum number of null links to allow */
	int max_null_count;    /* The maximum number of null links to allow */
	int null_block;        /* consecutive blocks of this many words are
	                          considered as one null link  (default=1) */
	Boolean islands_ok;    /* If TRUE, then linkages with islands
	                          (separate component of the link graph)
	                          will be generated (default=FALSE) */
	int twopass_length;    /* min length for two-pass post processing */
	int max_sentence_length;
	int short_length;      /* Links that are limited in length can be
	                        * no longer than this.  Default = 6 */
	Boolean all_short;     /* If true, there can be no connectors that are exempt */
	Boolean use_spell_guess; /* Perform spell-guessing of unknown words. */ 
	Cost_Model cost_model; /* For sorting linkages in post_processing */
	Resources resources;   /* For deciding when to abort the parsing */

	/* Flags governing the command-line client; not used by parser */
	Boolean display_short;
	Boolean display_word_subscripts;  /* as in "dog.n" as opposed to "dog" */
	Boolean display_link_subscripts;  /* as in "Ss" as opposed to "S" */
	Boolean display_walls;
#ifdef USE_FAT_LINKAGES
	Boolean use_fat_links;     /* Look for fat linkages */
	Boolean display_union;     /* print squashed version of linkage with conjunction? */
#endif /* USE_FAT_LINKAGES */
	Boolean allow_null;        /* true if we allow null links in parsing */
	Boolean use_cluster_disjuncts;    /* if true, atttempt using a borader list of disjuncts */
	Boolean echo_on;           /* true if we should echo the input sentence */
	Boolean batch_mode;        /* if true, process sentences non-interactively */
	Boolean panic_mode;        /* if true, parse in "panic mode" after all else fails */
	int screen_width;          /* width of screen for displaying linkages */
	Boolean display_on;        /* if true, output graphical linkage diagram */
	Boolean display_postscript;    /* if true, output postscript linkage */
	ConstituentDisplayStyle display_constituents;     /* style for displaying constituent structure */
	Boolean display_bad;       /* if true, bad linkages are displayed */
	Boolean display_disjuncts; /* if true, print disjuncts that were used */
	Boolean display_links;     /* if true, a list o' links is printed out */
	Boolean display_senses;    /* if true, sense candidates are printed out */
};

struct Connector_set_s
{
	Connector ** hash_table;
	int          table_size;
	int          is_defined;  /* if 0 then there is no such set */
};

struct Dictionary_s
{
	Dict_node *     root;
	Regex_node *    regex_root;
	const char *    name;
	const char *    lang;

	Boolean         use_unknown_word;
	Boolean         unknown_word_defined;
	Boolean         left_wall_defined;
	Boolean         right_wall_defined;
	Boolean         have_empty_suffix;

	/* Affixes are used during the tokenization stage. */
	Dictionary      affix_table;
	int r_strippable; /* right */
	int l_strippable; /* left */
	int u_strippable; /* units on left */
	int s_strippable; /* generic suffix */
	int p_strippable; /* generic prefix */
	const char ** strip_left;
	const char ** strip_right;
	const char ** strip_units;
	const char ** prefix;
	const char ** suffix;

	/* If not null, then use spelling guesser for unknown words */
	void *          spell_checker; /* spell checker handle */
#if USE_CORPUS
	Corpus *        corpus; /* Statistics database */
#endif

	Postprocessor * postprocessor;
	Postprocessor * constituent_pp;
	Connector_set * andable_connector_set;  /* NULL=everything is andable */
	Connector_set * unlimited_connector_set; /* NULL=everthing is unlimited */
	int             max_cost;
	String_set *    string_set;  /* Set of link names constructed during parsing */
	int             num_entries;
	Word_file *     word_file_header;

	/* exp_list links together all the Exp structs that are allocated
	 * in reading this dictionary.  Needed for freeing the dictionary
	 */
	Exp *           exp_list;

	/* Private data elements that come in play only while the
	 * dictionary is being read, and are not otherwise used.
	 */
	const wchar_t * input;
	const wchar_t * pin;
	char            token[MAX_TOKEN_LENGTH];
	Boolean         recursive_error;
	Boolean         is_special;
	wint_t          already_got_it;
	int             line_number;
	mbstate_t       mbss; /* multi-byte shift state */
};

struct Label_node_s
{
	int          label;
	Label_node * next;
};

#define HT_SIZE (1<<10)

struct And_data_s
{
	int          LT_bound;
	int          LT_size;
	Disjunct **  label_table;
	Label_node * hash_table[HT_SIZE];

	/* keeping statistics */
	int STAT_N_disjuncts;
	int STAT_calls_to_equality_test;
};

struct Parse_info_struct
{
	int            x_table_size;
	int            log2_x_table_size;
	X_table_connector ** x_table;
	Parse_set *    parse_set;
	int            N_words;
	Disjunct **    chosen_disjuncts;
	int            N_links;
	Link          link_array[MAX_LINKS];

	/* Points to the image structure for each word.
	 * NULL if not a fat word. */
	Image_node ** image_array;

#ifdef USE_FAT_LINKAGES
	/* Array of boolean flags, one per word. Set to TRUE if this 
	 * word has a fat down link. FALSE otherise */
	Boolean *has_fat_down;
#endif /* USE_FAT_LINKAGES */

	/* thread-safe random number state */
	unsigned int rand_state;
};

struct Sentence_s
{
	Dictionary  dict;           /* words are defined from this dictionary */
	const char *orig_sentence;  /* Copy of original sentence */
	int    length;              /* number of words */
	Word  *word;                /* array of words after tokenization */
	String_set *   string_set;  /* used for word names, not connectors */

	/* Parse results */
	int    num_linkages_found;  /* total number before postprocessing.  This
	                               is returned by the count() function */
	int    num_linkages_alloced;/* total number of linkages allocated.
	                               the number post-processed might be fewer
	                               because some are non-canonical */
	int    num_linkages_post_processed;
	                            /* The number of linkages that are actually
	                               put into the array that was alloced.
	                               This is not the same as num alloced
	                               because some may be non-canonical. */
	int    num_valid_linkages;  /* number with no pp violations */
	int    num_thin_linkages;   /* valid linkages which are not fat */
	int    null_links;          /* null links allowed */
	int    null_count;          /* number of null links in linkages */
	Parse_info     parse_info;  /* set of parses for the sentence */
	Linkage_info * link_info;   /* array of valid and invalid linkages (sorted) */

	/* Tokenizer internal/private state */
	Boolean       *post_quote;
	Tokenizer     tokenizer;

	/* parser internal/private state */
	analyze_context_t * analyze_ctxt; /* private state  used for analyzing */
	count_context_t * count_ctxt; /* private state info used for counting */
	match_context_t * match_ctxt; /* private state info used for matching */

#ifdef USE_FAT_LINKAGES
	/* Obsolete fat-linkage state */
	char * is_conjunction;      /* Array of flags, one per word; set to
	                               TRUE if conjunction, as defined by dictionary */
	char** deletable;           /* deletable regions in a sentence with conjunction */
	char** dptr;                /* private pointer for mem management only */
	char** effective_dist;
	And_data       and_data;    /* used to keep track of fat disjuncts */
#endif /* USE_FAT_LINKAGES */

	/* Post-processor priavte/internal state */
	Boolean  q_pruned_rules;    /* don't prune rules more than once in p.p. */

	/* thread-safe random number state */
	unsigned int rand_state;

#ifdef USE_SAT_SOLVER
	/* Hook for the SAT solver */
	void *hook;
#endif /* USE_SAT_SOLVER */
};

/*********************************************************
 *
 * Post processing
 * XXX FIXME: most of these structures should not be in the
 * public API; they're here only because they're tangled into
 * sublinkages, which will go away when fat-links are removed.
 **********************************************************/

struct Domain_s
{
	const char *   string;
	List_o_links * lol;
	DTreeLeaf *    child;
	Domain *       parent;
	int            size;
	int            start_link;  /* the link that started this domain */
	char           type;        /* one letter name */
};


struct DTreeLeaf_s
{
	Domain *    parent;
	DTreeLeaf * next;
	int         link;
};

struct PP_data_s
{
	int N_domains;
	List_o_links * word_links[MAX_SENTENCE];
	List_o_links * links_to_ignore;
	Domain domain_array[MAX_LINKS]; /* the domains, sorted by size */
	int length;                     /* length of current sentence */
};

struct PP_info_s
{
	int       num_domains;
	const char **   domain_name;
};

struct Postprocessor_s
{
	pp_knowledge *knowledge;             /* internal rep'n of the actual rules */
	int n_global_rules_firing;           /* this & the next are diagnostic     */
	int n_local_rules_firing;
	pp_linkset *set_of_links_of_sentence;     /* seen in *any* linkage of sent */
	pp_linkset *set_of_links_in_an_active_rule;/*used in *some* linkage of sent*/
	int *relevant_contains_one_rules;        /* -1-terminated list of indices  */
	int *relevant_contains_none_rules;
	/* the following maintain state during a call to post_process() */
	String_set *sentence_link_name_set;        /* link names seen for sentence */
	int visited[MAX_SENTENCE];                   /* for the depth-first search */
	PP_node *pp_node;
	PP_data pp_data;
};


/*********************************************************
 *
 * Linkages
 *
 **********************************************************/

/* XXX FIXME When fat links are removed, therre will only ever be
 * just one sublinkage, and so this struct can be merged into 
 * Linkage_s below, and removed from the API.
 */
struct Sublinkage_s
{
	int       num_links;          /* Number of links in array */
	Link **   link;               /* Array of links */
	PP_info * pp_info;            /* PP info for each link */
	const char * violation;       /* Name of violation, if any */
	PP_data   pp_data;
};

typedef struct DIS_node_struct DIS_node;

struct Linkage_s
{
	int             num_words;  /* number of (tokenized) words */
	const char *  * word;       /* array of word spellings */
	Linkage_info*   info;       /* index and cost information */
	int             num_sublinkages; /* One for thin linkages, bigger for fat */
	int             current;    /* Allows user to select particular sublinkage */
	Sublinkage *    sublinkage; /* A parse with conjunctions will have several */
	int             unionized;  /* if TRUE, union of links has been computed */
	Sentence        sent;
	Parse_Options   opts;
	DIS_node      * dis_con_tree; /* Disjunction-conjunction tree */
};



#endif


/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* Copyright 2008, 2009, 2012 Linas Vepstas <linasvepstas@gmail.com>     */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "api-structures.h"
#include "dict-api.h"
#include "dict-common.h"
#include "externs.h"
#include "read-dict.h"
#include "read-regex.h"
#include "regex-morph.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "utilities.h"
#include "word-utils.h"
#include "dict-sql/read-sql.h"  /* Temporary hack */


/***************************************************************
*
* Routines for manipulating Dictionary
*
****************************************************************/

/* Units will typically have a ".u" at the end. Get
 * rid of it, as otherwise stripping is messed up. */
static inline char * deinflect(const char * str)
{
	size_t len;
	char *s;
	char *p = strrchr(str, SUBSCRIPT_MARK);
	if (!p || (p == str)) return strdup(str);

	len = p - str;
	s = (char *)malloc(len + 1);
	strncpy(s, str, len);
	s[len] = '\0';
	return s;
}


/**
 * Call function "func" on each dictionary node in the dictionary.
 */
static void iterate_on_dictionary(Dictionary dict, Dict_node *root,
                                  void (*func)(Dictionary, Dict_node*))
{
	if (root == NULL) return;
	(*func)(dict, root);

	iterate_on_dictionary(dict, root->left, func);
	iterate_on_dictionary(dict, root->right, func);
}

static const char * rpunc_con = "RPUNC";
static const char * lpunc_con = "LPUNC";
static const char * units_con = "UNITS";

/* SUF is used in the Russian dict; PRE is not used anywhere, yet ... */
static const char * suf_con = "SUF";
static const char * pre_con = "PRE";
static const char * mpre_con = "MPRE"; /* multi-prefix, currently for Hebrew */
static const char * sane_morphism_con = "SANEMORPHISM"; /* regexp for sane_morphism() */

static void count_affix(Dictionary dict, Dict_node *dn)
{
	if (word_has_connector(dn, rpunc_con, '+')) dict->r_strippable++;
	if (word_has_connector(dn, lpunc_con, '+')) dict->l_strippable++;
	if (word_has_connector(dn, units_con, '+')) dict->u_strippable++;
	if (word_has_connector(dn, suf_con, '+')) dict->s_strippable++;
	if (word_has_connector(dn, pre_con, '+')) dict->p_strippable++;
	if (word_has_connector(dn, mpre_con, '+')) dict->mp_strippable++;
	if (word_has_connector(dn, sane_morphism_con, '+')) dict->sm_total++;
}

static void load_affix(Dictionary dict, Dict_node *dn)
{
	if (word_has_connector(dn, rpunc_con, '+'))
	{
		assert(dict->r_stripped < dict->r_strippable, "bad r affix table size");
		dict->strip_right[dict->r_stripped] = deinflect(dn->string);
		dict->r_stripped++;
	}
	if (word_has_connector(dn, lpunc_con, '+'))
	{
		assert(dict->l_stripped < dict->l_strippable, "bad l affix table size");
		dict->strip_left[dict->l_stripped] = deinflect(dn->string);
		dict->l_stripped++;
	}
	if (word_has_connector(dn, units_con, '+'))
	{
		assert(dict->u_stripped < dict->u_strippable, "bad u affix table size");
		dict->strip_units[dict->u_stripped] = deinflect(dn->string);
		dict->u_stripped++;
	}
	if (word_has_connector(dn, suf_con, '+'))
	{
		assert(dict->s_stripped < dict->s_strippable, "bad s affix table size");
		dict->suffix[dict->s_stripped] = dn->string;
		dict->s_stripped++;
	}
	if (word_has_connector(dn, pre_con, '+'))
	{
		assert(dict->p_stripped < dict->p_strippable, "bad p affix table size");
		dict->prefix[dict->p_stripped] = dn->string;
		dict->p_stripped++;
	}
	if (word_has_connector(dn, mpre_con, '+'))
	{
		assert(dict->mp_stripped < dict->mp_strippable, "bad mp affix table size");
		dict->mprefix[dict->mp_stripped] = dn->string;
		dict->mp_stripped++;
	}
	if (word_has_connector(dn, sane_morphism_con, '+'))
	{
		assert(dict->sm_current_count < dict->sm_total, "bad mp affix table size");
		dict->sane_morphism[dict->sm_current_count] = dn->string;
		dict->sm_current_count++;
	}
}

static void affix_list_create(Dictionary dict)
{
	dict->strip_left = NULL;
	dict->strip_right = NULL;
	dict->strip_units = NULL;
	dict->prefix = NULL;
	dict->mprefix = NULL;
	dict->suffix = NULL;

	dict->r_strippable = 0;
	dict->l_strippable = 0;
	dict->u_strippable = 0;
	dict->p_strippable = 0;
	dict->mp_strippable = 0;
	dict->s_strippable = 0;
	dict->sm_total = 0;

	/* Count how many affixes of each type we have ... */
	iterate_on_dictionary(dict, dict->root, count_affix);

	dict->strip_right = (const char **) xalloc(dict->r_strippable * sizeof(char *));
	dict->strip_left = (const char **) xalloc(dict->l_strippable * sizeof(char *));
	dict->strip_units = (const char **) xalloc(dict->u_strippable * sizeof(char *));
	dict->suffix = (const char **) xalloc(dict->s_strippable * sizeof(char *));
	dict->prefix = (const char **) xalloc(dict->p_strippable * sizeof(char *));
	dict->mprefix = (const char **) xalloc(dict->mp_strippable * sizeof(char *));
	dict->sane_morphism = (const char **) xalloc(dict->sm_total * sizeof(char *));

	/* Load affixes from the affix table. */
	iterate_on_dictionary(dict, dict->root, load_affix);
}

static void affix_list_delete(Dictionary dict)
{
	int i;
	for (i=0; i<dict->l_strippable; i++)
	{
		free((char *)dict->strip_left[i]);
	}
	for (i=0; i<dict->r_strippable; i++)
	{
		free((char *)dict->strip_right[i]);
	}
	for (i=0; i<dict->u_strippable; i++)
	{
		free((char *)dict->strip_units[i]);
	}
	xfree(dict->strip_right, dict->r_strippable * sizeof(char *));
	xfree(dict->strip_left, dict->l_strippable * sizeof(char *));
	xfree(dict->strip_units, dict->u_strippable * sizeof(char *));

	/* The prefix and suffix words are in the string set,
	 * and are deleted here. */
	xfree(dict->suffix, dict->s_strippable * sizeof(char *));
	xfree(dict->prefix, dict->p_strippable * sizeof(char *));
	xfree(dict->mprefix, dict->mp_strippable * sizeof(char *));
	xfree(dict->sane_morphism, dict->sm_total * sizeof(char *));
}

static Dictionary
dictionary_six(const char * lang, const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name);
/**
 * Read dictionary entries from a wide-character string "input".
 * All other parts are read from files.
 */
static Dictionary
dictionary_six_str(const char * lang,
                const char * input,
                const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name)
{
	const char * t;
	Dictionary dict;
	Dict_node *dict_node;

	dict = (Dictionary) xalloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	dict->string_set = string_set_create();

	dict->lang = lang;
	t = strrchr (lang, '/');
	if (t) dict->lang = string_set_add(t+1, dict->string_set);
	dict->name = string_set_add(dict_name, dict->string_set);
	dict->version = NULL;

	dict->num_entries = 0;
	dict->is_special = FALSE;
	dict->already_got_it = '\0';
	dict->line_number = 1;
	dict->root = NULL;
	dict->word_file_header = NULL;
	dict->exp_list = NULL;
	dict->affix_table = NULL;
	dict->recursive_error = FALSE;

	/* To disable spell-checking, just set the cheker to NULL */
	dict->spell_checker = spellcheck_create(dict->lang);
#ifdef HAVE_SQLITE
	dict->db_handle = NULL;
#endif

	/* Read dictionary from the input string. */
	dict->input = input;
	dict->pin = dict->input;
	if (!read_dictionary(dict))
	{
		dict->pin = NULL;
		dict->input = NULL;
		goto failure;
	}
	dict->pin = NULL;
	dict->input = NULL;

	dict->affix_table = NULL;
	if (affix_name != NULL)
	{
		dict->affix_table = dictionary_six(lang, affix_name, NULL, NULL, NULL, NULL);
		if (dict->affix_table == NULL)
		{
			prt_error("Error: Could not open affix file %s", affix_name);
			goto failure;
		}
		affix_list_create(dict->affix_table);
	}

	dict->regex_root = NULL;
	if (regex_name != NULL)
	{
		int rc;
		rc = read_regex_file(dict, regex_name);
		if (rc) goto failure;
		rc = compile_regexs(dict);
		if (rc) goto failure;
	}

#ifdef USE_CORPUS
	dict->corpus = NULL;
	if (affix_name != NULL) /* Don't do this for the second time */
	{
		dict->corpus = lg_corpus_new();
	}
#endif

	dict->left_wall_defined  = boolean_dictionary_lookup(dict, LEFT_WALL_WORD);
	dict->right_wall_defined = boolean_dictionary_lookup(dict, RIGHT_WALL_WORD);

	dict->empty_word_defined = boolean_dictionary_lookup(dict, EMPTY_WORD_MARK);

	dict->postprocessor	  = post_process_open(pp_name);
	dict->constituent_pp	 = post_process_open(cons_name);

	dict->unknown_word_defined = boolean_dictionary_lookup(dict, UNKNOWN_WORD);
	dict->use_unknown_word = TRUE;

#ifdef USE_FAT_LINKAGES
	if ((dict_node = dictionary_lookup_list(dict, ANDABLE_CONNECTORS_WORD)) != NULL) {
		dict->andable_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->andable_connector_set = NULL;
	}
	free_lookup_list(dict_node);
#endif /* USE_FAT_LINKAGES */

	if ((dict_node = dictionary_lookup_list(dict, UNLIMITED_CONNECTORS_WORD)) != NULL) {
		dict->unlimited_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->unlimited_connector_set = NULL;
	}
	free_lookup_list(dict_node);

	return dict;

failure:
	string_set_delete(dict->string_set);
	xfree(dict, sizeof(struct Dictionary_s));
	return NULL;
}

/**
 * Use filenames of six different files to put together the dictionary.
 */
static Dictionary
dictionary_six(const char * lang, const char * dict_name,
                const char * pp_name, const char * cons_name,
                const char * affix_name, const char * regex_name)
{
	Dictionary dict;

	char* input = get_file_contents(dict_name);
	if (NULL == input)
	{
		prt_error("Error: Could not open dictionary %s", dict_name);
		return NULL;
	}

	dict = dictionary_six_str(lang, input, dict_name, pp_name,
	                          cons_name, affix_name, regex_name);

	free(input);
	return dict;
}

Dictionary dictionary_create_from_file(const char * lang)
{
	Dictionary dictionary;

	init_memusage();
	if (lang && *lang)
	{
		char * dict_name;
		char * pp_name;
		char * cons_name;
		char * affix_name;
		char * regex_name;

		dict_name = join_path(lang, "4.0.dict");
		pp_name = join_path(lang, "4.0.knowledge");
		cons_name = join_path(lang, "4.0.constituent-knowledge");
		affix_name = join_path(lang, "4.0.affix");
		regex_name = join_path(lang, "4.0.regex");

		dictionary = dictionary_six(lang, dict_name, pp_name, cons_name,
		                             affix_name, regex_name);

		free(regex_name);
		free(affix_name);
		free(cons_name);
		free(pp_name);
		free(dict_name);
	}
	else
	{
		prt_error("Error: No language specified!");
		dictionary = NULL;
	}

	return dictionary;
}


/**
 * Use "string" as the input dictionary. All of the other parts,
 * including post-processing, affix table, etc, are NULL.
 * This routine is itended for unit-testing ONLY.
 */
Dictionary dictionary_create_from_utf8(const char * input)
{
	Dictionary dictionary = NULL;
	char * lang;

	init_memusage();

	lang = get_default_locale();
	if (lang && *lang) {
		dictionary = dictionary_six_str(lang, input, "string",
		                                NULL, NULL, NULL, NULL);
		free(lang);
	} else {
		/* Default to en when locales are broken (e.g. WIN32) */
		dictionary = dictionary_six_str("en", input, "string",
		                                NULL, NULL, NULL, NULL);
	}

	return dictionary;
}

int dictionary_delete(Dictionary dict)
{
	if (!dict) return 0;

	if (verbosity > 0) {
		prt_error("Info: Freeing dictionary %s", dict->name);
	}

#ifdef USE_CORPUS
	lg_corpus_delete(dict->corpus);
#endif

	if (dict->affix_table != NULL) {
		affix_list_delete(dict->affix_table);
		dictionary_delete(dict->affix_table);
	}
	spellcheck_destroy(dict->spell_checker);

#ifdef USE_FAT_LINKAGES
	connector_set_delete(dict->andable_connector_set);
#endif /* USE_FAT_LINKAGES */
	connector_set_delete(dict->unlimited_connector_set);

	post_process_close(dict->postprocessor);
	post_process_close(dict->constituent_pp);
	string_set_delete(dict->string_set);
	free_regexs(dict);
	free_dictionary(dict);
	xfree(dict, sizeof(struct Dictionary_s));

	return 0;
}

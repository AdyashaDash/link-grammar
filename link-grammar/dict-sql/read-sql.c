
/*
 * read-sql.c
 *
 * Read in dictionary from an SQL DB.
 * Keeping it simple for just right now, and using SQLite.
 *
 * The goal of reading the dictioary from SQL is to enable some 
 * other process (machine-learning algo) to dynamically update
 * the dictionary.
 *
 * Copyright (c) 2014 Linas Vepstas <linasvepstas@gmail.com>
 */

#ifdef HAVE_SQLITE

#include <sqlite3.h>

#include "api-structures.h"
#include "dict-api.h"
#include "dict-common.h"
#include "dict-structures.h"
#include "spellcheck.h"
#include "string-set.h"
#include "structures.h"
#include "utilities.h"
#include "word-utils.h"

#include "read-sql.h"

/*


static int morph_cb(void *user_data, int argc, char **argv, char **colName)
{
	bigstr* bs = user_data;

	int i;
	for (i=0; i<argc; i++)
	{
printf("duude %s = %s\n", colName[i], argv[i] ? argv[i] : "NULL");
	}
printf("\n");

	return 0;
}
*/

/* ========================================================= */
/* Dictionary word lookup proceedures. */

typedef struct 
{
	Dictionary dict;
	Dict_node *dn;
} cbdata;


static int morph_cb(void *user_data, int argc, char **argv, char **colName)
{
	cbdata* bs = user_data;
	Dict_node *dn;
	char *word;

	assert(3 == argc, "Bad column count");
	assert(argv[0], "NULL column value");
	word = argv[0];

	/* Put each word into a Dict_node. */
	dn = (Dict_node *) xalloc(sizeof(Dict_node));
	patch_subscript(word);
	dn->string = string_set_add(word, bs->dict->string_set);
	dn->left = bs->dn;
	bs->dn = dn;

	return 0;
}

Dict_node * dictionary_db_lookup_list(Dictionary dict, const char *s)
{
	sqlite3 *db = dict->db_handle;
	cbdata bs;
	dyn_str *qry;
	int rc;

printf("duude look %s\n", s);

	/* The token to look up is called the 'morpheme'. */
	qry = dyn_str_new();
	dyn_strcat(qry, "SELECT * FROM Morphemes WHERE morpheme = \'");
	dyn_strcat(qry, s);
	dyn_strcat(qry, "\';");

	bs.dict = dict;
	bs.dn = NULL;

	rc = sqlite3_exec(db, qry->str, morph_cb, &bs, NULL);
	if (SQLITE_OK != rc)
	{
		/* Normal exit, if the word is not in the dict. */
		dyn_str_delete(qry);
		return NULL;
	}
	dyn_str_delete(qry);

	return bs.dn;
}

/* ========================================================= */
/* Dictionary creation, setup, open proceedures */

Boolean check_db(const char *lang)
{
	char *dbname = join_path (lang, "dict.db");
	Boolean retval = file_exists(dbname);
	free(dbname);
	return retval;
}

static void db_setup(Dictionary dict)
{
	sqlite3 *db;
	int rc;

	/* Open the database */
	rc = sqlite3_open(dict->name, &db);
	if (rc)
	{
		prt_error("Error: Can't open %s database: %s\n",
			dict->name, sqlite3_errmsg(db));
		sqlite3_close(db);
		return;
	}

	dict->db_handle = db;
}

Dictionary dictionary_create_from_db(const char *lang)
{
	char *dbname;
	const char * t;
	Dictionary dict;
	Dict_node *dict_node;

	dict = (Dictionary) xalloc(sizeof(struct Dictionary_s));
	memset(dict, 0, sizeof(struct Dictionary_s));

	dict->version = NULL;
	dict->num_entries = 0;
	dict->affix_table = NULL;
	dict->regex_root = NULL;

	/* Language and file-name stuff */
	dict->string_set = string_set_create();
	dict->lang = lang;
	t = strrchr (lang, '/');
	if (t) dict->lang = string_set_add(t+1, dict->string_set);

	/* To disable spell-checking, just set the checker to NULL */
	dict->spell_checker = spellcheck_create(dict->lang);
	dict->postprocessor	 = NULL;
	dict->constituent_pp  = NULL;

	dbname = join_path (lang, "dict.db");
	dict->name = string_set_add(dbname, dict->string_set);
	free(dbname);

	/* Set up the database */
	db_setup(dict);

	/* Misc remaining common (generic) dict setup work */
	dict->left_wall_defined  = boolean_dictionary_lookup(dict, LEFT_WALL_WORD);
	dict->right_wall_defined = boolean_dictionary_lookup(dict, RIGHT_WALL_WORD);

	dict->empty_word_defined = boolean_dictionary_lookup(dict, EMPTY_WORD_MARK);

	dict->unknown_word_defined = boolean_dictionary_lookup(dict, UNKNOWN_WORD);
	dict->use_unknown_word = TRUE;

	dict_node = dictionary_lookup_list(dict, UNLIMITED_CONNECTORS_WORD);
	if (dict_node != NULL) {
		dict->unlimited_connector_set = connector_set_create(dict_node->exp);
	} else {
		dict->unlimited_connector_set = NULL;
	}
	free_lookup_list(dict_node);

	return dict;
}

void dictionary_db_close(Dictionary dict)
{
	sqlite3 *db = dict->db_handle;
	if (db)
		sqlite3_close(db);

	dict->db_handle = NULL;
}

#endif /* HAVE_SQLITE */

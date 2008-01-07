/*
 * Java JNI interfaces
 */

#include <jni.h>
#include <stdio.h>

#include "link-includes.h"
#include "command-line.h"
#include "jni-client.h"

static Dictionary    dict;
static Parse_Options opts, panic_parse_opts;
static Sentence      sent=NULL;
static Linkage       linkage=NULL;
static char *        diagram;
//CNode*        tree;
static int           num_linkages, cur_linkage;

static void setup_panic_parse_options(Parse_Options opts)
{
	parse_options_set_disjunct_cost(opts, 3);
	parse_options_set_min_null_count(opts, 1);
	parse_options_set_max_null_count(opts, MAX_SENTENCE);
	parse_options_set_max_parse_time(opts, 60);
	parse_options_set_islands_ok(opts, 1);
	parse_options_set_short_length(opts, 6);
	parse_options_set_all_short_connectors(opts, 1);
	parse_options_set_linkage_limit(opts, 100);
	parse_options_set_verbosity(opts,0);
}

static void test(void)
{
	//printf("%d\n",word_contains("said",PAST_TENSE_FORM_MARKER,dict));
	//printf("%d\n",word_contains("gave.v",PAST_TENSE_FORM_MARKER,dict));
	//printf("%d\n",word_contains("have",PAST_TENSE_FORM_MARKER,dict));
	//printf("%d\n",word_contains("had",PAST_TENSE_FORM_MARKER,dict));
}

static void init(char* linkparser_dir)
{
	panic_parse_opts = parse_options_create();
	opts	= parse_options_create();
	setup_panic_parse_options(panic_parse_opts);
	parse_options_set_disjunct_cost(opts, 3);
	parse_options_set_max_sentence_length(opts, 70);
	parse_options_set_panic_mode(opts, TRUE);
	parse_options_set_max_parse_time(opts, 30);
	parse_options_set_linkage_limit(opts, 1000);
	parse_options_set_short_length(opts, 10);
	parse_options_set_verbosity(opts,0);

	char* dictName = "4.0.dict";
	char* knowName = "4.0.knowledge";
	char* consName = "4.0.constituent-knowledge";
	char* affiName = "4.0.affix";
	dict	= dictionary_create2(dictName, knowName, consName, affiName, linkparser_dir);
	test();
}

static void r_printTree(CNode* cn, int level)
{
	int i;
	CNode* c;

	if (cn==NULL) return;

	//Print Spacer
	for(i=0;i<level;i++) {
		printf("	");
	}

	//print label
	if (cn->label != NULL) {
		printf("%s\n",cn->label);
	} else {
		printf("NULL\n");
	}

	//recurse on children
	for(c = cn->child; c!=NULL; c=c->next) {
		r_printTree(c,level+1);
	}
}

static void printTree(CNode* cn)
{
	r_printTree(cn,0);
	printf("\n");
}

static void jParse(char* inputString)
{
	sent = sentence_create(inputString, dict);
	num_linkages=0;

	if (sent == NULL) {
		if (verbosity > 0) fprintf(stderr, "%s\n", lperrmsg);
		if (lperrno != NOTINDICT) exit(-1);
		else return;
	}

	if (sentence_length(sent) > parse_options_get_max_sentence_length(opts)) {
		sentence_delete(sent);
		if (verbosity > 0) {
			fprintf(stdout,
				"Sentence length (%d words) exceeds maximum allowable (%d words)\n",
				sentence_length(sent), parse_options_get_max_sentence_length(opts));
		}
		return;
	}

	/* First parse with cost 0 or 1 and no null links */
	parse_options_set_disjunct_cost(opts, 2);
	parse_options_set_min_null_count(opts, 0);
	parse_options_set_max_null_count(opts, 0);
	parse_options_reset_resources(opts);

	num_linkages = sentence_parse(sent, opts);

	/* Now parse with null links */
	if ((num_linkages == 0) && (!parse_options_get_batch_mode(opts))) {
		if (verbosity > 0) fprintf(stdout, "No complete linkages found.\n");
		if (parse_options_get_allow_null(opts)) {
			parse_options_set_min_null_count(opts, 1);
			parse_options_set_max_null_count(opts, sentence_length(sent));
			num_linkages = sentence_parse(sent, opts);
		}
	}

	if (parse_options_timer_expired(opts)) {
		if (verbosity > 0) fprintf(stdout, "Timer is expired!\n");
	}
	if (parse_options_memory_exhausted(opts)) {
		if (verbosity > 0) fprintf(stdout, "Memory is exhausted!\n");
	}

	if ((num_linkages == 0) &&
			parse_options_resources_exhausted(opts) &&
			parse_options_get_panic_mode(opts)) {
		print_total_time(opts);
		if (verbosity > 0) fprintf(stdout, "Entering \"panic\" mode...\n");
		parse_options_reset_resources(panic_parse_opts);
		parse_options_set_verbosity(panic_parse_opts, verbosity);
		num_linkages = sentence_parse(sent, panic_parse_opts);
		if (parse_options_timer_expired(panic_parse_opts)) {
			if (verbosity > 0) fprintf(stdout, "Timer is expired!\n");
		}
	}
}

static void makeLinkage(int i)
{
	if (i<num_linkages) {
		linkage = linkage_create(i,sent,opts);
		linkage_compute_union(linkage);
		linkage_set_current_sublinkage(linkage,linkage_get_num_sublinkages(linkage)-1);
		//	tree = linkage_constituent_tree(linkage);
	}
}

static void finish(void)
{
	if (sent!=NULL)
		sentence_delete(sent);
	//	linkage_free_constituent_tree(tree);
	if (linkage!=NULL)
		linkage_delete(linkage);
	dictionary_delete(dict);
	parse_options_delete(opts);
}

/* =========================================================================== */
/* Java JNI wrappers */

JNIEXPORT void JNICALL
Java_relex_parser_LinkParserJNIClient_cSetMaxParseSeconds(JNIEnv *env, jclass cls, jint maxParseSeconds)
{
	parse_options_set_max_parse_time(opts, maxParseSeconds);
}

JNIEXPORT void JNICALL
Java_relex_parser_LinkParserJNIClient_cSetMaxCost(JNIEnv *env, jclass cls, jint maxCost)
{
	parse_options_set_disjunct_cost(opts, maxCost);
}

/* Inaccessible static: singleton */
/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_relex_parser_LinkParserJNIClient_cInit(JNIEnv *env, jclass cls, jstring str) {
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	char* tmp = strdup(cStr);
	init(tmp);
	free(tmp);
	(*env)->ReleaseStringUTFChars(env,str,cStr);

}

///
/* Inaccessible static: singleton */
/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_relex_parser_LinkParserJNIClient_cTest(JNIEnv *env, jclass cls)
{
	if (verbosity > 0)
		 printf("\n\n***************JNI WORKING**************\n\n");
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cParse
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_relex_parser_LinkParserJNIClient_cParse(JNIEnv *env, jclass cls, jstring str)
{
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	char* tmp = strdup(cStr);
	jParse(tmp);
	free(tmp);
	(*env)->ReleaseStringUTFChars(env,str,cStr);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_relex_parser_LinkParserJNIClient_cClose(JNIEnv *env, jclass cls)
{
	finish();
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cNumWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cNumWords(JNIEnv *env, jclass cls)
{
	return linkage_get_num_words(linkage);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cGetWord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_relex_parser_LinkParserJNIClient_cGetWord(JNIEnv *env, jclass cls, jint i)
{
	char* w = sentence_get_word(sent, i);
	jstring j = (*env)->NewStringUTF(env, w);
	//	delete[] w;
	return j;
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cNumSkippedWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cNumSkippedWords(JNIEnv *env, jclass cls)
{
	return sentence_null_count(sent);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cNumLinkages
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cNumLinkages(JNIEnv *env, jclass cls)
{
	return sentence_num_valid_linkages(sent);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cMakeLinkage
 * Signature: (I)I
 */
JNIEXPORT void JNICALL
Java_relex_parser_LinkParserJNIClient_cMakeLinkage(JNIEnv *env, jclass cls, jint i)
{
	cur_linkage = i;
	makeLinkage(cur_linkage);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cLinkageNumViolations
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkageNumViolations(JNIEnv *env, jclass cls)
{
	return sentence_num_violations(sent, cur_linkage);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cLinkageDisjunctCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkageDisjunctCost(JNIEnv *env, jclass cls)
{
	return sentence_disjunct_cost(sent, cur_linkage);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cNumLinks
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cNumLinks(JNIEnv *env, jclass cls)
{
	return linkage_get_num_links(linkage);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cLinkLWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkLWord(JNIEnv *env, jclass cls, jint i)
{
	return linkage_get_link_lword(linkage, i);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cLinkRWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkRWord(JNIEnv *env, jclass cls, jint i)
{
		return linkage_get_link_rword(linkage, i);
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cLinkLLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkLLabel(JNIEnv *env, jclass cls, jint i)
{
		char *s = linkage_get_link_llabel(linkage, i);
		jstring j = (*env)->NewStringUTF(env, s);
		//	delete[] s;
		return j;
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cLinkRLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkRLabel(JNIEnv *env, jclass cls, jint i)
{
		char *s = linkage_get_link_rlabel(linkage, i);
		jstring j = (*env)->NewStringUTF(env, s);
		//	delete[] s;
		return j;
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cLinkLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkLabel(JNIEnv *env, jclass cls, jint i)
{
	char *s = linkage_get_link_label(linkage, i);
	jstring j = (*env)->NewStringUTF(env, s);
	//	delete[] s;
	return j;
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cConstituentString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_relex_parser_LinkParserJNIClient_cConstituentString(JNIEnv *env, jclass cls)
{
	char *s = linkage_print_constituent_tree(linkage, 1);
	jstring j = (*env)->NewStringUTF(env, s);
	//	delete[] s;
	return j;
}

/*
 * Class:		 relex_parser_LinkParserJNIClient
 * Method:		cConstituentString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_relex_parser_LinkParserJNIClient_cLinkString(JNIEnv *env, jclass cls)
{
	char *s = linkage_print_diagram(linkage);
	jstring j = (*env)->NewStringUTF(env, s);
	//	delete[] s;
	return j;
}

/*
 * Class:		 relex_0002fLinkParser
 * Method:		cIsPastTenseForm
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
Java_relex_parser_LinkParserJNIClient_cIsPastTenseForm(JNIEnv *env, jclass cls, jstring str)
{
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	if (isPastTenseForm(cStr,dict)==1)
		return TRUE;
	return FALSE;
}

/*
 * Class:		 relex_0002fLinkParser
 * Method:		cIsPastTenseForm
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
Java_relex_parser_LinkParserJNIClient_cIsEntity(JNIEnv *env, jclass cls, jstring str)
{
	const char *cStr = (*env)->GetStringUTFChars(env,str,0);
	if (isEntity(cStr,dict)==1)
		return TRUE;
	return FALSE;
}




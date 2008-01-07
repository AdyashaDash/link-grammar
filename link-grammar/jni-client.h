
#include <jni.h>
/* Header for class relex_parser_LinkParserJNIClient */

#ifndef _LinkParserJNIClient
#define _LinkParserJNIClient

#ifdef __cplusplus
extern "C" {
#endif
#undef relex_parser_LinkParserJNIClient_verbosity
#define relex_parser_LinkParserJNIClient_verbosity 1L
/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cSetMaxParseSeconds
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_relex_parser_LinkParserJNIClient_cSetMaxParseSeconds
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cSetMaxCost
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_relex_parser_LinkParserJNIClient_cSetMaxCost
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cInit
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_relex_parser_LinkParserJNIClient_cInit
	(JNIEnv *, jclass, jstring);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cTest
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_relex_parser_LinkParserJNIClient_cTest
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cParse
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_relex_parser_LinkParserJNIClient_cParse
	(JNIEnv *, jclass, jstring);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_relex_parser_LinkParserJNIClient_cClose
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cNumWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cNumWords
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cGetWord
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_relex_parser_LinkParserJNIClient_cGetWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cNumSkippedWords
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cNumSkippedWords
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cNumLinkages
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cNumLinkages
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cMakeLinkage
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_relex_parser_LinkParserJNIClient_cMakeLinkage
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkageNumViolations
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cLinkageNumViolations
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkageDisjunctCost
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cLinkageDisjunctCost
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cNumLinks
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cNumLinks
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkLWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cLinkLWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkRWord
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_relex_parser_LinkParserJNIClient_cLinkRWord
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkLLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_relex_parser_LinkParserJNIClient_cLinkLLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkRLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_relex_parser_LinkParserJNIClient_cLinkRLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_relex_parser_LinkParserJNIClient_cLinkLabel
	(JNIEnv *, jclass, jint);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cConstituentString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_relex_parser_LinkParserJNIClient_cConstituentString
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cLinkString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_relex_parser_LinkParserJNIClient_cLinkString
	(JNIEnv *, jclass);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cIsPastTenseForm
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_relex_parser_LinkParserJNIClient_cIsPastTenseForm
	(JNIEnv *, jclass, jstring);

/*
 * Class:     relex_parser_LinkParserJNIClient
 * Method:    cIsEntity
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_relex_parser_LinkParserJNIClient_cIsEntity
	(JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif

#endif

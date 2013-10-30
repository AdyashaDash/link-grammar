/*************************************************************************/
/* Copyright (c) 2012, 2013 Linas Vepstas <linasvepstas@gmail.com>       */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

/// This file provides a unit test for the operation of the viterbi parser.

#include "test-header.h"
#include "environment.h"

// ==================================================================
bool test_func()
{
	total_tests++;

	Environment* env = new Environment();

	Atom* word = ANODE(WORD, "wtf");
	Atom* disj =
		ALINK2(AND,
			ANODE(CONNECTOR, "Xd-"),
			ANODE(CONNECTOR, "MX-")
		);

	env->set_function("con", word, disj);
	Atom* got = env->get_function_value("con", word);

	Atom* expected =  // same as disj, just different addrs
		ALINK2(AND,
			ANODE(CONNECTOR, "Xd-"),
			ANODE(CONNECTOR, "MX-")
		);

	if (not (got->operator==(expected)))
	{
		cout << "Error: test failure on test \"test_disjoin_cost\"" << endl;
		cout << "=== Expecting:\n" << expected << endl;
		cout << "=== Got:\n" << got << endl;
		return false;
	}
	cout << "PASS: test_disjoin_cost" << endl;
	return true;
}


int ntest_env()
{
	size_t num_failures = 0;

	if (!test_func()) num_failures++;
	return num_failures;
}

// ==================================================================

int
main(int argc, char *argv[])
{
	size_t num_failures = 0;
	bool exit_on_fail = true;

	num_failures += ntest_env();
	report(num_failures, exit_on_fail);

	exit (0);
}


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

#include <link-grammar/link-includes.h>
#include <link-grammar/read-dict.h>

// ==================================================================
// XXX currently a copy of test_short_sent ...
bool test_cost(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);
	// print_dictionary_data(dict);

cout<<"xxxxxxxxxxxxxxxxxxxxxxxx last test xxxxxxxxxxxxxxxx" <<endl;
	Parser parser(dict);

	// Expecting more words to follow, so a non-trivial state.
	// In particular, the dictionary will link the left-wall to
	// "is", so "this" has to be pushed on stack until the "is"
	// shows up.  The test_seq_sent() below will link the other
	// way around.
	parser.streamin("this is");

	Lynk* alts = parser.get_alternatives();

	// At least one result should be this state pair.
	Lynk* sp =
		ALINK3(STATE_TRIPLE,
			ALINK0(SEQ),  // empty input
			ALINK0(SEQ),  // empty state
			ALINK2(SET,
				ALINK3(LING,
					ANODE(LING_TYPE, "Ss*b"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Ss*b+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Ss-")))
				,
				ALINK3(LING,
					ANODE(LING_TYPE, "Wi"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "LEFT-WALL"),
						ANODE(CONNECTOR, "Wi+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Wi-")))
			));

	if (empty_state)
	{
		Lynk* ans = ALINK1(SET, sp);
		if (not (ans->operator==(alts)))
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}
	else
	{
		// At least one alternative should be the desired state pair.
		bool found = false;
		size_t sz = alts->get_arity();
		for (size_t i=0; i<sz; i++)
		{
			Atom* a = alts->get_outgoing_atom(i);
			if (sp->operator==(a))
				found = true;
		}
		if (not found)
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting one of them to be:\n" << sp << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}

	cout<<"PASS: test_short_sent(" << id << ") " << endl;
	return true;
}

bool test_cost_this()
{
	return test_cost("short cost sent",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
		"is.v: Ss- and Wi-;"
		"is.w: [[Ss- and Wd-]];",
		true
	);
}

int ntest_cost()
{
	size_t num_failures = 0;

	if (!test_cost_this()) num_failures++;
	return num_failures;
}

// ==================================================================

int
main(int argc, char *argv[])
{
	size_t num_failures = 0;
	bool exit_on_fail = true;

	num_failures += ntest_cost();
	report(num_failures, exit_on_fail);

	exit (0);
}


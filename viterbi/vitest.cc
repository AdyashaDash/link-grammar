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

#include <stdlib.h>

#include <iostream>

#include <link-grammar/link-includes.h>
#include "read-dict.h"

#include "parser.h"

using namespace std;
using namespace link_grammar::viterbi;

#define Lynk link_grammar::viterbi::Link

#define ANODE(TYPE,NAME) (new Node(TYPE,NAME))
#define ALINK0(TYPE) (new Lynk(TYPE))
#define ALINK1(TYPE,A) (new Lynk(TYPE, A))
#define ALINK2(TYPE,A,B) (new Lynk(TYPE, A,B))
#define ALINK3(TYPE,A,B,C) (new Lynk(TYPE, A,B,C))
#define ALINK4(TYPE,A,B,C,D) (new Lynk(TYPE, A,B,C,D))
#define ALINK5(TYPE,A,B,C,D,E) (new Lynk(TYPE, A,B,C,D,E))


#define CHECK(NAME, EXPECTED, COMPUTED)                              \
	total_tests++;                                                    \
	if (not (EXPECTED->operator==(COMPUTED)))                         \
	{                                                                 \
		cout << "Error: test failure on " << NAME << endl;             \
		cout << "=== Expecting:\n" << EXPECTED << endl;                \
		cout << "=== Got:\n" << COMPUTED << endl;                      \
		return false;                                                  \
	}                                                                 \
	cout<<"PASS: " << NAME << endl;                                   \
	return true;


int total_tests = 0;

// ==================================================================
// Make sure that the disjoind functions actually work.

bool test_and_dnf_single()
{
	And* and_singleton = new And(ANODE(WORD, "AA1"));
	Or* computed = and_singleton->disjoin();

	Lynk* expected = ALINK1(OR, ANODE(WORD, "AA1"));

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_dnf_double()
{
	And* and_two = new And(ANODE(WORD, "AA1"), ANODE(WORD, "BB2"));
	Or* computed = and_two->disjoin();

	Lynk* expected =
	ALINK1(OR, ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")));

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_left()
{
	And* and_right = new And(
      ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ANODE(WORD, "RR1"));
	Or* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "RR1")),
		ALINK2(AND, ANODE(WORD, "CC3"), ANODE(WORD, "RR1"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_right()
{
	And* and_right = new And(ANODE(WORD, "AA1"),
      ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")));
	Or* computed = and_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_middle()
{
	And* and_mid = new And(ANODE(WORD, "AA1"),
      ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
      ANODE(WORD, "DD4"));
	Or* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"), ANODE(WORD, "DD4")),
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), ANODE(WORD, "DD4"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_quad()
{
	And* and_mid = new And(
      ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
      ALINK2(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4")));
	Or* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3")),
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "DD4")),
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "DD4"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_quad_right()
{
	And* and_mid = new And(
      ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
      ALINK2(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4")),
      ANODE(WORD, "EE5")
   );
	Or* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "AA1"), ANODE(WORD, "DD4"), ANODE(WORD, "EE5")),
		ALINK3(AND, ANODE(WORD, "BB2"), ANODE(WORD, "DD4"), ANODE(WORD, "EE5"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_and_distrib_quad_left()
{
	And* and_mid = new And(
      ANODE(WORD, "EE5"),
      ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
      ALINK2(OR, ANODE(WORD, "CC3"), ANODE(WORD, "DD4")));
	Or* computed = and_mid->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "AA1"), ANODE(WORD, "CC3")),
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "AA1"), ANODE(WORD, "DD4")),
		ALINK3(AND, ANODE(WORD, "EE5"), ANODE(WORD, "BB2"), ANODE(WORD, "DD4"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_dnf_single()
{
	Or* or_singleton = new Or(ANODE(WORD, "AA1"));
	Or* computed = or_singleton->disjoin();

	Lynk* expected = ALINK1(OR, ANODE(WORD, "AA1"));

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_dnf_double()
{
	Or* or_two = new Or(ANODE(WORD, "AA1"), ANODE(WORD, "BB2"));
	Or* computed = or_two->disjoin();

	Lynk* expected =
	ALINK2(OR, ANODE(WORD, "AA1"), ANODE(WORD, "BB2"));

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_left()
{
	Or* or_right = new Or(
      ALINK2(AND,
      	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")),
			ANODE(WORD, "RR1"))
	);
	Or* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "BB2"), ANODE(WORD, "RR1")),
		ALINK2(AND, ANODE(WORD, "CC3"), ANODE(WORD, "RR1"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_right()
{
	Or* or_right = new Or(
		ALINK2(AND,
   		ANODE(WORD, "AA1"),
      	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3")))
	);
	Or* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest()
{
	Or* or_right = new Or(
		ALINK1(OR,
			ALINK2(AND,
   			ANODE(WORD, "AA1"),
     		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"))))
	);
	Or* computed = or_right->disjoin();

	Lynk* expected =
	ALINK2(OR,
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3"))
	);

	CHECK(__FUNCTION__, expected, computed);
}

bool test_or_distrib_nest2()
{
	Or* or_right = new Or(
		ALINK3(OR,
			ANODE(WORD, "DD4"),
			ALINK2(AND,
   			ANODE(WORD, "AA1"),
     		 	ALINK2(OR, ANODE(WORD, "BB2"), ANODE(WORD, "CC3"))),
			ANODE(WORD, "EE5"))
	);
	Or* computed = or_right->disjoin();

	Lynk* expected =
	ALINK4(OR,
		ANODE(WORD, "DD4"),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "BB2")),
		ALINK2(AND, ANODE(WORD, "AA1"), ANODE(WORD, "CC3")),
		ANODE(WORD, "EE5")
	);

	CHECK(__FUNCTION__, expected, computed);
}

int ntest_disjoin()
{
	size_t num_failures = 0;
	if (!test_and_dnf_single()) num_failures++;
	if (!test_and_dnf_double()) num_failures++;
	if (!test_and_distrib_left()) num_failures++;
	if (!test_and_distrib_right()) num_failures++;
	if (!test_and_distrib_middle()) num_failures++;
	if (!test_and_distrib_quad()) num_failures++;
	if (!test_and_distrib_quad_right()) num_failures++;
	if (!test_and_distrib_quad_left()) num_failures++;

	if (!test_or_dnf_single()) num_failures++;
	if (!test_or_dnf_double()) num_failures++;
	if (!test_or_distrib_left()) num_failures++;
	if (!test_or_distrib_right()) num_failures++;
	if (!test_or_distrib_nest()) num_failures++;
	if (!test_or_distrib_nest2()) num_failures++;
	return num_failures;
}


// ==================================================================
// A simple hello test; several different dictionaries
// should give exactly the same output.  The input sentence
// is just one word, it should connect to the left-wall in
// just one way. The result should be just one alternative:
// that alternatives has an empty state, and output with
// just one link.
bool test_hello(const char *id, const char *dict_str,
                bool empty_state, bool must_connect)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	parser.streamin("Hello");

	// This is the expected output, no matter what the
	// dictionary may be.  Its just one word, connected to the wall.
	Lynk* one_word =
	ALINK3(LING,
		ANODE(LING_TYPE, "Wd"),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "LEFT-WALL"),
			ANODE(CONNECTOR, "Wd+")
		),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "Hello"),
			ANODE(CONNECTOR, "Wd-")
		)
	);

	if (empty_state)
	{
		// This is the expected set of alternatives: just one alternative,
		// a single, empty state, and the output, above.
		Lynk* ans =
		ALINK1(SET,
			ALINK2(STATE_PAIR,
				ALINK0(SEQ),
				ALINK1(SEQ, one_word)
			)
		);

		Lynk* alts = parser.get_alternatives();
		if (not (ans->operator==(alts)))
		{
			cout << "Error: test failure on test \"" << id << "\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}
	else
	{
		// This test will have lots of alternatives. One should have
		// an empty state.
		Lynk* ans =
			ALINK2(STATE_PAIR,
				ALINK0(SEQ),
				ALINK1(SEQ, one_word)
			);

		Lynk* out = new Seq(one_word);

		bool pass_test = false;
		Lynk* alts = parser.get_alternatives();
		for (size_t i=0; i<alts->get_arity(); i++)
		{
			Atom* alt = alts->get_outgoing_atom(i);
			StatePair* sp = dynamic_cast<StatePair*>(alt);

			// At least one alternative should have an empty state.
			if (ans->operator==(alt))
				pass_test = true;

			// In all cases, the output should be just the one word,
			// no matter what the state.
			if (must_connect and not sp->get_output()->operator==(out))
				pass_test = false;
		}
		if (pass_test)
		{
			cout<<"PASS: test_hello(" << id << ") " << endl;
			return true;
		}
		cout << "Error: test failure on test \"" << id << "\"" << endl;
		cout << "=== Expecting:\n" << ans << endl;
		cout << "=== Got:\n" << alts << endl;
		return false;
	}

	cout<<"PASS: test_hello(" << id << ") " << endl;
	return true;
}

bool test_simplest()
{
	return test_hello ("test_simplest",
		"LEFT-WALL: Wd+;"
		"Hello: Wd-;",
		true, true
	);
}

bool test_simple_left_disj()
{
	return test_hello ("simple left disj",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd-;",
		true, true
	);
}

bool test_simple_optional_left_cset()
{
	return test_hello ("optional left cset",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {CP+} & {Xx+} & {RW+ or Xp+};"
		"Hello: Wd-;",
		false, true
	);
}

bool test_simple_right_disj()
{
	return test_hello ("simple right disj",
		"LEFT-WALL: Wd+;"
		"Hello: Wd- or Wi-;",
		true, true
	);
}

bool test_simple_right_required_cset()
{
	return test_hello ("required right cset",
		"LEFT-WALL: Wd+;"
		"Hello: Wd- or Xi- or (Xj- & (A+ or B+));",
		true, true
	);
}

bool test_simple_optional()
{
	return test_hello ("optionals in both csets",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {CP+} & {Xx+} & {RW+ or Xp+};"
		"Hello: Wd- or Xi- or (Xj- & {A+ or B+});",
		false, true
	);
}

bool test_simple_onereq()
{
	return test_hello ("one required link and opt righties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- & {A+} & {B+} & {C+};",
		false, true
	);
}

bool test_simple_zeroreq()
{
	return test_hello ("zero required links and opt righties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: {Wd-} & {A+} & {B+} & {C+};",
		false, false
	);
}

bool test_simple_onereq_and_left()
{
	return test_hello ("one required link and opt lefties (simple)",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- & {A-} & {B-} & {C+};",
		false, true
	);
}

int ntest_simple()
{
	size_t num_failures = 0;

	if (!test_simplest()) num_failures++;
	if (!test_simple_left_disj()) num_failures++;
	if (!test_simple_optional_left_cset()) num_failures++;
	if (!test_simple_right_disj()) num_failures++;
	if (!test_simple_right_required_cset()) num_failures++;
	if (!test_simple_optional()) num_failures++;
	if (!test_simple_onereq()) num_failures++;
	if (!test_simple_zeroreq()) num_failures++;
	if (!test_simple_onereq_and_left()) num_failures++;
	return num_failures;
}

// ==================================================================
// A test of two alternative parses of a sentence with single word in it.
// Expect to get back a set with two alternative parses, each parse is
// assigned a probability of 1/2.

bool test_alternative(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	parser.streamin("Hello");

	Lynk* alt_out_one =
	ALINK3(LING,
		ANODE(LING_TYPE, "Wd"),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "LEFT-WALL"),
			ANODE(CONNECTOR, "Wd+")
		),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "Hello"),
			ANODE(CONNECTOR, "Wd-")
		)
	);

	Lynk* alt_out_two =
	ALINK3(LING,
		ANODE(LING_TYPE, "Wi"),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "LEFT-WALL"),
			ANODE(CONNECTOR, "Wi+")
		),
		ALINK2(WORD_DISJ,
			ANODE(WORD, "Hello"),
			ANODE(CONNECTOR, "Wi-")
		)
	);

	Lynk* alt_pair_one =
	ALINK2(STATE_PAIR,
		ALINK0(SEQ),
		ALINK1(SEQ, alt_out_one)
	);

	Lynk* alt_pair_two =
	ALINK2(STATE_PAIR,
		ALINK0(SEQ),
		ALINK1(SEQ, alt_out_two)
	);

	if (empty_state)
	{
		// This is the expected set of alternatives: two alternatives,
		// each with an empty state, and one of the two outputs, above.
		Lynk* ans = ALINK2(SET, alt_pair_one, alt_pair_two);

		Lynk* output = parser.get_alternatives();
		if (not (ans->operator==(output)))
		{
			cout << "Error: test failure on test \"" << id <<"\"" << endl;
			cout << "=== Expecting:\n" << ans << endl;
			cout << "=== Got:\n" << output << endl;
			return false;
		}
	}
	else
	{
		// The final state here might not be empty.  However, both
		// of the alternatives should show up somwhere in the output.

		bool found_one = false;
		bool found_two = false;
		Lynk* alts = parser.get_alternatives();
		for (size_t i=0; i<alts->get_arity(); i++)
		{
			Atom* alt = alts->get_outgoing_atom(i);
			StatePair* sp = dynamic_cast<StatePair*>(alt);

			// At least one alternative should have an empty state.
			if (alt_pair_one->operator==(alt))
				found_one = true;

			if (alt_pair_two->operator==(alt))
				found_two = true;
		}

		// Both should have been found, somewhere.
		if (not alt_pair_one or not alt_pair_two)
		{
			cout << "Error: test failure on test \"" << id << "\"" << endl;
			cout << "=== Expecting this alt:\n" << alt_pair_one << endl;
			cout << "=== Expecting this alt:\n" << alt_pair_two << endl;
			cout << "=== Got:\n" << alts << endl;
			return false;
		}
	}

	cout<<"PASS: test_alternative(" << id << ") " << endl;
	return true;
}

bool test_two_alts()
{
	return test_alternative("two alternatives",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"Hello: Wd- or Wi-;",
		true
	);
}

bool test_two_opts()
{
	return test_alternative("two alts plus opts",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or Wi- or (Xj- & {A+ or B+});",
		false
	);
}

bool test_two_one_opts()
{
	return test_alternative("two alt, or one opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or {Wi-} or (Xj- & {A+ or B+});",
		false
	);
}

bool test_two_all_opts()
{
	return test_alternative("two alts, or all opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: {Wd-} or {Wi-} or (Xj- & {A+ or B+});",
		false
	);
}

bool test_two_and_opts()
{
	return test_alternative("two alts, and an opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or (Wi- & {Xj- & {A+ or B+}} & {C+});",
		false
	);
}

bool test_two_and_no_opts()
{
	return test_alternative("two alt, and all opt",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or ({Wi-} & {Xj- & {A+ or B+}} & {C+});",
		false
	);
}

bool test_two_and_excess()
{
	return test_alternative("two alt, and excess reqs",
		"LEFT-WALL: (Wd+ or Wi+ or Wq+) & {A+};"
		"Hello: Wd- or (Wi- & Xj- & {A+ or B+} & {C+}) or Wi-;",
		false
	);
}

int ntest_two()
{
	size_t num_failures = 0;

	if (!test_two_alts()) num_failures++;
	if (!test_two_opts()) num_failures++;
	if (!test_two_one_opts()) num_failures++;
	if (!test_two_all_opts()) num_failures++;
	if (!test_two_and_opts()) num_failures++;
	if (!test_two_and_no_opts()) num_failures++;
	if (!test_two_and_excess()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_simple_state(const char *id, const char *dict_str)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this");

	Lynk* ans_state =
	ALINK2(SEQ,
		ALINK2(WORD_CSET,
			ANODE(WORD, "this"),
			ANODE(CONNECTOR, "Ss*b+")
		),
		ALINK2(WORD_CSET,
			ANODE(WORD, "LEFT-WALL"),
			ALINK3(OR,
				ANODE(CONNECTOR, "Wd+"),
				ANODE(CONNECTOR, "Wi+"),
				ANODE(CONNECTOR, "Wq+")
			)
		)
	);

	Lynk* ans =
	ALINK1(SET,
		ALINK2(STATE_PAIR,
			ans_state,
			ALINK0(SEQ)
		)
	);

	Lynk* state = parser.get_alternatives();
	if (not (ans->operator==(state)))
	{
		cout << "Error: test failure on test " << id << endl;
		cout << "=== Expecting state:\n" << ans << endl;
		cout << "=== Got state:\n" << state << endl;
		return false;
	}

	cout<<"PASS: test_simple_state(" << id << ") " << endl;
	return true;
}

bool test_first_state()
{
	return test_simple_state("first state",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
	);
}

bool test_first_opt_lefty()
{
	return test_simple_state("first state, left-going optional",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ and {Xi-};"
	);
}

bool test_first_or_lefty()
{
	return test_simple_state("first state, OR left-going",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or Xi-;"
	);
}

bool test_first_or_multi_lefty()
{
	return test_simple_state("first state, multi-OR left-going",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or Xi- or Y- or Z-;"
	);
}

bool test_first_opt_cpx()
{
	return test_simple_state("first state, complex left-going optional",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ and {Xi- or P- or {Q- & Z+}};"
	);
}

bool test_first_infer_opt()
{
	return test_simple_state("first state, complex infer optional",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ and (Xi- or P- or {Q- & Z+});"
	);
}

int ntest_first()
{
	size_t num_failures = 0;

	if (!test_first_state()) num_failures++;
	if (!test_first_opt_lefty()) num_failures++;
	if (!test_first_or_lefty()) num_failures++;
	if (!test_first_or_multi_lefty()) num_failures++;
	if (!test_first_opt_cpx()) num_failures++;
	if (!test_first_infer_opt()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_short_sent(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);
	// print_dictionary_data(dict);

	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this is");

	Lynk* alts = parser.get_alternatives();

	// At least one result should be this state pair.
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),  // empty state
			ALINK2(SEQ,
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

bool test_short_this()
{
	return test_short_sent("short sent",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
		"is.v: Ss- and Wi-;",
		true
	);
}

bool test_short_this_opt()
{
	return test_short_sent("short sent opt",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+;"
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

bool test_short_this_obj_opt()
{
	return test_short_sent("short sent with obj",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or Os-;"
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

bool test_short_this_costly()
{
	return test_short_sent("short sent with costly null",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Ss*b+ or [[[()]]];"
		"is.v: Ss- and Wi-;",
		true
	);
}

bool test_short_this_complex()
{
	return test_short_sent("short sent complex",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		""
		"<CLAUSE>: {({@COd-} & (C-)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-];"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

bool test_short_this_noun_dict()
{
	return test_short_sent("short sent realistic dict entry for noun",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"<costly-null>: [[[()]]];"
		""
		"<post-nominal-x>:"
		"  ({[B*j+]} & Xd- & (Xc+ or <costly-null>) & MX-);"
		""
		"<clause-conjoin>: RJrc- or RJlc+;"
		""
		"<CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		""
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-]"
		"  or <post-nominal-x>"
		"  or <costly-null>;"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and Wi- and {O+};",
		false
	);
}

int ntest_short()
{
	size_t num_failures = 0;

	if (!test_short_this()) num_failures++;
	if (!test_short_this_opt()) num_failures++;
	if (!test_short_this_obj_opt()) num_failures++;
	if (!test_short_this_costly()) num_failures++;
	if (!test_short_this_complex()) num_failures++;
	if (!test_short_this_noun_dict()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_seq_sent(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this is");

	Lynk* alts = parser.get_alternatives();

	// At least one result should be this state pair.
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK0(SEQ),  // empty state
			ALINK2(SEQ,
				ALINK3(LING,
					ANODE(LING_TYPE, "Wd"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "LEFT-WALL"),
						ANODE(CONNECTOR, "Wd+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Wd-"))),
				ALINK3(LING,
					ANODE(LING_TYPE, "Ss*b"),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "this"),
						ANODE(CONNECTOR, "Ss*b+")),
					ALINK2(WORD_DISJ,
						ANODE(WORD, "is.v"),
						ANODE(CONNECTOR, "Ss-")))));

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

bool test_seq_this()
{
	return test_seq_sent("short seq sent",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and Ss*b+;"
		"is.v: Ss-;",
		true
	);
}

bool test_seq_this_opt()
{
	return test_seq_sent("short seq sent opt",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and Ss*b+;"
		"is.v: Ss- and {O+};",
		false
	);
}

bool test_seq_this_obj_opt()
{
	return test_seq_sent("short seq sent with obj",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and (Ss*b+ or Os-);"
		"is.v: Ss- and {O+};",
		false
	);
}

bool test_seq_this_costly()
{
	return test_seq_sent("short seq sent with costly null",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"this: Wd- and (Ss*b+ or [[[()]]]);"
		"is.v: Ss-;",
		true
	);
}

bool test_seq_this_complex()
{
	return test_seq_sent("short seq sent complex",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		""
		"<CLAUSE>: {({@COd-} & (C-)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-];"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and {O+};",
		false
	);
}

bool test_seq_this_noun_dict()
{
	return test_seq_sent("short seq sent realistic dict entry for noun",
		"LEFT-WALL: Wd+ or Wi+ or Wq+;"
		"<costly-null>: [[[()]]];"
		""
		"<post-nominal-x>:"
		"  ({[B*j+]} & Xd- & (Xc+ or <costly-null>) & MX-);"
		""
		"<clause-conjoin>: RJrc- or RJlc+;"
		""
		"<CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		""
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-]"
		"  or <post-nominal-x>"
		"  or <costly-null>;"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"is.v: Ss- and {O+};",
		false
	);
}


bool test_seq_this_verb_dict()
{
	return test_seq_sent("short seq sent realistic dict entry for verb",
		"LEFT-WALL:"
		"  (Wd+ or Wq+ or Ws+ or Wj+ or Wc+ or Wi+ or We+ or Qd+)"
		"    & {CP+} & {Xx+} & {RW+ or Xp+};"
		""
		"<costly-null>: [[[()]]];"
		""
		"<post-nominal-x>:"
		"  ({[B*j+]} & Xd- & (Xc+ or <costly-null>) & MX-);"
		""
		"<clause-conjoin>: RJrc- or RJlc+;"
		""
		"<CLAUSE>: {({@COd-} & (C- or <clause-conjoin>)) or ({@CO-} & (Wd- & {CC+})) or [Rn-]};"
		""
		"<noun-main-h>:"
		"  (Jd- & Dmu- & Os-)"
		"  or (Jd- & Dmu- & {Wd-} & Ss*b+)"
		"  or (Ss*b+ & <CLAUSE>) or SIs*b- or [[Js-]] or [Os-]"
		"  or <post-nominal-x>"
		"  or <costly-null>;"
		""
		"this:"
		"  <noun-main-h>;"
		""
		"<verb-and-s->: {@E-} & VJrs-;"
		"<verb-and-s+>: {@E-} & VJls+;"
		""
		"<verb-x-s,u>: {@E-} & (Ss- or SFs- or SFu- or (RS- & Bs-));"
		""
		"<vc-be-obj>:"
		"  {@EBm+} & O*t+ & {@MV+};"
		""
		"<vc-be-no-obj>:"
		"  ({@EBm+} & ((([B**t-] or [K+] or BI+ or OF+ or PF- or"
		"      (Osi+ & R+ & Bs+) or"
		"      (Opi+ & R+ & Bp+) or"
		"      [[()]]) & {@MV+}) or"
		"    (Pp+ & {THi+ or @MV+}) or"
		"    THb+ or"
		"    TO+ or"
		"    Pa+)) or"
		"  ({N+} & (AF- or Pv+ or I*v+)) or"
		"  (({N+} or {Pp+}) & Pg*b+);"
		""
		"<vc-be>: <vc-be-no-obj> or <vc-be-obj>;"
		""
		"is.v:"
		"  (<verb-x-s,u> & <vc-be>) or"
		"  (<verb-and-s-> & <vc-be>) or (<vc-be> & <verb-and-s+>) or"
		"  (((Rw- or ({Ic-} & Q-) or [()]) & (SIs+ or SFIs+)) & <vc-be>);"
		"",
		false
	);
}

int ntest_short_seq()
{
	size_t num_failures = 0;

	if (!test_seq_this()) num_failures++;
	if (!test_seq_this_opt()) num_failures++;
	if (!test_seq_this_obj_opt()) num_failures++;
	if (!test_seq_this_costly()) num_failures++;
	if (!test_seq_this_complex()) num_failures++;
	if (!test_seq_this_noun_dict()) num_failures++;
	if (!test_seq_this_verb_dict()) num_failures++;

	return num_failures;
}

// ==================================================================

bool test_state_sent(const char *id, const char *dict_str, bool empty_state)
{
	total_tests++;

	Dictionary dict = dictionary_create_from_utf8(dict_str);

	// print_dictionary_data(dict);

cout<<"xxxxxxxxxxxxxxxxxxxxxxxx last test xxxxxxxxxxxxxxxx" <<endl;
	Parser parser(dict);
	// Expecting more words to follow, so a non-trivial state.
	parser.streamin("this is a test");

	Lynk* alts = parser.get_alternatives();

	// We expect no output, and a crazy state:
	Lynk* sp =
		ALINK2(STATE_PAIR,
			ALINK5(SEQ,
				ALINK2(WORD_CSET,
					ANODE(WORD, "test.n"),
					ALINK2(OR,
						ANODE(CONNECTOR, "XXXGIVEN+"),
						ANODE(CONNECTOR, "AN+")))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "a"),
					ANODE(CONNECTOR, "Ds+"))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "is.v"),
					ANODE(CONNECTOR, "SIs+"))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "this.J2"),
					ANODE(CONNECTOR, "JDBKQ+"))
				,
				ALINK2(WORD_CSET,
					ANODE(WORD, "LEFT-WALL"),
					ANODE(CONNECTOR, "Wq+"))
			),
			ALINK0(SEQ));  // empty output

	Lynk* ans = ALINK1(SET, sp);
	if (not (ans->operator==(alts)))
	{
		cout << "Error: test failure on test \"" << id <<"\"" << endl;
		cout << "=== Expecting:\n" << ans << endl;
		cout << "=== Got:\n" << alts << endl;
		return false;
	}
	cout<<"PASS: test_state_sent(" << id << ") " << endl;
	return true;
}

bool test_state_order()
{
	return test_state_sent("short state sent",
		"LEFT-WALL: Wq+;"
		"this.J2: JDBKQ+;"
		"is.v: SIs+;"
		"a: Ds+;"
		"test.n: XXXGIVEN+ or AN+;",
		false
	);
}

int ntest_short_state()
{
	size_t num_failures = 0;

	if (!test_state_order()) num_failures++;
	return num_failures;
}

// ==================================================================

void report(int num_failures, bool exit_on_fail)
{
	if (num_failures)
	{
		cout << "Test failures = " << num_failures
		     << " out of " << total_tests
		     << " total, so far." << endl;
		if (exit_on_fail)
			exit(1);
	}

	cout << "All " << total_tests
	     << " tests so far pass." << endl;
}

int
main(int argc, char *argv[])
{
	size_t num_failures = 0;
	bool exit_on_fail = true;

	num_failures += ntest_disjoin();
	report(num_failures, exit_on_fail);

	num_failures += ntest_simple();
	report(num_failures, exit_on_fail);

	num_failures += ntest_two();
	report(num_failures, exit_on_fail);

	num_failures += ntest_first();
	report(num_failures, exit_on_fail);

	num_failures += ntest_short();
	report(num_failures, exit_on_fail);

	num_failures += ntest_short_seq();
	report(num_failures, exit_on_fail);

	num_failures += ntest_short_state();
	report(num_failures, exit_on_fail);

	exit (0);
}


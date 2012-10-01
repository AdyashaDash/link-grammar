/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <link-grammar/link-includes.h>
#include "api-types.h"
#include "read-dict.h"
#include "structures.h"

#include "atom.h"
#include "parser.h"
#include "viterbi.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

Parser::Parser(Dictionary dict)
	: _dict(dict), _state(NULL)
{
	initialize_state();
}

/**
 * Convert LG dictionary expression to atomic formula.
 *
 * The returned expression is in the form of an opencog-style
 * prefix-notation boolean expression.  Note that it is not in any
 * particular kind of normal form.  In particular, some AND nodes
 * may have only one child: these can be removed.
 *
 * Note that the order of the connectors is important: while linking,
 * these must be satisfied in left-to-right (nested!?) order.
 *
 * Optional clauses are indicated by OR-ing with null, where "null"
 * is a CONNECTOR Node with string-value "0".  Optional clauses are
 * not necessarily in any sort of normal form; the null connector can
 * appear anywhere.
 */
Atom * Parser::lg_exp_to_atom(Exp* exp)
{
	if (CONNECTOR_type == exp->type)
	{
		stringstream ss;
		if (exp->multi) ss << "@";
		ss << exp->u.string << exp->dir;
		return new Node(CONNECTOR, ss.str());
	}

	// Whenever a null appears in an OR-list, it means the 
	// entire OR-list is optional.  A null can never appear
	// in an AND-list.
	E_list* el = exp->u.l;
	if (NULL == el)
		return new Node(CONNECTOR, "0");

	// The data structure that link-grammar uses for connector
	// expressions is totally insane, as witnessed by the loop below.
	// Anyway: operators are infixed, i.e. are always binary,
	// with exp->u.l->e being the left hand side, and
	//      exp->u.l->next->e being the right hand side.
	// This means that exp->u.l->next->next is always null.
	OutList alist;
	alist.push_back(lg_exp_to_atom(el->e));
	el = el->next;

	while (el && exp->type == el->e->type)
	{
		el = el->e->u.l;
		alist.push_back(lg_exp_to_atom(el->e));
		el = el->next;
	}

	if (el)
		alist.push_back(lg_exp_to_atom(el->e));

	if (AND_type == exp->type)
		return new Link(AND, alist);

	if (OR_type == exp->type)
		return new Link(OR, alist);

	assert(0, "Not reached");
}

/**
 * Return atomic formula connector expression for the given word.
 *
 * This looks up the word in the link-grammar dictionary, and converts
 * the resulting link-grammar connective expression into an atomic
 * formula.
 */
Link * Parser::word_consets(const string& word)
{
	// See if we know about this word, or not.
	Dict_node* dn_head = dictionary_lookup_list(_dict, word.c_str());
	if (!dn_head) return NULL;

	OutList djset;
	for (Dict_node*dn = dn_head; dn; dn= dn->right)
	{
		Exp* exp = dn->exp;
cout<<"duude: " << dn->string << ": ";
print_expression(exp);

		Atom *dj = lg_exp_to_atom(exp);

		// First atom at the from of the outgoing set is the word itself.
		// Second atom is the first disjuct that must be fulfilled.
		OutList dlist;
		Node *nword = new Node(WORD, dn->string);
		dlist.push_back(nword);
		dlist.push_back(dj);

		djset.push_back(new Link(WORD_DISJ, dlist));
	}
	return new Link(SET, djset);
}

/**
 * Set up initial viterbi state for the parser
 */
void Parser::initialize_state()
{
	const char * wall_word = "xLEFT-WALL";

	Link *wall_disj = word_consets(wall_word);

	OutList statev;
	statev.push_back(wall_disj);

	_state = new Link(STATE, statev);
}

/**
 * Add a single word to the parse.
 */
void Parser::stream_word(const string& word)
{
	Link *djset = word_consets(word);
	if (!djset)
	{
		cout << "Unhandled error; word not in dict: " << word << endl;
		return;
	}

	// Try to add each dictionary entry to the parse state.
	for (int i = 0; i < djset->get_arity(); i++)
	{
		stream_word_conset(dynamic_cast<Link*>(djset->get_outgoing_atom(i)));
	}
}

/** convenience wrapper */
Link* Parser::get_state_set()
{
	Link* state_set = dynamic_cast<Link*>(_state->get_outgoing_atom(0));
	assert(state_set, "State set is empty");
	return state_set;
}

/**
 * Add a single dictionary entry to the parse stae, if possible.
 */
void Parser::stream_word_conset(Link* wconset)
{
	// wconset should be pointg at:
	// WORD_DISJ :
	//   WORD : LEFT-WALL
	//   AND :
	//      CONNECTOR : Wd+  etc...

	assert(wconset, "Unexpected NULL dictionary entry!");
	assert(WORD_DISJ == wconset->get_type(), "Unexpected link type in stream.");
	assert(2 == wconset->get_arity(), "Wrong arity for word connector set");

	Atom* conset = wconset->get_outgoing_atom(1);

cout<<"state "<<_state<<endl;
	Link* state_set = get_state_set();
	for (int i = 0; i < state_set->get_arity(); i++)
	{
		Link* swc = dynamic_cast<Link*>(state_set->get_outgoing_atom(i));
		assert(swc, "State word-connectorset is null");
		assert(2 == swc->get_arity(), "Wrong arity for state word conset");

		Atom* scon = swc->get_outgoing_atom(1);
		find_matches(scon, conset);
	}
}

/**
 * Compare two connector strings, se if they mate.
 */
bool Parser::conn_match(const string& ls, const string& rs)
{
	char ldir = *ls.rbegin();
	char rdir = *rs.rbegin();
	assert (ldir == '+' or ldir == '-', "Bad word direction");
	assert (rdir == '+' or rdir == '-', "Bad word direction");

	/* direction signs must couple */
	if ('+' == ldir and '-' != rdir) return false;
	if ('-' == ldir and '+' != rdir) return false;

	/* Captial letters must match. Wildcards match anything lower-case.
	 * This loop, as curretnly written, does some extra, needless compares.
	 * But connectors are so short, so screw it. 
	 */
	string::const_iterator lp = ls.begin();
	string::const_iterator rp = rs.begin();
	size_t len = -1 + min(ls.size(), rs.size());
	while (0 < len)
	{
		if ((isupper(*lp) or isupper(*rp)) and *lp != *rp) return false;
		else if (('*' != *lp or '*' != *rp) and *lp != *rp) return false;
		lp++;
		rp++;
		len--;
	}

	return true;
}

Link* Parser::find_matches(Atom* left, Atom* right)
{
cout<<"stah "<<left<<endl;
cout<<"dj "<<right<<endl;
	Node *lnode = dynamic_cast<Node*>(left);
	Node *rnode = dynamic_cast<Node*>(right);
	if (lnode and rnode)
	{
		assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
		assert(rnode->get_type() == CONNECTOR, "Expecting connector on right");
		if (conn_match(lnode->get_name(), rnode->get_name()))
		{
cout<<"Yayyyyae  nodes match!"<<endl;
		}
		return NULL;  // XXX!?!?!
	}
	return NULL;
}

/**
 * Add a stream of text to the input.
 *
 * No particular assumptiions are made about the input, other than
 * that its space-separated words (i.e. no HTML markup or other junk)
 */
void Parser::streamin(const string& text)
{
	stream_word(text);
}

void viterbi_parse(Dictionary dict, const char * sentence)
{
	Parser pars(dict);

	pars.streamin(sentence);
}

} // namespace viterbi
} // namespace link-grammar

// Wrapper to escape out from C++
void viterbi_parse(const char * sentence, Dictionary dict)
{
	link_grammar::viterbi::viterbi_parse(dict, sentence);
}


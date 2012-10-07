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
#include <vector>

#include "utilities.h"

#include "atom.h"
#include "connect.h"
#include "connector-utils.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

#define OPTIONAL_CLAUSE "0"

/**
 * constructor: argument is a right-sided connector that this class
 * will try connecting to.
 */
Connect::Connect(WordCset* right_wconset)
	: _right_cset(right_wconset)
{
	// right_cset should be pointing at:
	// WORD_CSET :
	//   WORD : blah.v
	//   AND :
	//      CONNECTOR : Wd-  etc...

	assert(_right_cset, "Unexpected NULL dictionary entry!");
	assert(WORD_CSET == _right_cset->get_type(), "Expecting right word cset.");
	assert(2 == _right_cset->get_arity(), "Wrong arity for word connector set");

	_rcons = _right_cset->get_outgoing_atom(1);
}

/**
 * Try connecting this connector set, from the left, to what was passed
 * in ctor.
 */
Link* Connect::try_connect(WordCset* left_cset)
{
	assert(left_cset, "State word-connectorset is null");
	assert(WORD_CSET == left_cset->get_type(), "Expecting left word cset.");
	assert(2 == left_cset->get_arity(), "Wrong arity for state word conset");
	Atom* left_a = left_cset->get_outgoing_atom(1);

	Atom *right_a = _rcons;
cout<<"state cset "<<left<<endl;
cout<<"word cset "<<right<<endl;

	// If the word connector set is a single solitary node, then
	// its not a set, its a single connecter.  Try to hook it up
	// to something on the left.
	Link* conn = conn_connect_aa(left_cset, left_a, right_a);
	if (!conn)
		return NULL;
cout<<"got one it is "<<conn<<endl;

	// At this point, conn holds an LG link type, and the
	// two disjuncts that were mated.  Re-assemble these
	// into a pair of word_disjuncts (i.e. stick the word
	// back in there, as that is what later stages need).
	return reassemble(conn, left_cset, _right_cset);
}

// =============================================================

// At this point, conn holds an LG link type, and the
// two disjuncts that were mated.  Re-assemble these
// into a pair of word_disjuncts (i.e. stick the word
// back in there, as that is what later stages need).
//
// The left_cset and right_cset are assumed to be the word-connector
// sets that matched. These are needed, only to extract the words;
// the rest is dicarded.
Link* Connect::reassemble(Link* conn, WordCset* left_cset, WordCset* right_cset)
{
	OutList lwdj;
	lwdj.push_back(left_cset->get_outgoing_atom(0));  // the word
	lwdj.push_back(conn->get_outgoing_atom(1));       // the connector
	Link *lwordj = new Link(WORD_DISJ, lwdj);

	OutList rwdj;
	rwdj.push_back(right_cset->get_outgoing_atom(0));   // the word
	rwdj.push_back(conn->get_outgoing_atom(2));         // the connector
	Link *rwordj = new Link(WORD_DISJ, rwdj);

	OutList lo;
	lo.push_back(conn->get_outgoing_atom(0));
	lo.push_back(lwordj);
	lo.push_back(rwordj);
	Link *lg_link = new Link (LINK, lo);

cout<<"normalized into "<<lg_link<<endl;
	return lg_link;
}

// =============================================================
/**
 * Dispatch appropriatly, depending on whether left atom is node or link
 */
Link* Connect::conn_connect_aa(WordCset* left_cset, Atom *latom, Atom* ratom)
{
	Node* lnode = dynamic_cast<Node*>(latom);
	if (lnode)
		return conn_connect_na(left_cset, lnode, ratom);

	Link* llink = dynamic_cast<Link*>(latom);
	return conn_connect_ka(left_cset, llink, ratom);
}

Link* Connect::conn_connect_na(WordCset* left_cset, Node* lnode, Atom *ratom)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
	Node* rnode = dynamic_cast<Node*>(ratom);
	if (rnode)
		return conn_connect_nn(left_cset, lnode, rnode);

	Link* rlink = dynamic_cast<Link*>(ratom);
	return conn_connect_nk(left_cset, lnode, rlink);
}

// =============================================================
/**
 * Connect left_cset and _right_cset with an LG_LINK
 * lnode and rnode are the two connecters that actually mate.
 */
Link* Connect::conn_connect_nn(WordCset* left_cset, Node* lnode, Node* rnode)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
cout<<"try match connectors l="<<lnode->get_name()<<" to r="<< rnode->get_name() << endl;
	if (!conn_match(lnode->get_name(), rnode->get_name()))
		return NULL;
	
cout<<"Yayyyyae connectors match!"<<endl;
	string link_name = conn_merge(lnode->get_name(), rnode->get_name());
	OutList linkage;
	linkage.push_back(new Node(LINK_TYPE, link_name));
	linkage.push_back(lnode);
	linkage.push_back(rnode);
	return new Link(LINK, linkage);
}

/**
 * Connect left_cset and _right_cset with an LG_LINK
 * lnode and rnode are the two connecters that actually mate.
 */
Link* Connect::conn_connect_nk(WordCset* left_cset, Node* lnode, Link* rlink)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
cout<<"try match con l="<<lnode->get_name()<<" to cset r="<< rlink << endl;

	// If the connector set is a disjunction, then try each of them, in turn.
	if (OR == rlink->get_type())
	{
		for (int i = 0; i < rlink->get_arity(); i++)
		{
			Atom* a = rlink->get_outgoing_atom(i);
			Link* conn = conn_connect_na(left_cset, lnode, a);

			// If the shoe fits, wear it.
			if (conn)
				return conn;
		}
	}

	return NULL;
}

// =============================================================
#if NOT_NEEDED
bool Connect::is_optional(Atom *a)
{
	AtomType ty = a->get_type();
	if (CONNECTOR == ty)
	{
		Node* n = dynamic_cast<Node*>(a);
		if (n->get_name() == OPTIONAL_CLAUSE)
			return true;
		return false;
	}
	assert (OR == ty or AND == ty, "Must be boolean junction");

	Link* l = dynamic_cast<Link*>(a);
	for (int i = 0; i < l->get_arity(); i++)
	{
		Atom *a = l->get_outgoing_atom(i);
		bool c = is_optional(a);
		if (OR == ty)
		{
			// If anything in OR is optional, the  whole clause is optional.
			if (c) return true;
		}
		else
		{
			// ty is AND
			// If anything in AND is isn't optional, then something is required
			if (!c) return false;
		}
	}
	
	// All disj were requied.
	if (OR == ty) return false;

	// All conj were optional.
	return true;
}
#endif

Link* Connect::conn_connect_ka(WordCset* left_cset, Link* llink, Atom* ratom)
{
cout<<"Enter recur l=" << llink->get_type()<<endl;

	for (int i = 0; i < llink->get_arity(); i++)
	{
		Atom* a = llink->get_outgoing_atom(i);
		Node* lnode = dynamic_cast<Node*>(a);
		if (lnode) 
		{
			if (lnode->get_name() == OPTIONAL_CLAUSE)
				continue;

			// Only one needs to be satisfied for OR clause
			if (OR == llink->get_type())
				return conn_connect_na(left_cset, lnode, ratom);

			// If we are here, then its an AND. 
			assert(0, "Implement me node AND");
		}

		Link* clink = dynamic_cast<Link*>(a);
		return conn_connect_ka(left_cset, clink, ratom);
	}

	return NULL;
}

} // namespace viterbi
} // namespace link-grammar


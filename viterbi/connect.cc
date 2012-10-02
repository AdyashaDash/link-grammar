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
Connect::Connect(Link* right_wconset)
	: _right_cset(right_wconset)
{
   // right_cset should be pointing at:
   // WORD_CSET :
   //   WORD : blah.v
   //   AND :
   //      CONNECTOR : Wd-  etc...

   assert(_right_cset, "Unexpected NULL dictionary entry!");
   assert(WORD_CSET == _right_cset->get_type(), "Unexpected link type word cset.");
   assert(2 == _right_cset->get_arity(), "Wrong arity for word connector set");

   _rcons = _right_cset->get_outgoing_atom(1);
}

/**
 * Try connecting this connector set, on the left, to what was passed
 * in ctor.
 */
Link* Connect::try_connect(Link* left_cset)
{
	assert(left_cset, "State word-connectorset is null");
	assert(2 == left_cset->get_arity(), "Wrong arity for state word conset");
	Atom* left = left_cset->get_outgoing_atom(1);

	Atom *right = _rcons;
cout<<"state con "<<left<<endl;
cout<<"word con "<<right<<endl;
	Node* rnode = dynamic_cast<Node*>(right);
	if (rnode)
	{
		Link* conn = conn_connect(left_cset, left, rnode);
		if (!conn)
			return NULL;
cout<<"got one it is "<<conn<<endl;
		// Unpack, and insert the words.
		OutList lwdj;
		lwdj.push_back(left_cset->get_outgoing_atom(0));
		lwdj.push_back(conn->get_outgoing_atom(1));
		Link *lwordj = new Link(WORD_DISJ, lwdj);

		OutList rwdj;
		rwdj.push_back(_right_cset->get_outgoing_atom(0));
		rwdj.push_back(conn->get_outgoing_atom(2));
		Link *rwordj = new Link(WORD_DISJ, rwdj);

		OutList lo;
		lo.push_bpack(conn->get_outgoing_atom(0));
		lo.push_back(lwordj);
		lo.push_back(rwordj);
		Link *lg_link = new Link (LINK, lo);

cout<<"normalized into "<<lg_link<<endl;
// XXX this is raw, need to put back the words.
		return lg_link;
	}
	return NULL;
}

/**
 * Dispatch appropriatly, depending on whether left atom is node or link
 */
Link* Connect::conn_connect(Link* left_cset, Atom *latom, Node* rnode)
{
	Node* lnode = dynamic_cast<Node*>(latom);
	if (lnode)
		return conn_connect(lnode, rnode);

	Link* llink = dynamic_cast<Link*>(latom);
	return conn_connect(left_cset, llink, rnode);
}

/**
 * Connect left_cset and _right_cset with an LG_LINK
 * lnode and rnode are the two connecters that actually mate.
 */
Link* Connect::conn_connect(Link* left_set, Node* lnode, Node* rnode)
{
	assert(lnode->get_type() == CONNECTOR, "Expecting connector on left");
	assert(rnode->get_type() == CONNECTOR, "Expecting connector on right");
cout<<"try mathc l="<<lnode->get_name()<<" to r="<< rnode->get_name() << endl;
	if (!conn_match(lnode->get_name(), rnode->get_name()))
		return NULL;
	
cout<<"Yayyyyae  nodes match!"<<endl;
	string link_name = conn_merge(lnode->get_name(), rnode->get_name());
	OutList linkage;
	linkage.push_back(new Node(LINK_TYPE, link_name));
	linkage.push_back(left_set);
	linkage.push_back(rnode);
	return new Link(LINK, linkage);
}

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

Link* Connect::conn_connect(Link* left_cset, Link* llink, Node* rnode)
{
cout<<"Enter recur l=" << llink->get_type()<<endl;

	for (int i = 0; i < llink->get_arity(); i++)
	{
		Atom *a = llink->get_outgoing_atom(i);
		Node* lnode = dynamic_cast<Node*>(a);
		if (lnode) 
		{
			if (lnode->get_name() == OPTIONAL_CLAUSE)
				continue;

			// Only one needs to be satisfied for OR clause
			if (OR == llink->get_type())
				return conn_connect(lnode, rnode);

			// If we are here, then its an AND. 
			assert(0, "Implement me");
		}

		Link* clink = dynamic_cast<Link*>(a);
		return conn_connect(left_cset, clink, rnode);
	}
	
	return NULL;
}

} // namespace viterbi
} // namespace link-grammar


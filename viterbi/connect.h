/*************************************************************************/
/* Copyright (c) 2012 Linas Vepstas <linasvepstas@gmail.com>             */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_CONNECT_H
#define _LG_VITERBI_CONNECT_H

#include "atom.h"
#include "compile.h"

namespace link_grammar {
namespace viterbi {

class Connect
{
	public:
		Connect(WordCset*);
		Set* try_connect(StatePair*);

	protected:
		Set* try_connect_a(StatePair*);
		Set* next_connect(WordCset*);

		Ling* conn_connect_nn(Connector*, Connector*);
		Ling* reassemble(Ling*, WordCset*, WordCset*);

		StatePair* alternative(Connector*, Connector*);
		StatePair* alternative(Connector*, And*);

		static const OutList& flatten(OutList&);

	private:
		WordCset* _right_cset;
		Atom* _rcons;  // just the connector(s), without the word.
		WordCset* _left_cset;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_CONNECT_H

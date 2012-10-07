/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
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
		Link* try_connect(WordCset*);

	protected:
		Link* reassemble(Link*, WordCset*, WordCset*);
		Set* reassemble(Set*, WordCset*, WordCset*);

		Link* conn_connect_aa(WordCset*, Atom*, Atom*);
		Link* conn_connect_na(WordCset*, Node*, Atom*);
		Link* conn_connect_ka(WordCset*, Link*, Atom*);

		Link* conn_connect_nn(WordCset*, Node*, Node*);
		Link* conn_connect_nk(WordCset*, Node*, Link*);

		static bool is_optional(Atom *);

	private:
		WordCset* _right_cset;
		Atom* _rcons;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_CONNECT_H

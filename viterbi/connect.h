/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_CONNECT_H
#define _LG_VITERBI_CONNECT_H

#include "atom.h"

namespace link_grammar {
namespace viterbi {

class Connect
{
	public:
		Connect(Link*);
		Link* try_connect(Link*);

	protected:
		static Link* conn_connect(Node*, Node*);
		static Link* conn_connect(Link*, Node*);

	private:
		Link* _right_cset;
		Atom* _rcons;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_CONNECT_H

/*************************************************************************/
/* Copyright (c) 2012                                                    */
/* Linas Vepstas <linasvepstas@gmail.com>                                */
/* All rights reserved                                                   */
/*                                                                       */
/*************************************************************************/

#ifndef _LG_VITERBI_PARSER_H
#define _LG_VITERBI_PARSER_H

#include <string>

#include "atom.h"
#include "api-types.h"
#include "structures.h"

namespace link_grammar {
namespace viterbi {

class Parser
{
	public:
		Parser(Dictionary dict);

		void streamin(const std::string&);
		void stream_word(const std::string&);
		void stream_word_conset(Link*);

		Link* word_consets(const std::string& word);
		Link* get_state_set();

	protected:
		void initialize_state();
		Atom* lg_exp_to_atom(Exp*);

		Dictionary _dict;
	private:
		Link *_state;
};


} // namespace viterbi
} // namespace link-grammar

#endif // _LG_VITERBI_PARSER_H

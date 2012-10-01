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

#include "utilities.h"

#include "connector-utils.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

/**
 * Compare two connector strings, see if they mate.
 * Return true if they do, else return false.
 * All upper-case letters must match exactly.
 * Lower case letters must match exactly, or must match wildcard '*'.
 * All strings are implicitly padded with an infinite number of
 * wild-cards on the right; thus, only need to compare against the
 * shorter of the * two strings.
 */
bool conn_match(const string& ls, const string& rs)
{
	char ldir = *ls.rbegin();
	char rdir = *rs.rbegin();
	assert (ldir == '+' or ldir == '-', "Bad word direction");
	assert (rdir == '+' or rdir == '-', "Bad word direction");

	// Direction signs must couple.
	if ('+' == ldir and '-' != rdir) return false;
	if ('-' == ldir and '+' != rdir) return false;

	// Captial letters must match. Wildcards match anything lower-case.
	string::const_iterator lp = ls.begin();
	string::const_iterator rp = rs.begin();
	size_t len = -1 + min(ls.size(), rs.size());
	while (0 < len)
	{
		if (*lp != *rp)
		{
			// All upper-case letters must match!
			if (isupper(*lp) or isupper(*rp)) return false;
			// Wild-card matches anything.
			if ('*' != *lp and '*' != *rp) return false;
		}
		lp++;
		rp++;
		len--;
	}

	return true;
}

/**
 * Merge two connector strings to create the linkage string.
 * It is assumed that the two connectors mate; no error checking is
 * done to detect if they don't. 
 * Example:   W*n+ merged with Wi*dy- gives Windy
 */
string conn_merge(const string& ls, const string& rs)
{
	string::const_iterator lp = ls.begin();
	string::const_iterator rp = rs.begin();
	size_t len = -1 + max(ls.size(), rs.size());
	string merger;
	merger.reserve(len);
	while (0 < len)
	{
		if (lp == ls.end())
			merger.push_back(*rp);
		else if (rp == rs.end())
			merger.push_back(*lp);
		else if ('*' != *lp and '+' != *lp and '-' != *lp)
			merger.push_back(*lp);
		else if ('+' != *rp and '-' != *rp)
			merger.push_back(*rp);
		else
			merger.push_back('*');
			
		if (lp != ls.end()) lp++;
		if (rp != rs.end()) rp++;
		len--;
	}
	return merger;
}

} // namespace viterbi
} // namespace link-grammar


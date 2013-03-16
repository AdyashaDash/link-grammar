/*************************************************************************/
/* Copyright (c) 2013 Linas Vepstas <linasvepstas@gmail.com>             */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the Viterbi parsing system is subject to the terms of the      */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include <gc/gc.h>
#include "garbage.h"

using namespace std;

namespace link_grammar {
namespace viterbi {

void lg_init_gc()
{
	GC_all_interior_pointers = 0;
	GC_init();

	/* Max heap size of a quarter-gig. */
	GC_set_max_heap_size(256*1024*1024);

}


} // namespace viterbi
} // namespace link-grammar

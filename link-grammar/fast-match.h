/*************************************************************************/
/* Copyright (c) 2004                                                    */
/* Daniel Sleator, David Temperley, and John Lafferty                    */
/* All rights reserved                                                   */
/*                                                                       */
/* Use of the link grammar parsing system is subject to the terms of the */
/* license set forth in the LICENSE file included with this software.    */
/* This license allows free redistribution and use in source and binary  */
/* forms, with or without modification, subject to certain conditions.   */
/*                                                                       */
/*************************************************************************/

#include "link-includes.h"
#include "structures.h"

match_context_t* alloc_fast_matcher(const Sentence, Resources);
void free_fast_matcher(match_context_t*);

void put_match_list(match_context_t *, Match_node *);
Match_node * form_match_list(match_context_t *, int, Connector *, int, Connector *, int);

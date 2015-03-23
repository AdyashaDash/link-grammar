/*
 * malloc-dbg.c
 *
 * Debug memory allocation.  Hacky code, used only 
 * for debugging.
 *
 * Copyright (c) 2008 Linas Vepstas <linas@linas.org>
 */

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <execinfo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/* ======================================================== */
void report_mem_dbg(void);

static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *, const void *);
static void *(*old_realloc_hook)(void *, size_t, const void *);

static void my_free_hook(void * mem, const void * caller);
static void * my_malloc_hook(size_t n_bytes, const void * caller);

#define TBLSZ 511366000
// #define TBLSZ 41366000
typedef struct {
	void * mem;
	const void * caller;
	size_t sz;
	int cnt;
	bool free;
} loc_t;

// static loc_t loc[TBLSZ];
static loc_t *loc;
static int mcnt = 0;

static FILE *fh = NULL;

static void init_io(void)
{
	if (fh) return;
	/* fh = fopen("/tmp/m", "w"); */
	fh = stdout;
}

// #define allprt(f, varargs...) fprintf(f, ##varargs)
#define allprt(f, varargs...) 

static void * my_realloc_hook(void * mem, size_t n_bytes, const void *caller)
{
	int i;
	for (i=0; i<TBLSZ; i++)
	{
		if (mem == loc[i].mem)
		{
#if TRACK_LEAK
			loc[i].mem = nm;
			loc[i].caller = (void *) -1;
			loc[i].sz = n_bytes;
#endif
			allprt(fh, "realloc free %d %p\n", i, mem);
			if (loc[i].free)
			{
				fprintf(fh, "Error realloc of prev free %d %p\n", i, mem);
			}
			loc[i].free = true;
			break;
		}
	}
	if (TBLSZ == i)
	{
		fprintf(fh, "Error: realloc address not previously alloed!\n");
	}

	__realloc_hook = old_realloc_hook;
	void * nm = realloc(mem, n_bytes);
	old_realloc_hook = __realloc_hook;
	__realloc_hook = my_realloc_hook;

#if TRACK_LEAK
	if (i < TBLSZ) return nm;
#endif
	if (i < TBLSZ)
	{
		loc[i].mem = nm;
		loc[i].caller = caller;
		loc[i].sz = n_bytes;
		mcnt++;
		loc[i].cnt = mcnt;
		loc[i].free = false;
		allprt(fh, "realloc reuse alloc %d %p\n", i, nm);
		return nm;
	}

	for (i=0; i<TBLSZ; i++)
	{
		if ((void *) -1 == loc[i].mem)
		{
			loc[i].mem = nm;
			loc[i].sz = n_bytes;
			loc[i].caller = caller;
			mcnt ++;
			loc[i].cnt = mcnt;
			loc[i].free = false;
			allprt(fh, "realloc new alloc %d %p\n", i, nm);
			break;
		}
	}
	if (i == TBLSZ) exit(1);
	return nm;
}

static void * my_malloc_hook(size_t n_bytes, const void * caller)
{
	__malloc_hook = old_malloc_hook;
	__free_hook = old_free_hook;
	void * mem = malloc(n_bytes);
	old_malloc_hook = __malloc_hook;
	old_free_hook = __free_hook;

	init_io();
	int i;
	for (i=0; i<TBLSZ; i++)
	{
		if (mem == loc[i].mem)
		{
			loc[i].mem = mem;
			loc[i].caller = caller;
			allprt(fh,"malloc reuse %d %p\n", i, mem);
			loc[i].sz = n_bytes;
			mcnt ++;
			loc[i].cnt = mcnt;
			loc[i].free = false;
			break;
		}
	}
	if (i == TBLSZ)
	{
		for (i=0; i<TBLSZ; i++)
		{
			if ((void *) -1 == loc[i].mem)
			{
				loc[i].mem = mem;
				loc[i].caller = caller;
				allprt(fh,"malloc fresh %d %p\n", i, mem);
				loc[i].sz = n_bytes;
				mcnt ++;
				loc[i].cnt = mcnt;
				loc[i].free = false;
				break;
			}
		}
	}

	/* This will be hit if there is a slow memory leak */
	if (i == TBLSZ)
	{
		size_t totsz=0;
#if REPORT
		for (i=0; i<TBLSZ; i++)
		{
		  /* size_t off = loc[i].caller - (void *) my_malloc_hook; */
			fprintf(fh, "%d caller=%p sz=%x %d\n",
			       i, loc[i].caller, loc[i].sz, loc[i].cnt);
			totsz += loc[i].sz;
		}
#endif
		fprintf (fh, "Total Size=%zu\n", totsz);
		*((int *)0) = 1; /* Force a crash, pop into debugger. */
		exit(1);
	}

	__malloc_hook = my_malloc_hook;
	__free_hook = my_free_hook;
	return mem;
}

static void my_free_hook(void * mem, const void * caller)
{
	if (0x0 == mem) return;
	__malloc_hook = old_malloc_hook;
	__free_hook = old_free_hook;
	int i;
	for (i=0; i<TBLSZ; i++)
	{
		if (mem == loc[i].mem)
		{
#if TRACK_LEAK
			loc[i].mem = (void *) -1;
			loc[i].caller = 0;
			loc[i].sz = 0;
			loc[i].cnt = -3;
#endif
			allprt(fh, "free %d %p\n", i, mem);
			if (loc[i].free)
			{
				fprintf(fh, "Error double free %d %p\n", i, mem);
			}
			loc[i].free = true;
			break;
		}
	}

	/* Its a double-free if the mem address wasn't found! */
#if TRACK_LEAK
	if (TBLSZ == i)
	{
		int i;
#define BTSZ 10
		void * btbuf[BTSZ];
		fprintf(fh, "Mem location %p is not alloced!\n", mem);
		int nt = backtrace (btbuf, BTSZ);
		char ** bt = backtrace_symbols(btbuf, nt);
		for (i=0; i<nt; i++)
		{
			fprintf(fh, "\t%s\n", bt[i]);
		}

		*((int *)0) = 1; /* Force a crash, pop into debugger. */
		exit(1);
	}
#endif

	__malloc_hook = my_malloc_hook;

	free(mem);
	old_free_hook = __free_hook;
	__free_hook = my_free_hook;
}

void report_mem_dbg(void)
{
	int i;
	int cnt = 0;
	size_t totsz = 0;
	for (i=0; i<TBLSZ; i++)
	{
		if(loc[i].mem != (void *)-1)
		{
			cnt++;
			totsz += loc[i].sz;
		}
	}
	fprintf (fh, "tbl slots used = %d alloc mem=%zu\n", cnt, totsz);
}

void my_init_hook(void);
void my_init_hook(void)
{
	static bool is_init = false;
	if (is_init) return;
	is_init = true;

	printf("init malloc trace hook loc_t size=%d mbytes=%d\n",
		sizeof(loc_t), (TBLSZ * sizeof(loc_t))/(1024*1024));

	loc = (loc_t*) malloc (TBLSZ * sizeof(loc_t));

	old_malloc_hook = __malloc_hook;
	__malloc_hook = my_malloc_hook;

	old_free_hook = __free_hook;
	__free_hook = my_free_hook;

	old_realloc_hook = __realloc_hook;
	__realloc_hook = my_realloc_hook;

	int i;
	for (i=0; i<TBLSZ; i++)
	{
		loc[i].mem = (void *) -1;
		loc[i].caller = 0;
		loc[i].sz = 0;
		loc[i].cnt = 0;
		loc[i].free = true;
	}
}

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) = my_init_hook;
#endif /*_MSC_VER && __MINGW32__*/

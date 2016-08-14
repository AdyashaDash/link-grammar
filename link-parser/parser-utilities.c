/***************************************************************************/
/* Copyright (c) 2016 Amir Plivatsky                                       */
/* Copyright (c) 2016 Linas Vepstas                                        */
/* All rights reserved                                                     */
/*                                                                         */
/* Use of the link grammar parsing system is subject to the terms of the   */
/* license set forth in the LICENSE file included with this software.      */
/* This license allows free redistribution and use in source and binary    */
/* forms, with or without modification, subject to certain conditions.     */
/*                                                                         */
/***************************************************************************/
#include "parser-utilities.h"

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
/**
 * Get a line from the console in UTF-8.
 * This function bypasses the code page conversion and reads Unicode
 * directly from the console.
 * @return An input line from the console in UTF-8 encoding.
 */
char *get_console_line(void)
{
	static HANDLE console_handle = NULL;
	wchar_t winbuf[MAX_INPUT];
	/* Worst-case: 4 bytes per UTF-8 char, 1 UTF-8 char per wchar_t char. */
	static char utf8inbuf[MAX_INPUT*4+1];

	if (NULL == console_handle)
	{
		console_handle = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ,
		                             NULL, OPEN_EXISTING, 0, NULL);
		if (!console_handle || (INVALID_HANDLE_VALUE == console_handle))
		{
			prt_error("CreateFileA CONIN$: Error %d.", GetLastError());
			return NULL;
		}
	}

	int nchar;
	if (!ReadConsoleW(console_handle, &winbuf, MAX_INPUT-sizeof(wchar_t), &nchar, NULL))
	{
		printf("ReadConsoleW: Error %d\n", GetLastError());
		return NULL;
	}
	winbuf[nchar] = L'\0';

	nchar = WideCharToMultiByte(CP_UTF8, 0, winbuf, -1, utf8inbuf,
	                            sizeof(utf8inbuf), NULL, NULL);
	if (0 == nchar)
	{
		prt_error("Error: WideCharToMultiByte CP_UTF8 failed: Error %d.",
		          GetLastError());
		return NULL;
	}

	/* Make sure we don't have conversion problems, by searching for '�'. */
	const char *invlid_char  = strstr(utf8inbuf, "\xEF\xBF\xBD");
	if (NULL != invlid_char)
		prt_error("Warning: Invalid input character encountered.");

	/* ^Z is read as a character. Convert it to an EOF indication. */
	if ('\x1A' == utf8inbuf[0]) /* Only handle it at line start. */
		return NULL;
	/* Note that nchar includes the terminating NUL. */
	if (3 <= nchar)                  /* Avoid a crash - just in case. */
		if ('\r' == utf8inbuf[nchar-3])
			utf8inbuf[nchar-3] = '\0';    /* Discard \r\n */

	return utf8inbuf;
}

static int console_input_cp;
static int console_output_cp;
static void restore_console_cp(void)
{
	SetConsoleCP(console_input_cp);
	SetConsoleOutputCP(console_output_cp);
}

static BOOL CtrlHandler(DWORD fdwCtrlType)
{
	if ((CTRL_C_EVENT == fdwCtrlType) || (CTRL_BREAK_EVENT  == fdwCtrlType))
	{
		fprintf(stderr, "Interrupt\n");
		restore_console_cp();
		exit(2);
	}
	return FALSE;
}

/**
 * Set the output conversion attributes for transparency.
 * This way UTF-8 output doesn't pass any conversion.
 */
void win32_set_utf8_output(void)
{
	if (-1 == _setmode(fileno(stdout), _O_BINARY))
	{
		prt_error("Warning: _setmode(fileno(stdout), _O_BINARY): %s.",
			strerror(errno));
	}

	console_input_cp = GetConsoleCP();
	console_output_cp = GetConsoleOutputCP();
	atexit(restore_console_cp);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		prt_error("Warning: Cannot not set code page restore handler.");
	}
	/* For file output. It is too late for output pipes.
	 * If output pipe is desired, one case set CP_UTF8 by the
	 * command "chcp 65001" before invoking link-parser. */
	if (!SetConsoleCP(CP_UTF8))
	{
		prt_error("Warning: Cannot set input codepage %d (error %d).",
			CP_UTF8, GetLastError());
	}
	/* For Console output. */
	if (!SetConsoleOutputCP(CP_UTF8))
	{
		prt_error("Warning: Cannot set output codepage %d (error %d).",
			CP_UTF8, GetLastError());
	}
}

#if 0/* error C2081: 'FILE_INFORMATION_CLASSX': name in formal parameter list illegal */
#include <stdio.h>
#include <io.h>

#include <errno.h>
#include <wchar.h>
#include <windows.h>
#include <winternl.h>

#ifndef __MINGW64_VERSION_MAJOR
/* MS winternl.h defines FILE_INFORMATION_CLASS, but with only a
	 different single member. */
enum FILE_INFORMATION_CLASSX
{
	FileNameInformation = 9
};

typedef struct _FILE_NAME_INFORMATION
{
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

NTSTATUS (NTAPI *pNtQueryInformationFile) (HANDLE, PIO_STATUS_BLOCK, PVOID,
						ULONG, FILE_INFORMATION_CLASSX);
#else
NTSTATUS (NTAPI *pNtQueryInformationFile) (HANDLE, PIO_STATUS_BLOCK, PVOID,
						ULONG, FILE_INFORMATION_CLASS);
#endif

/**
 * isatty() compatibility for running under Cygwin when compiling
 * using the Windows native C library.
 * WIN32 isatty() gives a wrong result (for Cygwin) on Cygwin's
 * pseudo-ttys, because they are implemented as Windows pipes.
 * Here is compatibility layer, originally sent to the Cygwin
 * discussion group by Corinna Vinschen, Cygwin Project Co-Leader.
 * See: https://www.cygwin.com/ml/cygwin/2012-11/msg00214.html
 */
int lg_isatty(int fd)
{
	HANDLE fh;
	NTSTATUS status;
	IO_STATUS_BLOCK io;
	long buf[66];  /* NAME_MAX + 1 + sizeof ULONG */
	PFILE_NAME_INFORMATION pfni = (PFILE_NAME_INFORMATION) buf;
	PWCHAR cp;

	/* First check using _isatty.

		 Note that this returns the wrong result for NUL, for instance!
		 Workaround is not to use _isatty at all, but rather GetFileType
		 plus object name checking. */
	if (_isatty (fd))
		return 1;

	/* Now fetch the underlying HANDLE. */
	fh = (HANDLE) _get_osfhandle (fd);
	if (!fh || fh == INVALID_HANDLE_VALUE)
		{
		  errno = EBADF;
		  return 0;
		}

	/* Must be a pipe. */
	if (GetFileType (fh) != FILE_TYPE_PIPE)
		goto no_tty;

	/* Calling the native NT function NtQueryInformationFile is required to
		 support pre-Vista systems.  If that's of no concern, Vista introduced
		 the GetFileInformationByHandleEx call with the FileNameInfo info class,
		 which can be used instead. */
	if (!pNtQueryInformationFile)
		{
			pNtQueryInformationFile = (NTSTATUS (NTAPI *)(HANDLE, PIO_STATUS_BLOCK,
							PVOID, ULONG, FILE_INFORMATION_CLASS))
						 GetProcAddress (GetModuleHandle ("ntdll.dll"),
								 "NtQueryInformationFile");
			if (!pNtQueryInformationFile)
				goto no_tty;
		}
	if (!NT_SUCCESS (pNtQueryInformationFile (fh, &io, pfni, sizeof buf,
						 FileNameInformation)))
		goto no_tty;

	/* The filename is not guaranteed to be NUL-terminated. */
	pfni->FileName[pfni->FileNameLength / sizeof (WCHAR)] = L'\0';

	/* Now check the name pattern.  The filename of a Cygwin pseudo tty pipe
		 looks like this:

			 \cygwin-%16llx-pty%d-{to,from}-master

		 %16llx is the hash of the Cygwin installation, (to support multiple
		 parallel installations), %d id the pseudo tty number, "to" or "from"
		 differs the pipe direction. "from" is a stdin, "to" a stdout-like
		 pipe. */
	cp = pfni->FileName;
	if (!wcsncmp (cp, L"\\cygwin-", 8)
			&& !wcsncmp (cp + 24, L"-pty", 4))
		{
			cp = wcschr (cp + 28, '-');
			if (!cp)
				goto no_tty;
			if (!wcscmp (cp, L"-from-master") || !wcscmp (cp, L"-to-master"))
				return 1;
		}
no_tty:
	errno = EINVAL;
	return 0;
}
#endif /* 0 */
#endif

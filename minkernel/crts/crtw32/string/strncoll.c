/***
*strncoll.c - Collate locale strings
*
*       Copyright (c) 1994-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*       Compares at most n characters of two strings.
*
*Revision History:
*       05-09-94  CFW   Created from strnicol.c.
*       05-26-94  CFW   If count is zero, return EQUAL.
*       09-06-94  CFW   Remove _INTL switch.
*       10-24-94  GJF   Sped up C locale, multi-thread case.
*       12-29-94  CFW   Merge non-Win32.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       10-30-95  GJF   Specify SORT_STRINGSORT to CompareString.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <errno.h>
#include <awint.h>
#endif /* _WIN32 */

/***
*int _strncoll() - Collate locale strings
*
*Purpose:
*	Compare two strings using the locale LC_COLLATE information.
*	Compares at most n characters of two strings.
*
*Entry:
*	const char *s1 = pointer to the first string
*	const char *s2 = pointer to the second string
*	size_t count - maximum number of characters to compare
*
*Exit:
*	Less than 0    = first string less than second string
*	0	       = strings are equal
*	Greater than 0 = first string greater than second string
*
*Exceptions:
*	_NLSCMPERROR    = error
*	errno = EINVAL
*
*******************************************************************************/

int __cdecl _strncoll (
	const char *_string1,
	const char *_string2,
	size_t count
	)
{
#if	defined(_WIN32) && !defined(_NTSUBSET_)

	int ret;
        int coll_codepage;
	WCHAR wcstmp[MAX_CP_LEN];
#if     defined(_CRTMT) && !defined(DLL_FOR_WIN32S)
        int local_lock_flag;
#endif

        if (!count)
            return 0;

	if (__lc_handle[LC_COLLATE] == _CLOCALEHANDLE) {
		return strncmp(_string1, _string2, count);
	}

	_lock_locale( local_lock_flag )

#if     defined(_CRTMT) && !defined(DLL_FOR_WIN32S)
	if (__lc_handle[LC_COLLATE] == _CLOCALEHANDLE) {
		_unlock_locale( local_lock_flag )
		return strncmp(_string1, _string2, count);
	}
#endif

        /*
         * Must use default code page for the LC_COLLATE category for
         * MB/WC conversion inside __crtxxx().
         */

	if (__crtGetLocaleInfoW(__lc_handle[LC_COLLATE], LOCALE_IDEFAULTCODEPAGE,
            wcstmp, MAX_CP_LEN, 0) == 0)
        	return _NLSCMPERROR;

	coll_codepage = (int)wcstol(wcstmp, NULL, 10);

	if (0 == (ret = __crtCompareStringA(__lc_handle[LC_COLLATE],
            SORT_STRINGSORT, _string1, count, _string2, count, 
            coll_codepage)))
            goto error_cleanup;

	_unlock_locale( local_lock_flag )
	return (ret - 2);

error_cleanup:
	_unlock_locale( local_lock_flag )
	errno = EINVAL;
	return _NLSCMPERROR;

#else

	return strncmp(_string1, _string2, count);

#endif
}

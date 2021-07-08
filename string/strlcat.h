#ifndef __STR_LCAT_H
#define __STR_LCAT_H
/*
#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: strlcat.c,v 1.2 1999/06/17 16:28:58 millert Exp $";
#endif // LIBC_SCCS and not lint 
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/libkern/strlcat.c 326271 2017-11-27 15:20:12Z pfg $");

#include <sys/types.h>
#include <sys/libkern.h>
*/

#ifndef __APPLE__

size_t strlcat(char *dst, const char *src, size_t siz);

#endif /* __APPLE__ */

#endif

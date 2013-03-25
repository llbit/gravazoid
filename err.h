#ifndef ARK_ERR_H
#define ARK_ERR_H

/* provide errx and err macros to replace glibc's corresponding functions */

#ifdef __WIN32__
#define err(eval, ...) \
	do {\
		fprintf(stderr, "%s: ", __FILE__);\
		fprintf(stderr, __VA_ARGS__);\
		fprintf(stderr, "\n");\
		exit(eval);\
	} while (0)

#define errx(eval, ...) err(eval, __VA_ARGS__)
#else
#include <err.h>
#endif

#endif

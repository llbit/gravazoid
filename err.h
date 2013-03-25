#ifndef ARK_ERR_H
#define ARK_ERR_H

/* provide errx and err macros to replace glibc's corresponding functions */

#ifdef __WIN32__
#define err(eval, fmt, ...) \
	do {\
		fprintf(stderr, "%s: ", __FILE__);\
		if (fmt)\
			fprintf(stderr, fmt, __VA_ARGS__);\
		fprintf(stderr, "\n");\
		exit(eval);\
	} while (0)

#define errx(eval, fmt, ...) \
	do {\
		fprintf(stderr, "%s: ", __FILE__);\
		if (fmt)\
			fprintf(stderr, fmt, __VA_ARGS__);\
		fprintf(stderr, "\n");\
		exit(eval);\
	} while (0)
#else
#include <err.h>
#endif

#endif

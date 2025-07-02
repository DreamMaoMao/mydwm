/* See LICENSE file for copyright and license details. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}

void
die(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

/*从窗口的tags元素变量中解析出当前窗口在哪个tag下面（1-32）*/
unsigned int get_tag_bit_position(unsigned int tags) {
	unsigned int i;
	for(i=0;i<=31;i++) {
		if (tags & 1 << i) {
			return i+1;
		}
	}
	return 0;
}
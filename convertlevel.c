#include "err.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#ifdef __WIN32__
/* index replacement for win32 */
static char* index(const char* str, int c) {
	char*	p = str;
	while (1) {
		if (*p == c) return p;
		if (*p == '\0') break;
		p += 1;
	}
	return NULL;
}
#endif

int main(int argc, char **argv) {
	FILE *f;
	int nblock, btype, x, z, width, height;
	char *fname, *levelname;
	char buf[128];
	int i;

	srand(0);

	if(argc != 2) errx(1, "Usage: %s levelfile", argv[0]);
	fname = argv[1];

	levelname = fname;
	while(index(levelname, '/')) levelname = index(levelname, '/') + 1;

	f = fopen(fname, "r");
	if(!f) err(1, "%s", fname);

	printf("uint8_t data_%s[] = {\n", levelname);

	if(!fgets(buf, sizeof(buf), f)) err(1, "%s: Empty file?", fname);
	if(1 == sscanf(buf, "%d", &nblock)) {
		if(nblock < 1 || nblock > 255) errx(1, "%s: Invalid block count", fname);

		printf("\t\t0x%02x,\n", nblock & 255);
		printf("\t\t0x%02x,\n", nblock >> 8);
		for(i = 0; i < nblock; i++) {
			if(!fgets(buf, sizeof(buf), f)) err(1, "%s: Unexpected EOF", fname);
			if(3 == sscanf(buf, "%d , %d , %d", &btype, &x, &z)) {
				printf("\t\t0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
					btype,
					rand() % 6,
					x,
					z);
			} else errx(1, "%s: Couldn't parse line", fname);
		}
	} else if(2 == sscanf(buf, "ascii %d %d", &width, &height)) {
		uint8_t buffer[65536 * 4];
		nblock = 0;

		for(z = 0; z < height; z++) {
			if(!fgets(buf, sizeof(buf), f)) err(1, "%s: Unexpected EOF", fname);
			for(x = 0; x < width; x++) {
				if(buf[x] != '.') {
					if(nblock >= 65535) errx(1, "%s: Too many blocks", fname);
					switch(buf[x]) {
						case '*':
							buffer[nblock * 4 + 0] = 1;
							buffer[nblock * 4 + 1] = 0;
							break;
						case 'R':
							buffer[nblock * 4 + 0] = 1;
							buffer[nblock * 4 + 1] = rand() % 6;
							break;
						default:
							buffer[nblock * 4 + 0] = 0;
							buffer[nblock * 4 + 1] = buf[x] - '1';
							break;
					}
					buffer[nblock * 4 + 2] = x * 16;
					buffer[nblock * 4 + 3] = (height - 1 - z) * 16;
					nblock++;
				}
			}
		}

		printf("\t\t0x%02x,\n", nblock & 255);
		printf("\t\t0x%02x,\n", nblock >> 8);
		for(i = 0; i < nblock; i++) {
			printf("\t\t0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
				buffer[i * 4 + 0],
				buffer[i * 4 + 1],
				buffer[i * 4 + 2],
				buffer[i * 4 + 3]);
		}
	} else errx(1, "%s: Unsupported high-level format", fname);

	printf("};\n");

	return 0;
}

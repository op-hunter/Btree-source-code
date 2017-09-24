#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>

uint64_t myrandom(uint64_t modulo) {
uint64_t ans = 0;

	ans |= rand() % 32768;
	ans <<= 15;
	ans |= rand() % 32768;

	if (modulo >> 30) {
		ans <<= 15;
		ans |= rand() % 32768;
	}

	return ans % modulo;
}

char usage[] = "usage: %s type filename reps megs [upd]\n"
	"	where type is:\n"
	"		c - use full file memory map\n"
	"		d - use disk read/writes for leaves\n"
	"	reps is the number of random pages to process\n"
	"	filename is the name of the test disk file\n"
	"	megs is the size of the test file in megabytes\n"
	"	upd is number of buffered leaf updates\n\n";
	
int main (int argc, char **argv) {
int fd = open (argv[2], O_CREAT | O_RDWR, 0666);
uint64_t size = 1024LL * 1024LL, off;
int cnt = atoi(argv[3]), i, j, k;
int scale = atoi(argv[4]);
int upd = atoi(argv[5]);
char *buff, *map;
int sum = 0;

	if (argc < 2) {
		fprintf (stderr, usage, argv[0]);
		exit(1);
	}

	off = 0;
	size *= scale;
	buff = calloc (1024, 1024);

	if (lseek(fd, 0L, 2) < size)
	  while (off < size)
		pwrite (fd, buff, 1024 * 1024, off), off += 1024 * 1024;

	if(argv[1][0] == 'c') {
		map = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);

		if (map == MAP_FAILED) {
			printf("mmap failed, errno = %d\n", errno);
			exit(1);
		}
	}

	for (i = 0; i < cnt; i++) {
		off = myrandom(size - 262144) & ~0xfffLL;

		// simulate in-memory operation on interior or leaf node

		if (i % upd) {
		  for (j = 0; j < 18; j++)
			sum += map[off + myrandom(262144)];

		  continue;
		}

		// simulate leaf level disk operation

		switch(argv[1][0]) {
		case 'c':
			madvise(map + off, 262144, MADV_WILLNEED);

			for(k = 0; k < upd; k++)
			 for(j = 0; j < 18; j++)
			  sum += buff[myrandom(262144)];

			madvise(map + off, 262144, MADV_DONTNEED);
			break;

		case 'd':
			j = pread (fd, buff, 262144, off);

			if (j < 262144) {
			  printf("pread failed, errno = %d offset = %" PRIx64 "len = %d\n", errno, off, j);
			  exit(1);
			}

			for(k = 0; k < upd; k++)
			 for(j = 0; j < 18; j++)
			  sum += buff[myrandom(262144)];

			continue;
		}
	}

	printf("sum %d\n", sum);
	return 0;
}

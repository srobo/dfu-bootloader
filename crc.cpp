#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <boost/crc.hpp>

void
usage()
{
	fprintf(stderr, "Usage: crctool [-w] [-S start_offset] filename\n");
	exit(1);
}

int
main(int argc, char * const*argv)
{
	struct stat fstat;
	FILE *foo;
	void *data;
	uint32_t result;
	int ret;
	bool write = false;
	int start = 0;

	do {
		ret = getopt(argc, argv, "wS:");
		switch (ret) {
		case '?':
			usage();
			break;
		case 'w':
			write = true;
			break;
		case 'S':
			start = atoi(optarg);
			break;
		}
	} while (ret != -1);

	if (optind != argc - 1)
		usage();

	ret = stat(argv[optind], &fstat);
	if (ret < 0) {
		fprintf(stderr, "Could not stat %s", argv[optind]);
		perror("");
		exit(1);
	}

	foo = fopen(argv[optind], "r+b");
	if (foo == NULL) {
		perror("Could not open input file");
		exit(1);
	}

	// Skip past first 'start' bytes.
	fseek(foo, start, SEEK_SET);
	int size = fstat.st_size - start;

	data = malloc(size);
	if (data == NULL) {
		fprintf(stderr, "OOM\n");
		exit(1);
	}

	if (fread(data, size, 1, foo) != 1) {
		fprintf(stderr, "Could not read whole input file\n");
		exit(1);
	}

	// Flip all 32 bit words. Really.
	{
		uint32_t *ptr = (uint32_t*)data;
		unsigned long count = size / 4;
		assert((size % 4) == 0);

		for (unsigned long i = 0; i < count; i++)
			// XXX: baked in assumption that this'll actually swap
			// bytes around.
			ptr[i] = htonl(ptr[i]);
	}

	boost::crc_basic<32> crc32(0x04C11DB7, 0xFFFFFFFF, 0, false, false);
	crc32.process_bytes(data, size);
	result = crc32.checksum();
	free(data);

	if (write) {
		fseek(foo, start+8, SEEK_SET);
		fwrite(&result, sizeof(result), 1, foo);
	} else {
		printf("%X\n", result);
	}

	fclose(foo);
	return 0;
}

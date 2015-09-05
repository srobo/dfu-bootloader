#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <boost/crc.hpp>

void
usage()
{
	fprintf(stderr, "Usage: crctool [-k] filename\n");
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
	
	ret = getopt(argc, argv, "w");
	if (ret == '?')
		usage();
	else if (ret == 'w')
		write = true;

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

	data = malloc(fstat.st_size);
	if (data == NULL) {
		fprintf(stderr, "OOM\n");
		exit(1);
	}

	if (fread(data, fstat.st_size, 1, foo) != 1) {
		fprintf(stderr, "Could not read whole input file\n");
		exit(1);
	}

	boost::crc_basic<32> crc32(0x04C11DB7, 0xFFFFFFFF, 0, false, false);
	crc32.process_bytes(data, fstat.st_size);
	result = crc32.checksum();
	free(data);

	if (write) {
		fseek(foo, 8, SEEK_SET);
		fwrite(&result, sizeof(result), 1, foo);
	} else {
		printf("%X\n", result);
	}

	fclose(foo);
	return 0;
}

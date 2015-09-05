#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <boost/crc.hpp>

int
main(int argc, const char **argv)
{
	struct stat fstat;
	int ret;
	FILE *foo;
	void *data;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: crc filename\n");
		exit(1);
	}

	ret = stat(argv[1], &fstat);
	if (ret < 0) {
		fprintf(stderr, "Could not stat %s", argv[1]);
		perror("");
		exit(1);
	}

	foo = fopen(argv[1], "rb");
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

	fclose(foo);

	boost::crc_basic<32> crc32(0x04C11DB7, 0xFFFFFFFF, 0, false, false);
	crc32.process_bytes(data, fstat.st_size);
	printf("%X\n", crc32.checksum());

	free(data);
	return 0;
}

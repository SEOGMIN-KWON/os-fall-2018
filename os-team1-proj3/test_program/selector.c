#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <gmp.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "/root/integer"

static mpz_t one;

int main(int argc, char **argv)
{
	mpz_init_set_str(one, "1", 10);
	mpz_t counter;

	//parameter check
	if (argc != 2) {
		printf("Usage: %s <Integer>\n", argv[0]);
		return 0;
	}

	for (int i = 0; argv[1][i] != '\0'; ++i) {
		if (!isdigit(argv[1][i])){ 
			printf("Usage: %s <Integer>\n", argv[0]);
			return 0;
		}
	}

	mpz_init_set_str(counter, argv[1], 10);

	const char *filename = FILENAME;

	setvbuf(stdout, NULL, _IONBF, 0);

	while (1) {
		//first, take the write lock for when the device is positioned at [0, 180]
		syscall(382, 90, 90); //sys_rotlock_write

		//second, it writes the integer from the argument to a file /root/integer
		FILE *integer_file = NULL;
		integer_file = fopen(filename, "w"); //overwrite the content of the file mode "w"
		char *buf;
		buf = mpz_get_str(NULL, 10, counter);
		fprintf(integer_file, "%s", buf);

		//third, after the integer has been written, output the integer to standard output, and close the file
		printf("selector: %s\n", buf);
		fclose(integer_file);

		//fourth, release write lock
		syscall(385, 90, 90); //sys_rotunlock_write

		//fifth, increase integer and keep repeating the loop to reacquire the lock again
		mpz_add(counter, counter, one); //void mpz_add(MP_INT *sum, MP_INT *addend1, MP_INT *addend2)
		free(buf);
	}
	return 0;
}

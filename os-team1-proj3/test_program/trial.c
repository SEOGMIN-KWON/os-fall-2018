#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <gmp.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>
#include "prime.h"

#define FILENAME "/root/integer"

static mpz_t one;
static mpz_t two;
static void find_factors(mpz_t base);
static char *read_line(FILE* input);

int main(int argc, const char *argv[])
{
	//parameter check
	if (argc != 2) {
		printf("Usage: %s <Identifier>\n", argv[0]);
		return 0;
	}

	for (int i = 0; argv[1][i] != '\0'; ++i) {
		if (!isdigit(argv[1][i])){ 
			printf("Invalid Identifier\n");
			return 0;
		}
	}

	setvbuf(stdout, NULL, _IONBF, 0);

	mpz_t integer;
	mpz_t largenum;

	mpz_init_set_str(one, "1", 10);
	mpz_init_set_str(two, "2", 10);
	mpz_init_set_d(integer, 2020);

	mpz_init(largenum);
	mpz_pow_ui(largenum, two, 20);

	while (1) {
		//first, take the read lock for when the device is positioned at [0, 180]
		syscall(381, 90, 90); //sys_rotlock_read

		//second, after taking the lock, it will open the file called integer
		const char *filename = FILENAME;
		char *read_number = NULL;

		FILE *pfile = fopen(filename, "r");
		if (pfile == NULL) {
			syscall(383, 90, 90); //sys_rotunlock_read
			continue;
		}
		if ((read_number = read_line(pfile)) != NULL) {
			mpz_init_set_str(integer, read_number, 10);
			free(read_number);
		}
		else {
			syscall(383, 90, 90); //sys_rotunlock_read
			continue;
		}

		//third, calculate the prime number factorization of the integer, and write the result to the standard output
		printf("trial-%s", argv[1]);
		find_factors(integer);

		//fourth, it will close the file
		fclose(pfile);

		//fifth, release the read lock
		syscall(383, 90, 90); //sys_rotunlock_read
	}
	return 0;
}

static void find_factors(mpz_t base)
{
	char *str;
	int res;
	mpz_t i;
	mpz_t half;
	mpz_t temp;
	int is_first = 1;

	mpz_init_set_str(i, "2", 10);
	mpz_init(half);
	mpz_init(temp);
	mpz_cdiv_q(half, base, two);
	mpz_cdiv_q(temp, base, one);

	str = mpz_to_str(base);
	if (!str)
		return;

	res = mpz_probab_prime_p(base, 10);
	if (res) {
		printf(": %s = Prime Number\n", str);
		free(str);
		return;
	}

	printf(": %s = ", str);
	free(str);
	do {
		if (mpz_divisible_p(temp, i) && verify_is_prime(i)) {
			str = mpz_to_str(i);
			if (!str){
				return;
			}
			else if(is_first == 1) {
				printf("%s", str);
				free(str);
				is_first = 0;
			}
			else{
				printf(" * %s", str);
				free(str);	
			}
			mpz_cdiv_q(temp, temp, i);
		}
		
		if(!mpz_divisible_p(temp, i) || !verify_is_prime(i))
			mpz_nextprime(i, i);
	} while (mpz_cmp(temp, one) > 0);
	printf("\n");
}

static char *read_line(FILE* input)
{
	char c = '\0';
	char *line = (char *) calloc(1, sizeof(char));

	if (line == NULL)
		return NULL;

	int i = 0;
	int string_size = 1;

	while (fscanf(input, "%c", &c) > 0 && c != '\n') {
		++string_size;
		int new_buffersize = sizeof(char) * (string_size);
		line = realloc(line, new_buffersize);
		if (line == NULL)
			return NULL;
		
		line[i] = c;
		line[i + 1] = '\0';
		++i;
	}
	if (c == '\0') 
		return NULL;
	
	return line;
}

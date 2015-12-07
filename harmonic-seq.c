#include <stdint.h>
#include <stdio.h>
#include <math.h>

const uint64_t n = 1000 * 1000 * 1000 * 5ULL;
const double gm = 0.5772156649015328606065120900824024310421;

int main()
{
	double sum1 = 0;
	double sum2 = 0;
/*	
	for (uint64_t i = 1; i < n; i++) {
		sum1 += (1.0 / (double)i);
		sum2 = gm + log(i);
		
		if (i % 10000000 == 0) {
			printf("%llu %f %f %f\n", i, sum1, sum2, sum2-sum1);
		}
	}
*/
	for (uint64_t i = n; i < n*200; i += 1000) {
		sum2 = gm + log(i);
		
		if (i % 10000000 == 0) {
			printf("%lluB %f \n", i / 1000000000, sum2);
		}
	}

}

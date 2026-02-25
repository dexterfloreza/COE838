#include "fdct.h"

void fdct :: calculate_dct( void ) {
	unsigned char 	u, v, x, y;
	double 	temp;

	// do forward discrete cosine transform
	for (u = 0; u < 8; u++)
		for (v = 0; v < 8; v++) {
			temp = 0.0;
			for (x = 0; x < 8; x++)
				for (y = 0; y < 8; y++)
					temp += input_data[x][y] * fcosine[x][u].read() * fcosine[y][v].read();
			if ((u == 0) && (v == 0))
				temp /= 8.0;
			else if (((u == 0) && (v != 0)) || ((u != 0) && (v == 0)))
				temp /= (4.0*sqrt(2.0));
			else
				temp /= 4.0;
			out64[u][v].write(temp);
		}

	printf(".");
}

void fdct :: read_data( void ) {
	// read the 8x8 block as a continuous 64 values and store them in
	// input_data as an 8x8 block
	fread(input_data, 1, 64, sc_input.read());
	// shift the unsigned integers from range [0, 2^8 - 1] to signed integers
	// of range [2^(8-1), 2^(8-1) - 1]
	for (unsigned char uv = 0; uv < 64; uv++)
		input_data[uv/8][uv%8] -= (char) (pow(2,8-1));
}

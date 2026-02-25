#include "functions.h"

#define rnd(x) (((x)>=0)?((signed char)((signed char)((x)+1.5)-1)):((signed char)((signed char)((x)-1.5)+1)))
#define rnd2(x) (((x)>=0)?((short int)((short int)((x)+1.5)-1)):((short int)((short int)((x)-1.5)+1)))
#define PI 3.1415926535897932384626433832795 // the value of PI

// the end of block marker, something which is highly unlikely to be found in a dct block
signed char MARKER = 127;

// quantization table
unsigned char quant[8][8] =
		{{16,11,10,16,24,40,51,61},
		 {12,12,14,19,26,58,60,55},
		 {14,13,16,24,40,57,69,56},
		 {14,17,22,29,51,87,80,62},
		 {18,22,37,56,68,109,103,77},
		 {24,35,55,64,81,104,113,92},
		 {49,64,78,87,103,121,120,101},
		 {72,92,95,98,112,100,103,99}};

// zigzag table
unsigned char zigzag_tbl[64] = {
		0,1,5,6,14,15,27,28,
		2,4,7,13,16,26,29,42,
		3,8,12,17,25,30,41,43,
		9,11,18,24,31,40,44,53,
		10,19,23,32,39,45,52,54,
		20,22,33,38,46,51,55,60,
		21,34,37,47,50,56,59,61,
		35,36,48,49,57,58,62,63};


void write_read_header(FILE *in, FILE *out) {
	unsigned char temp[60]; // temporary array of 60 characters, which is enough
							// for the bitmap header, which is 54 bytes
	printf("\nInput Header read and written to the output file");
	fread(temp, 1, 54, in);	// read 54 bytes from the input file and store them in temp
	fwrite(temp, 1, 54, out);	// write the 54 bytes to the output file
	printf("......Done\n");
	printf("Image is a %d bit Image. Press Enter to Continue\n>", temp[28]);
	getchar();
}


void make_cosine_tbl(double cosine[8][8]) {
	printf("Creating the cosine table for use in FDCT and IDCT");
	// calculate the cosine table as defined in the formula
	for (unsigned char i = 0; i < 8; i++)
		for (unsigned char j = 0; j < 8; j++)
			cosine[i][j] = cos((((2*i)+1)*j*PI)/16);
	printf("......Done\n");
}


void zigzag_quant(double data[8][8], FILE *output) {
	signed char to_write[8][8], final_write[8][8]; // this is the rounded values, to be written to the file
	char last_non_zero_value = 0; // this is the index to the last non-zero value in a block

	//quantization calculation with rounding (rnd) to nearest integer
	for (unsigned char i = 0; i < 8; i++) {
		for(unsigned char j = 0; j < 8; j++){
			to_write[i][j] = rnd(data[i][j] / quant[i][j]);
		}
	}

	// zigzag the data array and copy it back to final_write array
	//find out the index to the last non-zero value in the 8x8 block
	for (unsigned char i = 0; i < 64; i++) {
		final_write[zigzag_tbl[i]/8][zigzag_tbl[i]%8] = to_write[i/8][i%8];
			if (final_write[i/8][i%8] != 0)
				last_non_zero_value = i;
	}

	// write all the values in the block up to and including the last non-zero value
	for (unsigned char i = 0; i <= last_non_zero_value; i++)
		fwrite(&final_write[i/8][i%8], sizeof(signed char), 1, output);

	// write the end of block marker
	fwrite(&MARKER, sizeof(signed char), 1, output);
}


void unzigzag_iquant(double data[8][8], FILE *input) {
	signed char to_read[8][8]; // this is the data just read from the input file
	signed char temp_value = 0; // this is just the temporary value being read from the file

	// set all the values in the array to zeroes
	for (unsigned char i = 0; i < 64; i++)
		to_read[i/8][i%8] = 0;

	// read from file byte by byte, compare with the marker, and if it is not the marker
	// and it is not the end of file, read the next byte and store it in the array
	// keep doing this until the end of block is reached
	fread(&temp_value, sizeof(signed char), 1, input);

	for (unsigned char i = 0; (!(feof(input))) && temp_value != MARKER;
			i++, fread(&temp_value, sizeof(signed char), 1, input))
				to_read[i/8][i%8] = temp_value;

	// unzigzag the temporary array and copy it back to data
	for (unsigned char i = 0; i < 64; i++)
		data[i/8][i%8] = (double) to_read[zigzag_tbl[i]/8][zigzag_tbl[i]%8] * quant[i/8][i%8];
}

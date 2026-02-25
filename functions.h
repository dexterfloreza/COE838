#include <stdio.h>
#include <math.h>

// read the header of the bitmap and write it to the output file
void write_read_header(FILE *in, FILE *out);

// make the cosine table
void make_cosine_tbl(double cosine[8][8]);

// zigzag the quantized input data
void zigzag_quant(double data[8][8], FILE *output);

// unzigzag the zigzagged input data
void unzigzag_iquant(double data[8][8], FILE *input);

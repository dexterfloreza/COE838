#ifndef ARRAY_MULTIPLIER_8X8_H
#define ARRAY_MULTIPLIER_8X8_H

#include <systemc.h>
#include "csa.h"

SC_MODULE(array_multiplier_8x8) {

    // Ports
    sc_in<sc_uint<8>>  A;
    sc_in<sc_uint<8>>  B;
    sc_out<sc_uint<16>> P;

    // Internal signals
    sc_signal<bool> a[8];
    sc_signal<bool> b[8];

    sc_signal<bool> sum[8][8];
    sc_signal<bool> carry[8][8];

    sc_signal<bool> zero;   // shared constant 0

    csa* csa_mat[8][8];

    // Split input vectors into bits
    void split_inputs() {
        for (int i = 0; i < 8; i++) {
            a[i].write(A.read()[i]);
            b[i].write(B.read()[i]);
        }
    }

    // Collect product bits
    void collect_output() {
        sc_uint<16> prod = 0;

        // Lower bits from first column
        for (int i = 0; i < 8; i++)
            prod[i] = sum[i][0].read();

        // Upper bits from last row
        for (int i = 0; i < 8; i++)
            prod[i + 8] = sum[7][i].read();

        P.write(prod);
    }

    SC_CTOR(array_multiplier_8x8) {

        zero.write(false);

        // CSA matrix
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {

                csa_mat[r][c] = new csa(
                    ("csa_" + std::to_string(r) + "_" + std::to_string(c)).c_str()
                );

                csa_mat[r][c]->A(a[c]);
                csa_mat[r][c]->B(b[r]);

                // Sum input
                if (r == 0)
                    csa_mat[r][c]->Sin(zero);
                else
                    csa_mat[r][c]->Sin(sum[r - 1][c]);

                // Carry input
                if (c == 0)
                    csa_mat[r][c]->Cin(zero);
                else
                    csa_mat[r][c]->Cin(carry[r][c - 1]);

                csa_mat[r][c]->Sout(sum[r][c]);
                csa_mat[r][c]->Cout(carry[r][c]);
            }
        }

        SC_METHOD(split_inputs);
        sensitive << A << B;

        SC_METHOD(collect_output);
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                sensitive << sum[r][c];
    }

    ~array_multiplier_8x8() {
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                delete csa_mat[r][c];
    }
};

#endif

#ifndef TOP_H
#define TOP_H

#include <systemc.h>
#include "array_multiplier_8x8.h"

SC_MODULE(top) {
    sc_signal<sc_uint<8>> A;
    sc_signal<sc_uint<8>> B;
    sc_signal<sc_uint<16>> P;

    array_multiplier_8x8 dut;

    SC_CTOR(top) : dut("array_multiplier") {
        dut.A(A);
        dut.B(B);
        dut.P(P);
    }
};

#endif

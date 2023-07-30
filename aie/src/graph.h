
#include <adf.h>
#include "aie_kernels.h"

#ifndef AIE_KERNEL_NUMBER
#define AIE_KERNEL_NUMBER 3
#endif

using namespace adf;

template<int R=100>
class Filter2DBlock : public graph {
    private:
        kernel k;

    public:
        port<input> din;
        port<output> dout;

        Filter2DBlock() {
            k = kernel::create(filter2D);

            connect<window<8192>> int32_din (din, k.in[0]);
            connect<window<8192>> int32_dout(k.out[0], dout);

            source(k) = "aie_kernels/xf_filter2d.cpp";

            runtime<ratio>(k) = float(R/100.0);
        }
};

class TopGraph : public graph {
    public:
        static constexpr unsigned num_input = AIE_KERNEL_NUMBER, num_output = AIE_KERNEL_NUMBER;
        std::array<input_plio, num_input> in;
        std::array<output_plio, num_output> out;

        Filter2DBlock<100> F01;
        Filter2DBlock<100> F02;
        Filter2DBlock<100> F03;               
        TopGraph()
            : TopGraph({"DataIn0", "DataIn1", "DataIn2"},
                       {"data/input0.txt", "data/input1.txt", "data/input2.txt"}, 
                       {"DataOut0", "DataOut1", "DataOut2"}, 
                       {"data/output0.txt", "data/output1.txt", "data/output2.txt"}) {}



    private:
        TopGraph(const std::array<const char *, num_input> &input_names,
                 const std::array<const char *, num_input> &input_files,
                 const std::array<const char *, num_output> &output_names,
                 const std::array<const char *, num_output> &output_files) {
                    for (unsigned i = 0; i < in.size(); ++i) {
                        in[i] = input_plio::create(input_names[i], plio_32_bits, input_files[i]);
                    }
                    for (unsigned i = 0; i < out.size(); ++i) {
                        out[i] = output_plio::create(output_names[i], plio_32_bits, output_files[i]);
                    }

                    connect<> netlist[] = {
                        // input_plio[i].out[0] -> FXX.din -> FXX.dout -> output_plio[i].in[0]
                        {in[0].out[0],  F01.din}, {F01.dout, out[0].in[0]}, 
                        {in[1].out[0],  F02.din}, {F02.dout, out[1].in[0]}, 
                        {in[2].out[0],  F03.din}, {F03.dout, out[2].in[0]},
                    };
                }
};

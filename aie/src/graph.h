
#include <adf.h>
#include "aie_kernels.h"

#ifndef AIE_KERNEL_NUMBER
#define AIE_KERNEL_NUMBER 12
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
        Filter2DBlock<100> F04;
        Filter2DBlock<100> F05;
        Filter2DBlock<100> F06;
        Filter2DBlock<100> F07;
        Filter2DBlock<100> F08;
        Filter2DBlock<100> F09; 
        Filter2DBlock<100> F10;
        Filter2DBlock<100> F11;
        Filter2DBlock<100> F12;               
        TopGraph()
            : TopGraph({"DataIn0", "DataIn1", "DataIn2", "DataIn3", "DataIn4", "DataIn5", "DataIn6", "DataIn7", "DataIn8", "DataIn9", "DataIn10", "DataIn11"},
                       {"data/input1.txt", "data/input2.txt", "data/input3.txt", "data/input4.txt", "data/input5.txt", "data/input6.txt", "data/input7.txt", "data/input8.txt", "data/input9.txt", "data/input10.txt", "data/input11.txt", "data/input12.txt"}, 
                       {"DataOut0", "DataOut1", "DataOut2", "DataOut3", "DataOut4", "DataOut5", "DataOut6", "DataOut7", "DataOut8", "DataOut9", "DataOut10", "DataOut11"}, 
                       {"data/output1.txt", "data/output2.txt", "data/output3.txt", "data/output4.txt", "data/output5.txt", "data/output6.txt", "data/output7.txt", "data/output8.txt", "data/output9.txt", "data/output10.txt", "data/output11.txt", "data/output12.txt"}) {}




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
                        {in[0].out[0],  F01.din},  {F01.dout, out[0].in[0]}, 
                        {in[1].out[0],  F02.din},  {F02.dout, out[1].in[0]}, 
                        {in[2].out[0],  F03.din},  {F03.dout, out[2].in[0]}, 
                        {in[3].out[0],  F04.din},  {F04.dout, out[3].in[0]}, 
                        {in[4].out[0],  F05.din},  {F05.dout, out[4].in[0]}, 
                        {in[5].out[0],  F06.din},  {F06.dout, out[5].in[0]}, 
                        {in[6].out[0],  F07.din},  {F07.dout, out[6].in[0]}, 
                        {in[7].out[0],  F08.din},  {F08.dout, out[7].in[0]}, 
                        {in[8].out[0],  F09.din},  {F09.dout, out[8].in[0]},
                        {in[9].out[0],  F10.din},  {F10.dout, out[9].in[0]}, 
                        {in[10].out[0],  F11.din}, {F11.dout, out[10].in[0]}, 
                        {in[11].out[0],  F12.din}, {F12.dout, out[11].in[0]},
                    };
                }
};

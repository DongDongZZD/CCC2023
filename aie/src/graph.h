
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
        // Filter2DBlock<100> F13;
        // Filter2DBlock<100> F14;
        // Filter2DBlock<100> F15;
        // Filter2DBlock<100> F16;
        // Filter2DBlock<100> F17;
        // Filter2DBlock<100> F18;
        // Filter2DBlock<100> F19;
        // Filter2DBlock<100> F20;
        // Filter2DBlock<100> F21;
        // Filter2DBlock<100> F22;
        // Filter2DBlock<100> F23;
        // Filter2DBlock<100> F24;
        // Filter2DBlock<100> F25;
        // Filter2DBlock<100> F26;
        // Filter2DBlock<100> F27;
        // Filter2DBlock<100> F28;
        // Filter2DBlock<100> F29;
        // Filter2DBlock<100> F30;
        // Filter2DBlock<100> F31;
        // Filter2DBlock<100> F32;

        // TopGraph()
        //     : TopGraph({"DataIn1", "DataIn2", "DataIn3", "DataIn4", "DataIn5", "DataIn6", "DataIn7", "DataIn8",
        //                 "DataIn9", "DataIn10", "DataIn11", "DataIn12", "DataIn13", "DataIn14", "DataIn15", "DataIn16", 
        //                 "DataIn17", "DataIn18", "DataIn19", "DataIn20", "DataIn21", "DataIn22", "DataIn23", "DataIn24", 
        //                 "DataIn25", "DataIn26", "DataIn27", "DataIn28", "DataIn29", "DataIn30", "DataIn31", "DataIn32"},
        //                {"data/input1.txt", "data/input2.txt", "data/input3.txt", "data/input4.txt", "data/input5.txt", "data/input6.txt", "data/input7.txt", "data/input8.txt", 
        //                 "data/input9.txt", "data/input10.txt", "data/input11.txt", "data/input12.txt", "data/input13.txt", "data/input14.txt", "data/input15.txt", "data/input16.txt", 
        //                 "data/input17.txt", "data/input18.txt", "data/input19.txt", "data/input20.txt", "data/input21.txt", "data/input22.txt", "data/input23.txt", "data/input24.txt", 
        //                 "data/input25.txt", "data/input26.txt", "data/input27.txt", "data/input28.txt", "data/input29.txt", "data/input30.txt", "data/input31.txt", "data/input32.txt"}, 
        //                {"DataOut1", "DataOut2", "DataOut3", "DataOut4", "DataOut5", "DataOut6", "DataOut7", "DataOut8",
        //                 "DataOut9", "DataOut10", "DataOut11", "DataOut12", "DataOut13", "DataOut14", "DataOut15", "DataOut16", 
        //                 "DataOut17", "DataOut18", "DataOut19", "DataOut20", "DataOut21", "DataOut22", "DataOut23", "DataOut24", 
        //                 "DataOut25", "DataOut26", "DataOut27", "DataOut28", "DataOut29", "DataOut30", "DataOut31", "DataOut32"}, 
        //                {"data/output1.txt", "data/output2.txt", "data/output3.txt", "data/output4.txt", "data/output5.txt", "data/output6.txt", "data/output7.txt", "data/output8.txt", 
        //                 "data/output9.txt", "data/output10.txt", "data/output11.txt", "data/output12.txt", "data/output13.txt", "data/output14.txt", "data/output15.txt", "data/output16.txt", 
        //                 "data/output17.txt", "data/output18.txt", "data/output19.txt", "data/output20.txt", "data/output21.txt", "data/output22.txt", "data/output23.txt", "data/output24.txt", 
        //                 "data/output25.txt", "data/output26.txt", "data/output27.txt", "data/output28.txt", "data/output29.txt", "data/output30.txt", "data/output31.txt", "data/output32.txt"}) {}
        TopGraph()
            : TopGraph({"DataIn0", "DataIn1", "DataIn2", "DataIn3", "DataIn4", "DataIn5", "DataIn6", "DataIn7", "DataIn8",
                        "DataIn9", "DataIn10", "DataIn11"},
                       {"data/input0.txt", "data/input1.txt", "data/input2.txt", "data/input3.txt", "data/input4.txt", "data/input5.txt", "data/input6.txt", "data/input7.txt", "data/input8.txt", 
                        "data/input9.txt", "data/input10.txt", "data/input11.txt"}, 
                       {"DataOut0", "DataOut1", "DataOut2", "DataOut3", "DataOut4", "DataOut5", "DataOut6", "DataOut7", "DataOut8",
                        "DataOut9", "DataOut10", "DataOut11"}, 
                       {"data/output0.txt", "data/output1.txt", "data/output2.txt", "data/output3.txt", "data/output4.txt", "data/output5.txt", "data/output6.txt", "data/output7.txt", "data/output8.txt", 
                        "data/output9.txt", "data/output10.txt", "data/output11.txt"}) {}



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
                        {in[3].out[0],  F04.din}, {F04.dout, out[3].in[0]}, 
                        {in[4].out[0],  F05.din}, {F05.dout, out[4].in[0]}, 
                        {in[5].out[0],  F06.din}, {F06.dout, out[5].in[0]}, 
                        {in[6].out[0],  F07.din}, {F07.dout, out[6].in[0]}, 
                        {in[7].out[0],  F08.din}, {F08.dout, out[7].in[0]}, 
                        {in[8].out[0],  F09.din}, {F09.dout, out[8].in[0]}, 
                        {in[9].out[0],  F10.din}, {F10.dout, out[9].in[0]}, 
                        {in[10].out[0], F11.din}, {F11.dout, out[10].in[0]}, 
                        {in[11].out[0], F12.din}, {F12.dout, out[11].in[0]}, 
                        // {in[12].out[0], F13.din}, {F13.dout, out[12].in[0]}, 
                        // {in[13].out[0], F14.din}, {F14.dout, out[13].in[0]}, 
                        // {in[14].out[0], F15.din}, {F15.dout, out[14].in[0]}, 
                        // {in[15].out[0], F16.din}, {F16.dout, out[15].in[0]}, 
                        // {in[16].out[0], F17.din}, {F17.dout, out[16].in[0]}, 
                        // {in[17].out[0], F18.din}, {F18.dout, out[17].in[0]}, 
                        // {in[18].out[0], F19.din}, {F19.dout, out[18].in[0]}, 
                        // {in[19].out[0], F20.din}, {F20.dout, out[19].in[0]}, 
                        // {in[20].out[0], F21.din}, {F21.dout, out[20].in[0]}, 
                        // {in[21].out[0], F22.din}, {F22.dout, out[21].in[0]}, 
                        // {in[22].out[0], F23.din}, {F23.dout, out[22].in[0]}, 
                        // {in[23].out[0], F24.din}, {F24.dout, out[23].in[0]}, 
                        // {in[24].out[0], F25.din}, {F25.dout, out[24].in[0]}, 
                        // {in[25].out[0], F26.din}, {F26.dout, out[25].in[0]}, 
                        // {in[26].out[0], F27.din}, {F27.dout, out[26].in[0]}, 
                        // {in[27].out[0], F28.din}, {F28.dout, out[27].in[0]}, 
                        // {in[28].out[0], F29.din}, {F29.dout, out[28].in[0]}, 
                        // {in[29].out[0], F30.din}, {F30.dout, out[29].in[0]}, 
                        // {in[30].out[0], F31.din}, {F31.dout, out[30].in[0]}, 
                        // {in[31].out[0], F32.din}, {F32.dout, out[31].in[0]}, 
                    };
                }
};

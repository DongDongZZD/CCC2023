#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <typeinfo>
#include <math.h>

#include <xrt.h>
#include <experimental/xrt_kernel.h>

#define AIE_KERNEL_NUMBER 7

void cal_ref(int* input_buffer, unsigned width, unsigned height, int* kernel_coeff, int* ref_buffer);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    /////////////////////////////////////////////////
    // Get device index and download xclbin
    /////////////////////////////////////////////////
    std::cout << "Open the device" << std::endl;
    auto device = xrt::device(0);
    std::string binaryFile = argv[1];
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);

    /////////////////////////////////////////////////
    // Get reference to the kernels
    /////////////////////////////////////////////////
    std::cout << "Get references to compute units" << std::endl;
    auto tile_mm2mm_1    = xrt::kernel(device, uuid, "tile_mm2mm:{tile_mm2mm1}");
    auto sticker_mm2mm_1 = xrt::kernel(device, uuid, "sticker_mm2mm:{sticker_mm2mm1}");

    auto s2mm_0 = xrt::kernel(device, uuid, "s2mm:{s2mm1}");
    auto s2mm_1 = xrt::kernel(device, uuid, "s2mm:{s2mm2}");
    auto s2mm_2 = xrt::kernel(device, uuid, "s2mm:{s2mm3}");
    auto s2mm_3 = xrt::kernel(device, uuid, "s2mm:{s2mm4}");
    auto s2mm_4 = xrt::kernel(device, uuid, "s2mm:{s2mm5}");
    auto s2mm_5 = xrt::kernel(device, uuid, "s2mm:{s2mm6}");
    auto s2mm_6 = xrt::kernel(device, uuid, "s2mm:{s2mm7}");

    /////////////////////////////////////////////////
    // Allocating Buffer in Global Memory
    /////////////////////////////////////////////////
    std::cout << "Allocate Buffer in Global Memory" << std::endl;
    unsigned img_width   = 720;
    unsigned img_height  = 480;
    unsigned img_number  = 2;
    unsigned tile_width  = 64;
    unsigned tile_height = 32;
    
    // 所有 img 中的元素个数
    unsigned img_element_number  = img_width * img_height * img_number;
    // 单个 tile 中的元素个数
    unsigned tile_element_number = tile_width * tile_height;

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_width_number   = ceil((float)(img_width  - tile_width)  / (tile_width  - 2)) + 1;
	unsigned tile_height_number  = ceil((float)(img_height - tile_height) / (tile_height - 2)) + 1;

    // 每个 aie kernel 需要循环计算的总次数
    unsigned iteration = ceil((float)(tile_width_number * tile_height_number) / AIE_KERNEL_NUMBER) * img_number;
    
    // 所有输入图片的大小
    size_t img_size_in_bytes  = sizeof(int) * img_element_number;
    // 所有 tile 的大小
    size_t tile_size_in_bytes = sizeof(int) * tile_element_number * iteration;

    // 用来存储所有的图片
    // host mem ------> device mem (img_in_buffer)
    // 后续：img_in_buffer ---(PL:tile_mm2mm_1)---> aie kernel
    auto img_in_buffer = xrt::bo(device, img_size_in_bytes, tile_mm2mm_1.group_id(0));

    // 用于存储 aie kernel 的计算结果
    // aie kernel ---(PL:s2mm_)---> out_buffer_
    auto out_buffer_0 = xrt::bo(device, tile_size_in_bytes, s2mm_0.group_id(0));
    auto out_buffer_1 = xrt::bo(device, tile_size_in_bytes, s2mm_1.group_id(0));
    auto out_buffer_2 = xrt::bo(device, tile_size_in_bytes, s2mm_2.group_id(0));
    auto out_buffer_3 = xrt::bo(device, tile_size_in_bytes, s2mm_3.group_id(0));
    auto out_buffer_4 = xrt::bo(device, tile_size_in_bytes, s2mm_4.group_id(0));
    auto out_buffer_5 = xrt::bo(device, tile_size_in_bytes, s2mm_5.group_id(0));
    auto out_buffer_6 = xrt::bo(device, tile_size_in_bytes, s2mm_6.group_id(0));
    
    // 用来存储最后的计算结果
    // out_buffer_ ---(PL:sticker_mm2mm_1)---> img_out_buffer
    // 后续：device mem (img_out_buffer) ------> host mem
    auto img_out_buffer = xrt::bo(device, img_size_in_bytes, sticker_mm2mm_1.group_id(7));

    /////////////////////////////////////////////////
    // Read data from file 
    /////////////////////////////////////////////////
    std::cout << "Read data from file" << std::endl;
    auto *img_input      = new int [img_element_number];
    auto *img_output_aie = new int [img_element_number];
    auto *img_output_ref = new int [img_element_number];

    for (unsigned int i = 0; i < img_element_number; i++) {
        img_input[i] = rand() % 100;
    }

    /////////////////////////////////////////////////
    // Cal output reference
    /////////////////////////////////////////////////
    std::cout << "Cal output reference" << std::endl;
    int kernel_coeff[16] = {64, 128, 64, 128, 256, 128, 64, 128, 64};
    for (unsigned img_index = 0; img_index < img_number; img_index++) {
    	cal_ref(img_input + img_index * img_width * img_height, img_width, img_height, kernel_coeff, img_output_ref + img_index * img_width * img_height);
    }

    /////////////////////////////////////////////////
    // Write input data to device global memory
    /////////////////////////////////////////////////
    std::cout << "Write input data to device global memory" << std::endl;
    auto start = std::chrono::steady_clock::now();
    img_in_buffer.write(img_input);

    /////////////////////////////////////////////////
    // Synchronize input buffers data to device global memory
    /////////////////////////////////////////////////
    img_in_buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    auto end = std::chrono::steady_clock::now();
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Transfer data from host to device in nanoseconds: "
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        << " ns" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
 
    /////////////////////////////////////////////////
    // Execute the PL compute units
    /////////////////////////////////////////////////
    std::cout << "Run the PL kernels" << std::endl;

    start = std::chrono::steady_clock::now();
    auto run_s2mm_0 = s2mm_0(out_buffer_0, nullptr, tile_size_in_bytes);
    auto run_s2mm_1 = s2mm_1(out_buffer_1, nullptr, tile_size_in_bytes);
    auto run_s2mm_2 = s2mm_2(out_buffer_2, nullptr, tile_size_in_bytes);
    auto run_s2mm_3 = s2mm_3(out_buffer_3, nullptr, tile_size_in_bytes);
    auto run_s2mm_4 = s2mm_4(out_buffer_4, nullptr, tile_size_in_bytes);
    auto run_s2mm_5 = s2mm_5(out_buffer_5, nullptr, tile_size_in_bytes);
    auto run_s2mm_6 = s2mm_6(out_buffer_6, nullptr, tile_size_in_bytes);

    auto run_tile_mm2mm_1 = tile_mm2mm_1(
	    img_in_buffer, 
	    nullptr, nullptr, nullptr, nullptr, nullptr,
	    nullptr, nullptr);
    run_tile_mm2mm_1.wait();

    run_s2mm_0.wait();
    run_s2mm_1.wait();
    run_s2mm_2.wait();
    run_s2mm_3.wait();
    run_s2mm_4.wait();
    run_s2mm_5.wait();
    run_s2mm_6.wait();

    auto run_sticker_mm2mm_1 = sticker_mm2mm_1(
	    out_buffer_0, out_buffer_1, out_buffer_2, out_buffer_3, out_buffer_4,
	    out_buffer_5, out_buffer_6, 
	    img_out_buffer);
    run_sticker_mm2mm_1.wait();
    end = std::chrono::steady_clock::now();
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "PL & aie kernels complete in nanoseconds: "
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        << " ns" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;

    /////////////////////////////////////////////////
    // Synchronize the output buffer data from the device
    /////////////////////////////////////////////////
    std::cout << "Synchronize output buffers data to device global memory" << std::endl;
    start = std::chrono::steady_clock::now();
    img_out_buffer.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    /////////////////////////////////////////////////
    // Read output buffer data to local buffer
    /////////////////////////////////////////////////
    img_out_buffer.read(img_output_aie);
    end = std::chrono::steady_clock::now();
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Transfer data from device to host in nanoseconds: "
        << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
        << " ns" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;

    /////////////////////////////////////////////////
    // Correctness verification
    /////////////////////////////////////////////////
    std::cout << "Correctness verification" << std::endl;
    unsigned erro = 0;
    for (unsigned i = 0; i < img_element_number; i++) {
        if (abs(img_output_aie[i] - img_output_ref[i]) > 1e-3) {
            erro++;
        }
    }
    std::cout << "Erro time: " << erro << std::endl;

    /////////////////////////////////////////////////
    // Writing data to output file
    /////////////////////////////////////////////////
    std::cout << "Writing data to output file" << std::endl;
    std::ofstream outputfile;
    std::cout << "Ref Out1 " << std::to_string(img_output_ref[0]) << std::endl;
    std::cout << "AIE Out1 " << std::to_string(img_output_aie[0]) << std::endl;
    outputfile.open("build.hw/aie_hw_run_data/output.txt");
    for (unsigned i = 0; i < img_element_number; i++) {
        outputfile << img_output_aie[i] << std::endl;
    }
    outputfile.close();

    delete [] img_input;
    delete [] img_output_aie;
    delete [] img_output_ref;

    return 0;
}

void cal_ref(int* input_buffer, unsigned width, unsigned height, int* kernel_coeff, int* ref_buffer) {
    unsigned padding_width   = width + 2;
    unsigned padding_height  = height + 2;
    unsigned padding_elements_number = padding_width * padding_height;
    auto     *padding_buffer = new int [padding_elements_number];

    // 对输入图片进行 padding
    // 首先将 input_buffer 全部拷贝到 padding_buffer “中间“ 区域
    for (unsigned i = 0; i < height; i++) {
        for (unsigned j = 0; j < width; j++) {
            padding_buffer[(i + 1) * padding_width + (j + 1)] = input_buffer[i * width + j];
        }
    }

    // 对 padding_buffer 四个角的元素赋值 
    padding_buffer[0]                                    = padding_buffer[padding_width + 1];
    padding_buffer[padding_width - 1]                    = padding_buffer[2 * padding_width - 2];
    padding_buffer[(padding_height - 1) * padding_width] = padding_buffer[(padding_height - 2) * padding_width + 1];
    padding_buffer[padding_height * padding_width - 1]   = padding_buffer[(padding_height - 1) * padding_width - 2];

    // 对 padding_buffer 剩下的四条边进行赋值
    for (unsigned i = 1; i < padding_width - 1; i++) {
        padding_buffer[i] = padding_buffer[i + padding_width];
    }
    for (unsigned i = 1; i < padding_height - 1; i++) {
        padding_buffer[i * padding_width]           = padding_buffer[i * padding_width + 1];
        padding_buffer[(i + 1) * padding_width - 1] = padding_buffer[(i + 1) * padding_width - 2];
    }
    for (unsigned i = 1; i < padding_width - 1; i++) {
        padding_buffer[(padding_height - 1) * padding_width + i] = padding_buffer[(padding_height - 2) * padding_width + i];
    }

    // 对 padding 后的图片进行卷积运算
    unsigned width_loop  = padding_width - 2;
    unsigned height_loop = padding_height - 2;
    for (unsigned i = 0; i < height_loop; i++) {
        for (unsigned j = 0; j < width_loop; j++) {
            ref_buffer[i * width + j] = 0;
            for (unsigned x = 0; x < 3; x++) {
                for (unsigned y = 0; y < 3; y++) {
                    ref_buffer[i * width + j] += kernel_coeff[x * 3 + y] * padding_buffer[(i + x) * padding_width + j + y];
                }
            }
        }
    }

	delete [] padding_buffer;
}

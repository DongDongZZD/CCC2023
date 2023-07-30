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

#define AIE_KERNEL_NUMBER 12

void cal_ref(int* input_buffer, unsigned width, unsigned height, int* kernel_coeff, int* ref_buffer);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << " <IMAGE NUMBER>" << std::endl;
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
    auto tile_mm2s_1    = xrt::kernel(device, uuid, "tile_mm2s:{tile_mm2s1}");
    auto sticker_s2mm_1 = xrt::kernel(device, uuid, "sticker_s2mm:{sticker_s2mm1}");

    /////////////////////////////////////////////////
    // Allocating Buffer in Global Memory
    /////////////////////////////////////////////////
    std::cout << "Allocate Buffer in Global Memory" << std::endl;
    unsigned img_width   = 3840;
    unsigned img_height  = 2160;
    unsigned img_number  = strtoul(argv[2], NULL, 0);
    
    // 一张 img 中的元素个数
    unsigned img_element_number  = img_width * img_height;
    
    // 一张输入图片的拼接后的大小
    size_t img_buffer_size  = sizeof(int) * img_element_number;

    // 用来存储一张图片
    // host mem ------> device mem (img_in_buffer)
    // 后续：img_in_buffer ---(PL:tile_mm2s_1)---> aie kernel
    auto img_in_buffer = xrt::bo(device, img_buffer_size, tile_mm2s_1.group_id(0));
    
    // 用来存储最后的计算结果
    // aie kernel ---(PL:sticker_s2mm_1)---> img_out_buffer
    // 后续：device mem (img_out_buffer) ------> host mem
    auto img_out_buffer = xrt::bo(device, img_buffer_size, sticker_s2mm_1.group_id(12));

    /////////////////////////////////////////////////
    // Create buffer for running time
    /////////////////////////////////////////////////
    auto *img_trans_to_time     = new long [img_number];
    auto *img_trans_from_time   = new long [img_number];
    auto *img_execute_time      = new long [img_number];

    /////////////////////////////////////////////////
    // Generating data
    /////////////////////////////////////////////////
    auto *img_input      = new int [img_element_number];
    auto *img_output_aie = new int [img_element_number];
    auto *img_output_ref = new int [img_element_number];

    std::cout << "==========  START  ==========" << std::endl;
    std::cout << "IMG " << "0/" << img_number << " : " ;
    for (unsigned int i = 0; i < img_element_number; i++) {
        img_input[i] = rand() % 100;
    }

    /////////////////////////////////////////////////
    // Cal output reference
    /////////////////////////////////////////////////
    int kernel_coeff[16] = {64, 128, 64, 128, 256, 128, 64, 128, 64};
    cal_ref(img_input, img_width, img_height, kernel_coeff, img_output_ref);

    /////////////////////////////////////////////////
    // Write input data to device global memory
    /////////////////////////////////////////////////
    auto start = std::chrono::steady_clock::now();
    img_in_buffer.write(img_input);

    /////////////////////////////////////////////////
    // Synchronize input buffers data to device global memory
    /////////////////////////////////////////////////
    img_in_buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    auto end = std::chrono::steady_clock::now();
    img_trans_to_time[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    /////////////////////////////////////////////////
    // Execute the PL compute units
    /////////////////////////////////////////////////
    start = std::chrono::steady_clock::now();
    auto run_sticker_s2mm_1 = sticker_s2mm_1(
	    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	    img_out_buffer);

    auto run_tile_mm2s_1 = tile_mm2s_1(
	    img_in_buffer, 
	    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    run_tile_mm2s_1.wait();
    run_sticker_s2mm_1.wait();
    end = std::chrono::steady_clock::now();
    img_execute_time[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    /////////////////////////////////////////////////
    // Synchronize the output buffer data from the device
    ///////////////////////////////////////////////
    start = std::chrono::steady_clock::now();
    img_out_buffer.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // /////////////////////////////////////////////////
    // // Read output buffer data to local buffer
    // /////////////////////////////////////////////////
    img_out_buffer.read(img_output_aie);
    end = std::chrono::steady_clock::now();
    img_trans_from_time[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    /////////////////////////////////////////////////
    // Correctness verification
    /////////////////////////////////////////////////
    unsigned erro = 0;
    for (unsigned i = 0; i < img_element_number; i++) {
        if (abs(img_output_aie[i] - img_output_ref[i]) > 1e-3) {
            erro++;
        }
    }
    std::cout << "Erro time: " << erro << std::endl;

    for(unsigned id = 1; id < img_number; id++){

	    std::cout << "IMG " << id << "/" << img_number << " : " ;
	    for (unsigned int i = 0; i < img_element_number; i++) {
	        img_input[i] = rand() % 100;
	    }
	
	    /////////////////////////////////////////////////
	    // Cal output reference
	    /////////////////////////////////////////////////
        cal_ref(img_input, img_width, img_height, kernel_coeff, img_output_ref);
	
	    /////////////////////////////////////////////////
	    // Write input data to device global memory
	    /////////////////////////////////////////////////
	    auto start = std::chrono::steady_clock::now();
	    img_in_buffer.write(img_input);
	
	    /////////////////////////////////////////////////
	    // Synchronize input buffers data to device global memory
	    /////////////////////////////////////////////////
	    img_in_buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	    auto end = std::chrono::steady_clock::now();
        img_trans_to_time[id] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	
	    /////////////////////////////////////////////////
	    // Execute the PL compute units
	    /////////////////////////////////////////////////
	    start = std::chrono::steady_clock::now();

	    run_sticker_s2mm_1.start();
	    run_tile_mm2s_1.start();
	
	    run_tile_mm2s_1.wait();
	    run_sticker_s2mm_1.wait();
	    end = std::chrono::steady_clock::now();
        img_execute_time[id] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	
	    /////////////////////////////////////////////////
	    // Synchronize the output buffer data from the device
	    ///////////////////////////////////////////////
	    start = std::chrono::steady_clock::now();
	    img_out_buffer.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	
	    // /////////////////////////////////////////////////
	    // // Read output buffer data to local buffer
	    // /////////////////////////////////////////////////
	    img_out_buffer.read(img_output_aie);
	    end = std::chrono::steady_clock::now();
        img_trans_from_time[id] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	
	    /////////////////////////////////////////////////
	    // Correctness verification
	    /////////////////////////////////////////////////
	    unsigned erro = 0;
	    for (unsigned i = 0; i < img_element_number; i++) {
	        if (abs(img_output_aie[i] - img_output_ref[i]) > 1e-3) {
	            erro++;
	        }
	    }
	    std::cout << "Erro time: " << erro << std::endl;
    }
    std::cout << "==========  END  ==========" << std::endl;
	/////////////////////////////////////////////////
	// Process run time data
	/////////////////////////////////////////////////
    double average_trans_to_time = 0;
    double average_trans_from_time = 0;
    double average_execute_time = 0;
    std::sort(img_trans_to_time, img_trans_to_time + img_number);
    std::sort(img_trans_from_time, img_trans_from_time + img_number);
    std::sort(img_execute_time, img_execute_time + img_number);

    for (unsigned i = 1; i < img_number - 1; i++) {
        average_execute_time    += img_execute_time[i];
        average_trans_to_time   += img_trans_to_time[i];
        average_trans_from_time += img_trans_from_time[i];
    }
    average_execute_time    /= (img_number - 2);
    average_trans_to_time   /= (img_number - 2);
    average_trans_from_time /= (img_number - 2);

    std::cout << "********************************************************************************" << std::endl << std::endl;
    std::cout << "\tAverage transefer time from host TO   device: " << average_trans_to_time << "ns" << std::endl;
    std::cout << "\tAverage transefer time from host FROM device: " << average_trans_from_time << "ns" << std::endl;
    std::cout << "\tAverage execution time                      : " << average_execute_time << "ns" << std::endl << std::endl;
    std::cout << "********************************************************************************" << std::endl << std::endl;


    delete [] img_input;
    delete [] img_output_aie;
    delete [] img_output_ref;
    delete [] img_trans_to_time;
    delete [] img_trans_from_time;
    delete [] img_execute_time;

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

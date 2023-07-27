#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

// 对一张图片的指定位置进行 tile 操作
void tile_mm2mm(ap_int<DWIDTH> *mem_in, 
ap_int<DWIDTH> *mem_out1, ap_int<DWIDTH> *mem_out2, ap_int<DWIDTH> *mem_out3, ap_int<DWIDTH> *mem_out4, 
ap_int<DWIDTH> *mem_out5, ap_int<DWIDTH> *mem_out6, ap_int<DWIDTH> *mem_out7, ap_int<DWIDTH> *mem_out8, ap_int<DWIDTH> *mem_out9, ap_int<DWIDTH> *mem_out10,
ap_int<DWIDTH> *mem_out11, ap_int<DWIDTH> *mem_out12, ap_int<DWIDTH> *mem_out13, ap_int<DWIDTH> *mem_out14, ap_int<DWIDTH> *mem_out15) {


    // 计算一张图片有多少 tile
    unsigned tile_num_width  = ceil((float)(img_width - tile_width) / (tile_width - 2)) + 1;
    unsigned tile_num_height = ceil((float)(img_height - tile_height) / (tile_height - 2)) + 1;

    unsigned count[15] = {0};
    int mem_in_index;
    // unsigned mem_out_index;

    // 遍历所有图片
    for (unsigned i = 0; i < img_number; i++) {

        unsigned offset_img = i * img_width * img_height;

        // 遍历所有的 tile
        for (unsigned j = 0; j < tile_num_height; j++) {
            for (unsigned k = 0; k < tile_num_width; k++) {

                // 当前的 tile 应该传输给第 aie_index 个 aie kernel
                unsigned aie_index = (j * tile_num_width + k) % aie_kernel_num + 1;
                
                // 当前 tile 相对于第一个元素的偏移
                unsigned offset_width  = k * (tile_width - 2);
                unsigned offset_height = j * (tile_height - 2);

                // 遍历当前的 tile
                for (unsigned ti = 0; ti < tile_height; ti++) {
                    for (unsigned tj = 0; tj < tile_width; tj++) {
                        
                        // 判断是否遍历到图片边缘
                        bool edge_flag_width  = ((tj + offset_width) == (img_width));
                        bool edge_flag_height = ((ti + offset_height) == (img_height));

                        // 遍历到图片边缘后需要进行 padding 操作 
                        // mem_in_index == -1 表示补零
                        if (!edge_flag_height && !edge_flag_width)
                            mem_in_index = (ti + offset_height) * img_width + tj + offset_width + offset_img;                        
                        else if (edge_flag_height && !edge_flag_width)
                            mem_in_index = (img_height - 1) * img_width + tj + offset_width + offset_img;
                        else if (!edge_flag_height && edge_flag_width)
                            mem_in_index = (ti + offset_height) * img_width + img_width - 1 + offset_img;
                        else if (edge_flag_height && edge_flag_width)
                            mem_in_index = (img_height - 1) * img_width + img_width - 1 + offset_img;
                        else
                            mem_in_index = -1;

                        // 将分块好的数据存入对应 aie 所读取的 mem 区域
                        switch(aie_index) {
                            case 1:
                                if (mem_in_index = -1)
                                    mem_out1[count[aie_index]++] = 0;
                                else 
                                    mem_out1[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 2:
                                if (mem_in_index = -1)
                                    mem_out2[count[aie_index]++] = 0;
                                else 
                                    mem_out2[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 3:
                                if (mem_in_index = -1)
                                    mem_out3[count[aie_index]++] = 0;
                                else 
                                    mem_out3[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 4:
                                if (mem_in_index = -1)
                                    mem_out4[count[aie_index]++] = 0;
                                else 
                                    mem_out4[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 5:
                                if (mem_in_index = -1)
                                    mem_out5[count[aie_index]++] = 0;
                                else 
                                    mem_out5[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 6:
                                if (mem_in_index = -1)
                                    mem_out6[count[aie_index]++] = 0;
                                else 
                                    mem_out6[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 7:
                                if (mem_in_index = -1)
                                    mem_out7[count[aie_index]++] = 0;
                                else 
                                    mem_out7[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 8:
                                if (mem_in_index = -1)
                                    mem_out8[count[aie_index]++] = 0;
                                else 
                                    mem_out8[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 9:
                                if (mem_in_index = -1)
                                    mem_out9[count[aie_index]++] = 0;
                                else 
                                    mem_out9[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 10:
                                if (mem_in_index = -1)
                                    mem_out10[count[aie_index]++] = 0;
                                else 
                                    mem_out10[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 11:
                                if (mem_in_index = -1)
                                    mem_out11[count[aie_index]++] = 0;
                                else 
                                    mem_out11[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 12:
                                if (mem_in_index = -1)
                                    mem_out12[count[aie_index]++] = 0;
                                else 
                                    mem_out12[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 13:
                                if (mem_in_index = -1)
                                    mem_out13[count[aie_index]++] = 0;
                                else 
                                    mem_out13[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 14:
                                if (mem_in_index = -1)
                                    mem_out14[count[aie_index]++] = 0;
                                else 
                                    mem_out14[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            case 15:
                                if (mem_in_index = -1)
                                    mem_out15[count[aie_index]++] = 0;
                                else 
                                    mem_out15[count[aie_index]++] = mem_in[mem_in_index];
                                break;
                            default:
                                if (mem_in_index = -1)
                                    mem_out1[count[aie_index]++] = 0;
                                else 
                                    mem_out1[count[aie_index]++] = mem_in[mem_in_index];
                        }
                    }
                }
            }
        }
    }
}
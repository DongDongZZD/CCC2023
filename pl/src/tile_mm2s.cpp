#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

void load_tail(ap_int<BUS_DWIDTH> *mem_in, ap_int<DWIDTH> *tile_input_buffer, unsigned gid, unsigned uid, unsigned tile_width_number, unsigned tile_height_number);
void transfer_tail();

// 对一张图片的指定位置进行 tile 操作
void tile_mm2s(ap_int<BUS_DWIDTH> *mem_in, 
hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2,  hls::stream<data> &s3, 
hls::stream<data> &s4, hls::stream<data> &s5, hls::stream<data> &s6,  hls::stream<data> &s7,
hls::stream<data> &s8, hls::stream<data> &s9, hls::stream<data> &s10, hls::stream<data> &s11) {

    ap_int<DWIDTH> tile_input_buffer_0[AIE_KERNEL_NUMBER][TILE_ELEMENT];
    ap_int<DWIDTH> tile_input_buffer_1[AIE_KERNEL_NUMBER][TILE_ELEMENT];

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_width_number  = ceil((float)(IMG_WIDTH  - TILE_WIDTH)  / (TILE_WIDTH  - 2)) + 1;
    unsigned tile_height_number = ceil((float)(IMG_HEIGHT - TILE_HEIGHT) / (TILE_HEIGHT - 2)) + 1;
    unsigned tile_loop_group    = ceil((float)(tile_width_number * tile_height_number) / AIE_KERNEL_NUMBER);

    unsigned pingpong = 0;

    for (unsigned i = 0; i < AIE_KERNEL_NUMBER; i++) {
        #pragma HLS unroll
        load_tail(mem_in, tile_input_buffer_0[i], 0, i);
    }

    tile_loop:
    for (unsigned gid = 1; gid < tile_loop_group; gid++) {

        if (pingpong = 0) {

            for (unsigned i = 0; i < AIE_KERNEL_NUMBER; i++) {
                #pragma HLS unroll
                load_tail(mem_in, tile_input_buffer_1[i], gid, i, tile_width_number, tile_height_number);
            }

            for (unsigned i = 0; i < AIE_KERNEL_NUMBER; i++) {
                #pragma HLS unroll
                transfer_tail(tile_input_buffer_0[i], tl,
                                s0, s1, s2, s3, s4,  s5,
                                s6, s7, s8, s9, s10, s11);
            }

            pingpong = 1;

        }

        else {
            
            for (unsigned i = 0; i < AIE_KERNEL_NUMBER; i++) {
                #pragma HLS unroll
                load_tail(mem_in, tile_input_buffer_0[i], gid, i);
            }

            for (unsigned i = 0; i < AIE_KERNEL_NUMBER; i++) {
                #pragma HLS unroll
                transfer_tail(tile_input_buffer_1[i], tl,
                                s0, s1, s2, s3, s4,  s5,
                                s6, s7, s8, s9, s10, s11);
            }

            pingpong = 0;

        }

    }

    if (pingpong == 0) {
        for (unsigned i = 0; i < AIE_KERNEL_NUMBER; i++) {
            #pragma HLS unroll
            transfer_tail(tile_input_buffer_0[i], tl,
                            s0, s1, s2, s3, s4,  s5,
                            s6, s7, s8, s9, s10, s11);
        }
    }
    else {
        for (unsigned i = 0; i < AIE_KERNEL_NUMBER; i++) {
            #pragma HLS unroll
            transfer_tail(tile_input_buffer_1[i], tl,
                            s0, s1, s2, s3, s4,  s5,
                            s6, s7, s8, s9, s10, s11);
        }
    }
}

void load_tail(ap_int<BUS_DWIDTH> *mem_in, ap_int<DWIDTH> *tile_input_buffer, unsigned gid, unsigned uid, unsigned tile_width_number, unsigned tile_height_number) {
    
    unsigned tile_index_width  = (gid * AIE_KERNEL_NUMBER + uid) % tile_width_number;
    unsigned tile_index_height = (gid * AIE_KERNEL_NUMBER + uid) / tile_width_number;

    unsigned offset_width  = tile_index_width  * (TILE_WIDTH  - 2);
    unsigned offset_height = tile_index_height * (TILE_HEIGHT - 2); 

    ap_int<DWIDTH>* base = (ap_int<DWIDTH>*)mem_in + offset_height * IMG_WIDTH + offset_width;

    unsigned count = 0;

    if (tile_index_height >= 0 && tile_index_height < tile_height_number - 1 
            && tile_index_width >= 0 && tile_index_width < tile_width_number - 1) {
        for (unsigned th = 0; th < TILE_HEIGHT; th++) {
            for (unsigned tw = 0; tw < TILE_WIDTH; tw++) {
                
                unsigned mem_in_index_uid = (th * TILE_WIDTH + tw) % DATA_NUM;

                if (mem_in_index_uid == 0) {
                    ap_int<BUS_DWIDTH> mem_in_tmp = *((ap_int<BUS_DWIDTH>*)(base + th * img_width + tw));
                }

                tile_input_buffer[count++] = mem_in_tmp.range((mem_in_index_uid + 1) * DWIDTH - 1, mem_in_index_uid * DWIDTH);
            }
        }
    }

    else {
        for (unsigned th = 0; th < TILE_HEIGHT; th++) {
            for (unsigned tw = 0; tw < TILE_WIDTH; tw++) {
                
                // 遍历到图片边缘后需要进行 padding 操作 
                // mem_in_index == -1 表示补零
                if ((th + offset_height < img_height) && (tw + offset_width < img_width)) 
                    mem_in_index = (th + offset_height) * img_width + tw + offset_width + offset_img;
                else if ((th + offset_height == img_height) && (tw + offset_width < img_width))
                    mem_in_index = (img_height - 1) * img_width + tw + offset_width + offset_img;
                else if ((th + offset_height < img_height) && (tw + offset_width == img_width))
                    mem_in_index = (th + offset_height) * img_width + img_width - 1 + offset_img;
                else if ((th + offset_height == img_height) && (tw + offset_width == img_width))
                    mem_in_index = (img_height - 1) * img_width + img_width - 1 + offset_img;
                else
                    mem_in_index = -1;

                if (mem_in_index == -1)
                    tile_input_buffer[count++] = 0;
                else {
                    mem_in_index_gid = mem_in_index / DATA_NUM;
                    mem_in_index_uid = mem_in_index % DATA_NUM;
                    mem_tmp = mem_in[mem_in_index_gid].range((mem_in_index_uid + 1) * DWIDTH - 1, mem_in_index_uid * DWIDTH);
                    x.data = mem_tmp;
                }
                    
                x.keep_all();
                // 将分块好的数据存入对应 aie 所读取的 mem 区域
                switch(aie_index) {
                    case 0:
                        s0.write(x);
                        break;
                    case 1:
                        s1.write(x);
                        break;
                    case 2:
                        s2.write(x);
                        break;
                    case 3:
                        s3.write(x);
                        break;
                    case 4:
                        s4.write(x);
                        break;
                    case 5:
                        s5.write(x);
                        break;
                    case 6:
                        s6.write(x);
                        break;
                    default:
                        s0.write(x);
                }
            }
        }
    }
}
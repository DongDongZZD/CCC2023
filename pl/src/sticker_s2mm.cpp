#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

void load_tile(hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2, 
hls::stream<data> &s3, hls::stream<data> &s4, hls::stream<data> &s5, hls::stream<data> &s6,
hls::stream<data> &s7, hls::stream<data> &s8, hls::stream<data> &s9, hls::stream<data> &s10, hls::stream<data> &s11,
ap_int<DWIDTH> aie_input_buffer[TILE_ELEMENT], unsigned uid);

void transfer_tile(ap_int<DWIDTH> aie_input_buffer[TILE_ELEMENT], ap_int<DWIDTH>* mem_out,
                    unsigned gid, unsigned uid, unsigned tile_num_width, unsigned tile_num_height);

// 将当前横纵坐标对应的 tile 拼接到图片中
void sticker_s2mm(hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2, 
hls::stream<data> &s3, hls::stream<data> &s4, hls::stream<data> &s5, hls::stream<data> &s6,
hls::stream<data> &s7, hls::stream<data> &s8, hls::stream<data> &s9, hls::stream<data> &s10, hls::stream<data> &s11,
ap_int<DWIDTH> *mem_out) {

    ap_int<DWIDTH> aie_input_buffer_0[AIE_KERNEL_NUMBER][TILE_ELEMENT];
    ap_int<DWIDTH> aie_input_buffer_1[AIE_KERNEL_NUMBER][TILE_ELEMENT];

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_num_width  = ceil((float)(IMG_WIDTH  - TILE_WIDTH)  / (TILE_WIDTH  - 2)) + 1;
    unsigned tile_num_height = ceil((float)(IMG_HEIGHT - TILE_HEIGHT) / (TILE_HEIGHT - 2)) + 1;
    unsigned tile_loop_group = ceil((float)(tile_num_width * tile_num_height) / AIE_KERNEL_NUMBER);

    unsigned pingpong = 0;

    for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
        #pragma HLS unroll AIE_KERNEL_NUMBER
        load_tile(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
                  aie_input_buffer_0[uid], uid);
    }

    tile_loop:
    for (unsigned gid = 1; gid < tile_loop_group; gid++) {

        if (pingpong == 0) {

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                #pragma HLS unroll AIE_KERNEL_NUMBER
                load_tile(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
                          aie_input_buffer_1[uid], uid);
            }

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                #pragma HLS unroll AIE_KERNEL_NUMBER
                transfer_tile(aie_input_buffer_0[uid], mem_out, gid - 1, uid, tile_num_width, tile_num_height);
            }

            pingpong = 1;

        }

        else {
            
            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                #pragma HLS unroll AIE_KERNEL_NUMBER
                load_tile(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
                          aie_input_buffer_0[uid], uid);
            }

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                #pragma HLS unroll AIE_KERNEL_NUMBER
                transfer_tile(aie_input_buffer_1[uid], mem_out, gid - 1, uid, tile_num_width, tile_num_height);
            }

            pingpong = 0;

        }

    }

    if (pingpong == 0) {
        for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
            #pragma HLS unroll AIE_KERNEL_NUMBER
            transfer_tile(aie_input_buffer_0[uid], mem_out, tile_loop_group - 1, uid, tile_num_width, tile_num_height);
        }
    }
    else {
        for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
            #pragma HLS unroll AIE_KERNEL_NUMBER
            transfer_tile(aie_input_buffer_1[uid], mem_out, tile_loop_group - 1, uid, tile_num_width, tile_num_height);
        }
    }
}

void transfer_tile(ap_int<DWIDTH> aie_input_buffer[TILE_ELEMENT], ap_int<DWIDTH>* mem_out,
                    unsigned gid, unsigned uid, unsigned tile_num_width, unsigned tile_num_height) {
    
    unsigned tile_index_width  = (gid * AIE_KERNEL_NUMBER + uid) % tile_num_width;
    unsigned tile_index_height = (gid * AIE_KERNEL_NUMBER + uid) / tile_num_width;

    unsigned offset_width  = tile_index_width  * (TILE_WIDTH  - 2);
    unsigned offset_height = tile_index_height * (TILE_HEIGHT - 2); 

    for (int th = 0; th < TILE_HEIGHT; th++) {
        for (int tw = 0; tw < TILE_WIDTH; tw++) {

            unsigned mem_out_index = (th + offset_height) * IMG_WIDTH + tw + offset_width;
            unsigned aie_input_index = th * TILE_WIDTH + tw;

            if (tile_index_height == 0 && tile_index_width == 0) {
                if (th >= 0 && th < TILE_HEIGHT - 1 && tw >= 0 && tw < TILE_WIDTH - 1) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_height == 0 && tile_index_width > 0 && tile_index_width <= tile_num_width - 2) {
                if (th >= 0 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH - 1) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_width == 0 && tile_index_height > 0 && tile_index_height <= tile_num_height - 2) {
                if (th >= 1 && th < TILE_HEIGHT - 1 && tw >= 0 && tw < TILE_WIDTH - 1) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_width > 0 && tile_index_height > 0 && tile_index_width <= tile_num_width - 2 && tile_index_height <= tile_num_height - 2) {
                if (th >= 1 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH - 1) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_height == 0 && tile_index_width == tile_num_width - 1) {
                if (th >= 0 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH && tw + offset_width < IMG_WIDTH) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_height > 0 && tile_index_height <= tile_num_height - 2 && tile_index_width == tile_num_width - 1) {
                if (th >= 1 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH && tw + offset_width < IMG_WIDTH) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_width == 0 && tile_index_height == tile_num_height - 1) {
                if (th >= 1 && th < TILE_HEIGHT && tw >= 0 && tw < TILE_WIDTH - 1 && th + offset_height < IMG_HEIGHT) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_height == tile_num_height - 1 && tile_index_width > 0 && tile_index_width <= tile_num_width - 2) {
                if (th >= 1 && th < TILE_HEIGHT && tw >= 1 && tw < TILE_WIDTH - 1 && th + offset_height < IMG_HEIGHT) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }

            else if (tile_index_height == tile_num_height - 1 && tile_index_width == tile_num_width - 1) {
                if (th >= 1 && th < TILE_HEIGHT && tw >= 1 && tw < TILE_WIDTH && (th + offset_height < IMG_HEIGHT) && (tw + offset_width < IMG_WIDTH)) {
                    mem_out[mem_out_index] = aie_input_buffer[aie_input_index];
                }
            }
        }
    }
}

void load_tile(hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2, 
hls::stream<data> &s3, hls::stream<data> &s4, hls::stream<data> &s5, hls::stream<data> &s6,
hls::stream<data> &s7, hls::stream<data> &s8, hls::stream<data> &s9, hls::stream<data> &s10, hls::stream<data> &s11,
ap_int<DWIDTH> aie_input_buffer[TILE_ELEMENT], unsigned uid) {

    for (int i = 0; i < TILE_ELEMENT; i++) {
        data x;
        switch(uid) {
            case 0:
                x = s0.read();
                break;
            case 1:
                x = s1.read();
                break;
            case 2:
                x = s2.read();
                break;
            case 3:
                x = s3.read();
                break;
            case 4:
                x = s4.read();
                break;
            case 5:
                x = s5.read();
                break;
            case 6:
                x = s6.read();
                break;
            case 7:
                x = s7.read();
                break;
            case 8:
                x = s8.read();
                break;
            case 9:
                x = s9.read();
                break;
            case 10:
                x = s10.read();
                break;
            case 11:
                x = s11.read();
                break;
            default:
                x = s0.read();
                break;
        }

        aie_input_buffer[i] = x.data;
    }
}

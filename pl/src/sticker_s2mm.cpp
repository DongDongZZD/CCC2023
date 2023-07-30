#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

// 将当前横纵坐标对应的 tile 拼接到图片中
void sticker_s2mm(hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2, 
                  hls::stream<data> &s3, hls::stream<data> &s4, hls::stream<data> &s5,
                  hls::stream<data> &s6, hls::stream<data> &s7, hls::stream<data> &s8,
                  hls::stream<data> &s9, hls::stream<data> &s10, hls::stream<data> &s11,
                  ap_int<DWIDTH> *mem_out) {

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_num_width  = ceil((float)(IMG_WIDTH  - TILE_WIDTH)  / (TILE_WIDTH  - 2)) + 1;
    unsigned tile_num_height = ceil((float)(IMG_HEIGHT - TILE_HEIGHT) / (TILE_HEIGHT - 2)) + 1;

    // 用作 mem_out 的索引
    unsigned mem_out_index;

    // 遍历所有的 tile
    for (unsigned tile_index_height = 0; tile_index_height < tile_num_height; tile_index_height++) {
        for (unsigned tile_index_width = 0; tile_index_width < tile_num_width; tile_index_width++) {

            // 当前的 tile 应该从第 aie_index 个 aie kernel 对应的 mem 处取得
            unsigned aie_index = (tile_index_height * tile_num_width + tile_index_width) % AIE_KERNEL_NUMBER;

            // 当前 tile 在当前图片中的偏移
            unsigned offset_width  = tile_index_width  * (TILE_WIDTH  - 2);
            unsigned offset_height = tile_index_height * (TILE_HEIGHT - 2);

            // 遍历当前 tile
            for (int th = 0; th < TILE_HEIGHT; th++) {
                for (int tw = 0; tw < TILE_WIDTH; tw++) {
                    data x;
                    switch(aie_index) {
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

                    mem_out_index = (th + offset_height) * IMG_WIDTH + tw + offset_width;

                    if (tile_index_height == 0 && tile_index_width == 0) {
                        if (th >= 0 && th < TILE_HEIGHT - 1 && tw >= 0 && tw < TILE_WIDTH - 1) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_height == 0 && tile_index_width > 0 && tile_index_width <= tile_num_width - 2) {
                        if (th >= 0 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH - 1) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_width == 0 && tile_index_height > 0 && tile_index_height <= tile_num_height - 2) {
                        if (th >= 1 && th < TILE_HEIGHT - 1 && tw >= 0 && tw < TILE_WIDTH - 1) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_width > 0 && tile_index_height > 0 && tile_index_width <= tile_num_width - 2 && tile_index_height <= tile_num_height - 2) {
                        if (th >= 1 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH - 1) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_height == 0 && tile_index_width == tile_num_width - 1) {
                        if (th >= 0 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH && tw + offset_width < IMG_WIDTH) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_height > 0 && tile_index_height <= tile_num_height - 2 && tile_index_width == tile_num_width - 1) {
                        if (th >= 1 && th < TILE_HEIGHT - 1 && tw >= 1 && tw < TILE_WIDTH && tw + offset_width < IMG_WIDTH) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_width == 0 && tile_index_height == tile_num_height - 1) {
                        if (th >= 1 && th < TILE_HEIGHT && tw >= 0 && tw < TILE_WIDTH - 1 && th + offset_height < IMG_HEIGHT) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_height == tile_num_height - 1 && tile_index_width > 0 && tile_index_width <= tile_num_width - 2) {
                        if (th >= 1 && th < TILE_HEIGHT && tw >= 1 && tw < TILE_WIDTH - 1 && th + offset_height < IMG_HEIGHT) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }

                    else if (tile_index_height == tile_num_height - 1 && tile_index_width == tile_num_width - 1) {
                        if (th >= 1 && th < TILE_HEIGHT && tw >= 1 && tw < TILE_WIDTH && (th + offset_height < IMG_HEIGHT) && (tw + offset_width < IMG_WIDTH)) {
                            mem_out[mem_out_index] = x.data;
                        }
                    }
                }
            }
        }
    }
}

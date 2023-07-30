#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

// typedef qdma_axis<32,0,0,0> data_32;

// 将当前横纵坐标对应的 tile 拼接到图片中
void sticker_s2mm(hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2, 
hls::stream<data> &s3, hls::stream<data> &s4, hls::stream<data> &s5, hls::stream<data> &s6,
ap_int<DWIDTH> *mem_out) {

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_num_width  = ceil((float)(img_width  - tile_width)  / (tile_width  - 2)) + 1;
    unsigned tile_num_height = ceil((float)(img_height - tile_height) / (tile_height - 2)) + 1;

    // 用作 mem_out 的索引
    unsigned mem_out_index;

    // 遍历所有图片
    for (unsigned img_index = 0; img_index< img_number; img_index++) {

        // 已经处理过的图片对应的元素个数
        unsigned offset_img = img_index * img_width * img_height;
        
        // 遍历所有的 tile
        for (unsigned tile_index_height = 0; tile_index_height < tile_num_height; tile_index_height++) {
            for (unsigned tile_index_width = 0; tile_index_width < tile_num_width; tile_index_width++) {

                // 当前的 tile 应该从第 aie_index 个 aie kernel 对应的 mem 处取得
                unsigned aie_index = (tile_index_height * tile_num_width + tile_index_width) % AIE_KERNEL_NUMBER;

                // 当前 tile 在当前图片中的偏移
                unsigned offset_width  = tile_index_width  * (tile_width  - 2);
                unsigned offset_height = tile_index_height * (tile_height - 2);

                // 遍历当前 tile
                for (int th = 0; th < tile_height; th++) {
                    for (int tw = 0; tw < tile_width; tw++) {
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
                            default:
                                x = s0.read();
                                break;
                        }

                        mem_out_index = (th + offset_height) * img_width + tw + offset_width + offset_img;

                        if (tile_index_height == 0 && tile_index_width == 0) {
                            if (th >= 0 && th < tile_height - 1 && tw >= 0 && tw < tile_width - 1) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_height == 0 && tile_index_width > 0 && tile_index_width <= tile_num_width - 2) {
                            if (th >= 0 && th < tile_height - 1 && tw >= 1 && tw < tile_width - 1) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_width == 0 && tile_index_height > 0 && tile_index_height <= tile_num_height - 2) {
                            if (th >= 1 && th < tile_height - 1 && tw >= 0 && tw < tile_width - 1) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_width > 0 && tile_index_height > 0 && tile_index_width <= tile_num_width - 2 && tile_index_height <= tile_num_height - 2) {
                            if (th >= 1 && th < tile_height - 1 && tw >= 1 && tw < tile_width - 1) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_height == 0 && tile_index_width == tile_num_width - 1) {
                            if (th >= 0 && th < tile_height - 1 && tw >= 1 && tw < tile_width && tw + offset_width < img_width) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_height > 0 && tile_index_height <= tile_num_height - 2 && tile_index_width == tile_num_width - 1) {
                            if (th >= 1 && th < tile_height - 1 && tw >= 1 && tw < tile_width && tw + offset_width < img_width) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_width == 0 && tile_index_height == tile_num_height - 1) {
                            if (th >= 1 && th < tile_height && tw >= 0 && tw < tile_width - 1 && th + offset_height < img_height) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_height == tile_num_height - 1 && tile_index_width > 0 && tile_index_width <= tile_num_width - 2) {
                            if (th >= 1 && th < tile_height && tw >= 1 && tw < tile_width - 1 && th + offset_height < img_height) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }

                        else if (tile_index_height == tile_num_height - 1 && tile_index_width == tile_num_width - 1) {
                            if (th >= 1 && th < tile_height && tw >= 1 && tw < tile_width && (th + offset_height < img_height) && (tw + offset_width < img_width)) {
                                mem_out[mem_out_index] = x.data;
                            }
                        }
                    }
                }
            }
        }
    }
}
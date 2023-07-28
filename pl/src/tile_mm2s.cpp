#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

// 对一张图片的指定位置进行 tile 操作
void tile_mm2s(ap_int<DWIDTH> *mem_in, 
hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2, hls::stream<data> &s3, 
hls::stream<data> &s4, hls::stream<data> &s5, hls::stream<data> &s6) {

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_width_number  = ceil((float)(img_width  - tile_width)  / (tile_width  - 2)) + 1;
    unsigned tile_height_number = ceil((float)(img_height - tile_height) / (tile_height - 2)) + 1;

    // 用作 mem_in 的索引
    int mem_in_index;

    // 遍历所有图片
    for (unsigned img_index = 0; img_index < img_number; img_index++) {

        // 已经处理过的图片对应的元素个数
        unsigned offset_img = img_index * img_width * img_height;

        // 遍历所有的 tile
        for (unsigned tile_index_height = 0; tile_index_height < tile_height_number; tile_index_height++) {
            for (unsigned tile_index_width = 0; tile_index_width < tile_width_number; tile_index_width++) {

                // 当前的 tile 应该传输给第 aie_index 个 aie kernel 计算
                unsigned aie_index = (tile_index_height * tile_width_number + tile_index_width) % AIE_KERNEL_NUMBER;
                
                // 当前 tile 相对于第一个元素的偏移
                unsigned offset_width  = tile_index_width * (tile_width - 2);
                unsigned offset_height = tile_index_height * (tile_height - 2);

                // 遍历当前的 tile
                for (unsigned th = 0; th < tile_height; th++) {
                    for (unsigned tw = 0; tw < tile_width; tw++) {
                        
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

                        data x;
                        if (mem_in_index == -1)
                            x.data = 0;
                        else 
                            x.data = mem_in[mem_in_index];
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
    }
}

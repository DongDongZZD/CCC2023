#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

// 对一张图片的指定位置进行 tile 操作
void tile_mm2s(ap_int<BUS_DWIDTH> *mem_in, 
hls::stream<data_128> &s0, hls::stream<data_128> &s1, hls::stream<data_128> &s2, hls::stream<data_128> &s3, 
hls::stream<data_128> &s4, hls::stream<data_128> &s5, hls::stream<data_128> &s6) {

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_width_number  = ceil((float)(img_width  - tile_width)  / (tile_width  - 2)) + 1;
    unsigned tile_height_number = ceil((float)(img_height - tile_height) / (tile_height - 2)) + 1;

    // 用作 mem_in 的索引
    int mem_in_index;
    unsigned mem_in_index_gid;
    unsigned mem_in_index_uid;
    ap_int<BUS_DWIDTH> mem_in_tmp;
    ap_int<32> mem_tmp;
    ap_int<32> mem_tmp_32;
    ap_int<64> mem_tmp_64;
    ap_int<96> mem_tmp_96;
    ap_int<128> mem_tmp_128;

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
                // ap_int<DWIDTH>* base = (ap_int<DWIDTH>*)mem_in + offset_img + offset_height * img_width + offset_width;
                unsigned base = offset_img + offset_height * img_width + offset_width;
                unsigned count = 0;

                if (tile_index_height >= 0 && tile_index_height < tile_height_number - 1 
                        && tile_index_width >= 0 && tile_index_width < tile_width_number - 1) {
                    for (unsigned th = 0; th < tile_height; th++) {
                        for (unsigned tw = 0; tw < tile_width; tw++) {
                            
                            mem_in_index_uid = (th * img_width + tw + base) % DATA_NUM;
                            mem_in_index_gid = (th * img_width + tw + base) / DATA_NUM;

                            if (tw == 0 || mem_in_index_uid == 0) {
                                // mem_in_tmp = *((ap_int<BUS_DWIDTH>*)(base + th * img_width + tw));
                                mem_in_tmp = mem_in[mem_in_index_gid];
                            }

                            mem_tmp = mem_in_tmp.range((mem_in_index_uid + 1) * 32 - 1, mem_in_index_uid * 32);
                            
                            if (count == 0) {
                                mem_tmp_32 = mem_tmp;
                                count++;
                            }
                            else if (count == 1) {
                                mem_tmp_64 = mem_tmp_32.concat(mem_tmp);
                                count++;
                            }
                            else if (count == 2) {
                                mem_tmp_96 = mem_tmp_64.concat(mem_tmp);
                                count++;
                            }
                            else if (count == 3) {
                                mem_tmp_128 = mem_tmp_96.concat(mem_tmp);
                                data_128 x;
                                x.data = mem_tmp_128;
                                x.keep_all();
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
                                count = 0;
                                mem_tmp_128 = ap_int<128>("0", 16);
                            }
                        }
                    }
                }

                else {
                    count = 0;
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

                            data_128 x;
                            if (mem_in_index == -1)
                                // x.data = 0;
                                mem_tmp = ap_int<32>("0", 16);
                            else {
                                mem_in_index_gid = mem_in_index / DATA_NUM;
                                mem_in_index_uid = mem_in_index % DATA_NUM;
                                mem_tmp = mem_in[mem_in_index_gid].range((mem_in_index_uid + 1) * 32 - 1, mem_in_index_uid * 32);
                                // x.data = mem_tmp;
                            }
                            
                            if (count == 0) {
                                mem_tmp_32 = mem_tmp;
                                count++;
                            }
                            else if (count == 1) {
                                mem_tmp_64 = mem_tmp_32.concat(mem_tmp);
                                count++;
                            }
                            else if (count == 2) {
                                mem_tmp_96 = mem_tmp_64.concat(mem_tmp);
                                count++;
                            }
                            else if (count == 3) {
                                mem_tmp_128 = mem_tmp_96.concat(mem_tmp);
                                x.data = mem_tmp_128;
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
                                count = 0;
                                mem_tmp_128 = ap_int<128>("0", 16);
                            }
                        }
                    }
                }
            }
        }
    }
}

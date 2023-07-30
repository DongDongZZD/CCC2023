#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "config.h"

void transfer_mm2s(ap_int<BUS_DWIDTH>* mem_in, hls::stream<data> &s, unsigned gid, unsigned uid, unsigned tile_width_number, unsigned tile_height_number);

// 对一张图片的指定位置进行 tile 操作
void tile_mm2s(ap_int<BUS_DWIDTH> *mem_in_0, ap_int<BUS_DWIDTH> *mem_in_1, ap_int<BUS_DWIDTH> *mem_in_2,
hls::stream<data> &s0, hls::stream<data> &s1, hls::stream<data> &s2) {

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_width_number  = ceil((float)(IMG_WIDTH  - TILE_WIDTH)  / (TILE_WIDTH  - 2)) + 1;
    unsigned tile_height_number = ceil((float)(IMG_HEIGHT - TILE_HEIGHT) / (TILE_HEIGHT - 2)) + 1;
    unsigned tile_loop_group    = ceil((float)(tile_width_number * tile_height_number) / AIE_KERNEL_NUMBER);

    tile_loop:
    for (unsigned gid = 0; gid < tile_loop_group; gid++) {
        
        transfer_mm2s(mem_in_0, s0, gid, 0, tile_width_number, tile_height_number);
        transfer_mm2s(mem_in_1, s1, gid, 1, tile_width_number, tile_height_number);
        transfer_mm2s(mem_in_2, s2, gid, 2, tile_width_number, tile_height_number);

    }
}

void transfer_mm2s(ap_int<BUS_DWIDTH>* mem_in, hls::stream<data> &s, unsigned gid, unsigned uid, unsigned tile_width_number, unsigned tile_height_number) {

    unsigned tile_index_width  = (gid * AIE_KERNEL_NUMBER + uid) % tile_width_number;
    unsigned tile_index_height = (gid * AIE_KERNEL_NUMBER + uid) / tile_width_number;

    unsigned offset_width  = tile_index_width  * (TILE_WIDTH  - 2);
    unsigned offset_height = tile_index_height * (TILE_HEIGHT - 2); 

    unsigned base = offset_height * IMG_WIDTH + offset_width;

    ap_int<BUS_DWIDTH> mem_in_tmp;
    unsigned mem_in_index;

    if (tile_index_height >= 0 && tile_index_height < tile_height_number - 1 
            && tile_index_width >= 0 && tile_index_width < tile_width_number - 1) {
        for (unsigned th = 0; th < TILE_HEIGHT; th++) {
            for (unsigned tw = 0; tw < TILE_WIDTH; tw++) {

                #pragma HLS unroll
                
                unsigned mem_in_index_uid = (th * IMG_WIDTH + tw + base) % DATA_NUM;
                unsigned mem_in_index_gid = (th * IMG_WIDTH + tw + base) / DATA_NUM;

                if (tw == 0 || mem_in_index_uid == 0) {
                    mem_in_tmp = mem_in[mem_in_index_gid];
                }

                data x;
                x.data = mem_in_tmp.range((mem_in_index_uid + 1) * DWIDTH - 1, mem_in_index_uid * DWIDTH);
                x.keep_all();
                s.write(x);
            }
        }
    }

    else {
        for (unsigned th = 0; th < TILE_HEIGHT; th++) {
            for (unsigned tw = 0; tw < TILE_WIDTH; tw++) {
                
                // 遍历到图片边缘后需要进行 padding 操作 
                // mem_in_index == -1 表示补零
                if ((th + offset_height < IMG_HEIGHT) && (tw + offset_width < IMG_WIDTH)) 
                    mem_in_index = (th + offset_height) * IMG_WIDTH + tw + offset_width;
                else if ((th + offset_height == IMG_HEIGHT) && (tw + offset_width < IMG_WIDTH))
                    mem_in_index = (IMG_HEIGHT - 1) * IMG_WIDTH + tw + offset_width;
                else if ((th + offset_height < IMG_HEIGHT) && (tw + offset_width == IMG_WIDTH))
                    mem_in_index = (th + offset_height) * IMG_WIDTH + IMG_WIDTH - 1;
                else if ((th + offset_height == IMG_HEIGHT) && (tw + offset_width == IMG_WIDTH))
                    mem_in_index = (IMG_HEIGHT - 1) * IMG_WIDTH + IMG_WIDTH - 1;
                else
                    mem_in_index = -1;

                data x;
                if (mem_in_index == -1)
                    x.data = 0;
                else {
                    unsigned mem_in_index_gid = mem_in_index / DATA_NUM;
                    unsigned mem_in_index_uid = mem_in_index % DATA_NUM;
                    x.data = mem_in[mem_in_index_gid].range((mem_in_index_uid + 1) * DWIDTH - 1, mem_in_index_uid * DWIDTH);
                x.keep_all();
                s.write(x);
                }
            }
        }
    }

}

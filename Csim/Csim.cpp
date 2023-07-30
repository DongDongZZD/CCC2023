#include <iostream>
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <string.h>


#define AIE_KERNEL_NUMBER 12
#define BUS_DWIDTH 256
#define DWIDTH 32
#define DATA_NUM (BUS_DWIDTH / DWIDTH)
#define IMG_WIDTH 3840
#define IMG_HEIGHT 2160
#define TILE_WIDTH 64
#define TILE_HEIGHT 32
#define TILE_ELEMENT TILE_WIDTH * TILE_HEIGHT

unsigned img_number = 3;

typedef struct {
    int data[DATA_NUM];
} data_bus;

void load_tile_s(int* s0, int* s1, int* s2,
    int* s3, int* s4, int* s5, int* s6,
    int* s7, int* s8, int* s9, int* s10, int* s11,
    int aie_input_buffer[TILE_ELEMENT], unsigned uid, unsigned* count);

void transfer_tile_s(int aie_input_buffer[TILE_ELEMENT], int* mem_out,
    unsigned gid, unsigned uid, unsigned tile_num_width, unsigned tile_num_height);
void sticker_s2mm(int* s0, int* s1, int* s2,
    int* s3, int* s4, int* s5, int* s6,
    int* s7, int* s8, int* s9, int* s10, int* s11,
    int* mem_out);
void tile_mm2s(data_bus* mem_in,
    int* s0, int* s1, int* s2, int* s3,
    int* s4, int* s5, int* s6, int* s7,
    int* s8, int* s9, int* s10, int* s11);
void cal_ref(int* input_buffer, unsigned width, unsigned height, int* kernel_coeff, int* ref_buffer);
void load_tile(data_bus* mem_in, int* tile_input_buffer, unsigned gid, unsigned uid, unsigned tile_width_number, unsigned tile_height_number);
void transfer_tile(int* tile_input_buffer, unsigned uid,
    int* s0, int* s1, int* s2, int* s3,
    int* s4, int* s5, int* s6, int* s7,
    int* s8, int* s9, int* s10, int* s11, unsigned* count);
void tile_mm2s(data_bus* mem_in,
    int* s0, int* s1, int* s2, int* s3,
    int* s4, int* s5, int* s6, int* s7,
    int* s8, int* s9, int* s10, int* s11);

int main() {

    /////////////////////////////////////////////////
    // Allocating Buffer in Global Memory
    /////////////////////////////////////////////////
    std::cout << "Allocate Buffer in Global Memory" << std::endl;

    unsigned img_element_number  = IMG_WIDTH * IMG_HEIGHT;
    unsigned tile_element_number = TILE_WIDTH * TILE_HEIGHT;

    unsigned tile_width_number = ceil((float)(IMG_WIDTH - TILE_WIDTH) / (TILE_WIDTH - 2)) + 1;
    unsigned tile_height_number = ceil((float)(IMG_HEIGHT - TILE_HEIGHT) / (TILE_HEIGHT - 2)) + 1;

    unsigned iteration = ceil((float)(tile_width_number * tile_height_number) / AIE_KERNEL_NUMBER);

    size_t img_size_in_bytes = sizeof(int) * img_element_number;

    // host mem ------> device mem (img_in_buffer)
    data_bus* img_in_buffer = (data_bus*)calloc(img_element_number / DATA_NUM, sizeof(data_bus));

    // img_in_buffer ---(copy)---> in_buffer_
    // std::array<xrt::bo, AIE_KERNEL_NUMBER> in_buffer_;
    int** in_buffer  = (int**)calloc(AIE_KERNEL_NUMBER, sizeof(int*));
    int** out_buffer = (int**)calloc(AIE_KERNEL_NUMBER, sizeof(int*));
    for (int i = 0; i < AIE_KERNEL_NUMBER; i++) {
        in_buffer[i]  = (int*)calloc(tile_element_number * iteration, sizeof(int));
        out_buffer[i] = (int*)calloc(tile_element_number * iteration, sizeof(int));
    }
    // in_buffer_ ---(aie kernel)---> out_buffer_

    // out_buffer_ ---(PL)---> img_out_buffer
    int* img_out_buffer = (int*)calloc(img_element_number, sizeof(int));
    int* img_input = (int*)calloc(img_element_number, sizeof(int));
    int* img_output_aie = (int*)calloc(img_element_number, sizeof(int));
    int* img_output_ref = (int*)calloc(img_element_number, sizeof(int));

    for (unsigned img_index = 0; img_index < img_number; img_index++) {
        /////////////////////////////////////////////////
        // Read data from file 
        /////////////////////////////////////////////////
        std::cout << "Read data from file" << std::endl;

        for (unsigned int i = 0; i < img_element_number; i++) {
            img_input[i] = rand() % 10;
        }
        /*printf("\n---------------------------------------------------\n");
        for (int i = 0; i < IMG_HEIGHT; i++) {
            for (int j = 0; j < IMG_WIDTH; j++) {
                printf("%5d ", img_input[i * IMG_WIDTH + j]);
            }
            printf("\n");
        }*/

        /////////////////////////////////////////////////
        // Cal output reference
        /////////////////////////////////////////////////
        std::cout << "Cal output reference" << std::endl;
        int kernel_coeff[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        cal_ref(img_input, IMG_WIDTH, IMG_HEIGHT, kernel_coeff, img_output_ref);
        /*printf("\n---------------------------------------------------\n");
        for (int i = 0; i < IMG_HEIGHT; i++) {
            for (int j = 0; j < IMG_WIDTH; j++) {
                printf("%5d ", img_output_ref[i * IMG_WIDTH + j]);
            }
            printf("\n");
        }*/

        /////////////////////////////////////////////////
        // Write input data to device global memory
        /////////////////////////////////////////////////
        std::cout << "Write input data to device global memory" << std::endl;

        for (int i = 0; i < img_element_number / DATA_NUM; i++) {
            for (int j = 0; j < DATA_NUM; j++) {
                img_in_buffer[i].data[j] = img_input[i * DATA_NUM + j];
            }
        }

        /*printf("\n---------------------------------------------------\n");
        for (int i = 0; i < IMG_WIDTH; i++) {
            for (int j = 0; j < IMG_HEIGHT; j++) {
                printf("%5d ", img_in_buffer[i * IMG_WIDTH + j]);
            }
            printf("\n");
        }*/

        /////////////////////////////////////////////////
        // Synchronize input buffers data to device global memory
        /////////////////////////////////////////////////
        std::cout << "Synchronize input buffers data to device global memory" << std::endl;

        /////////////////////////////////////////////////
        // Execute the PL compute units
        /////////////////////////////////////////////////
        std::cout << "Run the PL kernels" << std::endl;

        std::cout << "Run the tile PL" << std::endl;
        tile_mm2s(
            img_in_buffer,
            in_buffer[0], in_buffer[1], in_buffer[2], in_buffer[3], in_buffer[4], in_buffer[5],
            in_buffer[6], in_buffer[7], in_buffer[8], in_buffer[9], in_buffer[10], in_buffer[11]);

        std::cout << "AIE cal" << std::endl;

        int count_in[AIE_KERNEL_NUMBER]  = { 0 };
        int count_out[AIE_KERNEL_NUMBER] = { 0 };

        for (int i = 0; i < tile_height_number; i++) {
            for (int j = 0; j < tile_width_number; j++) {

                int* tile_input_buffer = (int*)calloc(tile_element_number, sizeof(int));
                int* tile_output_buffer = (int*)calloc(tile_element_number, sizeof(int));

                int aie_index = (i * tile_width_number + j) % AIE_KERNEL_NUMBER;

                for (int m = 0; m < tile_element_number; m++) {
                    tile_input_buffer[m] = in_buffer[aie_index][count_in[aie_index]++];
                }

                /*printf("---------------------------------------------------\n");
                printf("aie %d  input\n", aie_index);
                for (int x = 0; x < TILE_HEIGHT; x++) {
                    for (int y = 0; y < TILE_WIDTH; y++) {
                        printf("%5d ", tile_input_buffer[x * TILE_WIDTH + y]);
                    }
                    printf("\n");
                }*/

                cal_ref(tile_input_buffer, TILE_WIDTH, TILE_HEIGHT, kernel_coeff, tile_output_buffer);

                for (int m = 0; m < tile_element_number; m++) {
                    out_buffer[aie_index][count_out[aie_index]++] = tile_output_buffer[m];
                }

                /*printf("---------------------------------------------------\n");
                printf("aie %d  output\n", aie_index);
                for (int x = 0; x < TILE_HEIGHT; x++) {
                    for (int y = 0; y < TILE_WIDTH; y++) {
                        printf("%5d ", tile_output_buffer[x * IMG_WIDTH + y]);
                    }
                    printf("\n");
                }*/

                free(tile_input_buffer);
                free(tile_output_buffer);
            }
        }


        //for (int i = 0; i < iteration; i++) {
        //    //printf("\niteration:%d\n", i);
        //    int tile_offset = i * TILE_HEIGHT * TILE_WIDTH;
        //    
        //    int* tile_input_buffer = (int*)calloc(tile_element_number, sizeof(int));
        //    int* tile_output_buffer = (int*)calloc(tile_element_number, sizeof(int));

        //    for (int m = 0; m < AIE_KERNEL_NUMBER; m++) {

        //        /*printf("---------------------------------------------------\n");
        //        printf("aie %d  input\n", m);
        //        for (int x = 0; x < TILE_WIDTH; x++) {
        //            for (int y = 0; y < TILE_HEIGHT; y++) {
        //                printf("%5d ", in_buffer[m][x * TILE_HEIGHT + y + tile_offset]);
        //            }
        //            printf("\n");
        //        }*/

        //        for (int j = 0; j < tile_element_number; j++) {
        //            tile_input_buffer[j] = in_buffer[m][j + tile_offset];
        //        }
        //        cal_ref(tile_input_buffer, TILE_WIDTH, TILE_HEIGHT, kernel_coeff, tile_output_buffer);
        //        for (int j = 0; j < tile_element_number; j++) {
        //            out_buffer[m][j + tile_offset] = tile_output_buffer[j];
        //        }

        //        /*printf("---------------------------------------------------\n");
        //        printf("aie %d  output\n", m);
        //        for (int x = 0; x < TILE_WIDTH; x++) {
        //            for (int y = 0; y < TILE_HEIGHT; y++) {
        //                printf("%5d ", out_buffer[m][x * TILE_HEIGHT + y + tile_offset]);
        //            }
        //            printf("\n");
        //        }*/
        //    }
        //    free(tile_input_buffer);
        //    free(tile_output_buffer);
        //}

        std::cout << "Run the sticker PL" << std::endl;
        sticker_s2mm(
            out_buffer[0], out_buffer[1], out_buffer[2], out_buffer[3], out_buffer[4],  out_buffer[5], 
            out_buffer[6], out_buffer[7], out_buffer[8], out_buffer[9], out_buffer[10], out_buffer[11],
            img_out_buffer);

        /////////////////////////////////////////////////
        // Read output buffer data to local buffer
        /////////////////////////////////////////////////
        std::cout << "Read output data from device global memory" << std::endl;
        memcpy(img_output_aie, img_out_buffer, img_size_in_bytes);

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
    }

    

    free(img_in_buffer);
    for (int i = 0; i < AIE_KERNEL_NUMBER; i++) {
        free(in_buffer[i]);
        free(out_buffer[i]);
    }
    free(in_buffer);
    free(out_buffer);
    free(img_out_buffer);
    free(img_input);
    free(img_output_aie);
    free(img_output_ref);

    return 0;
}

// 将当前横纵坐标对应的 tile 拼接到图片中
void sticker_s2mm(int* s0, int* s1, int* s2,
    int* s3, int* s4, int* s5, int* s6,
    int* s7, int* s8, int* s9, int* s10, int* s11,
    int* mem_out) {

    int aie_input_buffer_0[AIE_KERNEL_NUMBER][TILE_ELEMENT];
    int aie_input_buffer_1[AIE_KERNEL_NUMBER][TILE_ELEMENT];

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_num_width = ceil((float)(IMG_WIDTH - TILE_WIDTH) / (TILE_WIDTH - 2)) + 1;
    unsigned tile_num_height = ceil((float)(IMG_HEIGHT - TILE_HEIGHT) / (TILE_HEIGHT - 2)) + 1;
    unsigned tile_loop_group = ceil((float)(tile_num_width * tile_num_height) / AIE_KERNEL_NUMBER);

    unsigned pingpong = 0;
    unsigned count[AIE_KERNEL_NUMBER] = { 0 };

    for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
        load_tile_s(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
            aie_input_buffer_0[uid], uid, count);
    }

    for (unsigned gid = 1; gid < tile_loop_group; gid++) {

        if (pingpong == 0) {

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                load_tile_s(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
                    aie_input_buffer_1[uid], uid, count);
            }

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                transfer_tile_s(aie_input_buffer_0[uid], mem_out, gid - 1, uid, tile_num_width, tile_num_height);
                /*printf("---------------------pingpong = %d |  gid = %d  uid = %d ------------------------------\n", pingpong, gid - 1, uid);
                for (int x = 0; x < IMG_HEIGHT; x++) {
                    for (int y = 0; y < IMG_WIDTH; y++) {
                        printf("%5d ", mem_out[x * IMG_WIDTH + y]);
                    }
                    printf("\n");
                }*/
                /*for (int x = 0; x < TILE_HEIGHT; x++) {
                    for (int y = 0; y < TILE_WIDTH; y++) {
                        printf("%5d ", aie_input_buffer_0[uid][x * TILE_WIDTH + y]);
                    }
                    printf("\n");
                }*/
            }

            pingpong = 1;

        }

        else {

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                load_tile_s(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
                    aie_input_buffer_0[uid], uid, count);
            }

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                transfer_tile_s(aie_input_buffer_1[uid], mem_out, gid - 1, uid, tile_num_width, tile_num_height);
                /*printf("---------------------pingpong = %d |  gid = %d  uid = %d ------------------------------\n", pingpong, gid - 1, uid);
                for (int x = 0; x < IMG_HEIGHT; x++) {
                    for (int y = 0; y < IMG_WIDTH; y++) {
                        printf("%5d ", mem_out[x * IMG_WIDTH + y]);
                    }
                    printf("\n");
                }*/
            }

            pingpong = 0;

        }

    }

    if (pingpong == 0) {
        for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
            transfer_tile_s(aie_input_buffer_0[uid], mem_out, tile_loop_group - 1, uid, tile_num_width, tile_num_height);
            //printf("---------------------pingpong = %d |  gid = %d  uid = %d ------------------------------\n", pingpong, tile_loop_group - 1, uid);
            /*for (int x = 0; x < IMG_HEIGHT; x++) {
                for (int y = 0; y < IMG_WIDTH; y++) {
                    printf("%5d ", mem_out[x * IMG_WIDTH + y]);
                }
                printf("\n");
            }*/
            /*for (int x = 0; x < TILE_WIDTH; x++) {
                for (int y = 0; y < TILE_HEIGHT; y++) {
                    printf("%5d ", aie_input_buffer_0[uid][x * TILE_WIDTH + y]);
                }
                printf("\n");
            }*/
        }
    }
    else {
        for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
            transfer_tile_s(aie_input_buffer_1[uid], mem_out, tile_loop_group - 1, uid, tile_num_width, tile_num_height);
            /*printf("---------------------pingpong = %d |  gid = %d ------------------------------\n", pingpong, tile_loop_group - 1);
            for (int x = 0; x < IMG_HEIGHT; x++) {
                for (int y = 0; y < IMG_WIDTH; y++) {
                    printf("%5d ", mem_out[x * IMG_WIDTH + y]);
                }
                printf("\n");
            }*/
        }
    }
}

void transfer_tile_s(int aie_input_buffer[TILE_ELEMENT], int* mem_out,
    unsigned gid, unsigned uid, unsigned tile_num_width, unsigned tile_num_height) {

    unsigned tile_index_width = (gid * AIE_KERNEL_NUMBER + uid) % tile_num_width;
    unsigned tile_index_height = (gid * AIE_KERNEL_NUMBER + uid) / tile_num_width;

    unsigned offset_width = tile_index_width * (TILE_WIDTH - 2);
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

void load_tile_s(int* s0, int* s1, int* s2,
    int* s3, int* s4, int* s5, int* s6,
    int* s7, int* s8, int* s9, int* s10, int* s11,
    int aie_input_buffer[TILE_ELEMENT], unsigned uid, unsigned* count) {

    for (int i = 0; i < TILE_ELEMENT; i++) {
        int x;
        switch (uid) {
        case 0:
            x = s0[count[uid]++];
            break;
        case 1:
            x = s1[count[uid]++];
            break;
        case 2:
            x = s2[count[uid]++];
            break;
        case 3:
            x = s3[count[uid]++];
            break;
        case 4:
            x = s4[count[uid]++];
            break;
        case 5:
            x = s5[count[uid]++];
            break;
        case 6:
            x = s6[count[uid]++];
            break;
        case 7:
            x = s7[count[uid]++];
            break;
        case 8:
            x = s8[count[uid]++];
            break;
        case 9:
            x = s9[count[uid]++];
            break;
        case 10:
            x = s10[count[uid]++];
            break;
        case 11:
            x = s11[count[uid]++];
            break;
        default:
            x = s0[count[uid]++];
            break;
        }

        aie_input_buffer[i] = x;
    }
}

// 对一张图片的指定位置进行 tile 操作
void tile_mm2s(data_bus* mem_in,
    int* s0, int* s1, int* s2, int* s3,
    int* s4, int* s5, int* s6, int* s7,
    int* s8, int* s9, int* s10, int* s11) {

   int tile_input_buffer_0[AIE_KERNEL_NUMBER][TILE_ELEMENT];
   int tile_input_buffer_1[AIE_KERNEL_NUMBER][TILE_ELEMENT];

    // 每张图片的 tile 个数（width 和 height 两个维度）
    unsigned tile_width_number = ceil((float)(IMG_WIDTH - TILE_WIDTH) / (TILE_WIDTH - 2)) + 1;
    unsigned tile_height_number = ceil((float)(IMG_HEIGHT - TILE_HEIGHT) / (TILE_HEIGHT - 2)) + 1;
    unsigned tile_loop_group = ceil((float)(tile_width_number * tile_height_number) / AIE_KERNEL_NUMBER);

    unsigned pingpong = 0;
    unsigned count[AIE_KERNEL_NUMBER] = { 0 };

    for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
        load_tile(mem_in, tile_input_buffer_0[uid], 0, uid, tile_width_number, tile_height_number);
        /*printf("---------------------pingpong = %d |  gid = %d  uid = %d ------------------------------\n", pingpong, 0, uid);
        for (int x = 0; x < TILE_HEIGHT; x++) {
            for (int y = 0; y < TILE_WIDTH; y++) {
                printf("%5d ", tile_input_buffer_0[uid][x * TILE_WIDTH + y]);
            }
            printf("\n");
        }*/
    }

    for (unsigned gid = 1; gid < tile_loop_group; gid++) {

        if (pingpong == 0) {

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                load_tile(mem_in, tile_input_buffer_1[uid], gid, uid, tile_width_number, tile_height_number);
                /*printf("---------------------pingpong = %d |  gid = %d  uid = %d ------------------------------\n", pingpong, gid, uid);
                for (int x = 0; x < TILE_HEIGHT; x++) {
                    for (int y = 0; y < TILE_WIDTH; y++) {
                        printf("%5d ", tile_input_buffer_0[uid][x * TILE_WIDTH + y]);
                    }
                    printf("\n");
                }*/
            }

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                transfer_tile(tile_input_buffer_0[uid], uid,
                    s0, s1, s2, s3, s4, s5,
                    s6, s7, s8, s9, s10, s11, count);
                /*printf("---------------------pingpong = %d |  gid = %d ------------------------------\n", pingpong, gid);
                printf("---------------------pingpong = %d |  gid = %d ------------------------------\n", pingpong, gid);
                printf("---------------------pingpong = %d |  gid = %d ------------------------------\n", pingpong, gid);
                for (int x = 0; x < TILE_HEIGHT; x++) {
                    for (int y = 0; y < TILE_WIDTH; y++) {
                        printf("%5d ", s0[x * TILE_WIDTH + y]);
                    }
                    printf("\n");
                }*/
            }

            pingpong = 1;

        }

        else {

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                load_tile(mem_in, tile_input_buffer_0[uid], gid, uid, tile_width_number, tile_height_number);
                /*printf("---------------------pingpong = %d |  gid = %d  uid = %d ------------------------------\n", pingpong, gid, uid);
                for (int x = 0; x < TILE_HEIGHT; x++) {
                    for (int y = 0; y < TILE_WIDTH; y++) {
                        printf("%5d ", tile_input_buffer_0[uid][x * TILE_WIDTH + y]);
                    }
                    printf("\n");
                }*/
            }

            for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
                transfer_tile(tile_input_buffer_1[uid], uid,
                    s0, s1, s2, s3, s4, s5,
                    s6, s7, s8, s9, s10, s11, count);
            }

            pingpong = 0;

        }

    }

    if (pingpong == 0) {
        for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
            transfer_tile(tile_input_buffer_0[uid], uid,
                s0, s1, s2, s3, s4, s5,
                s6, s7, s8, s9, s10, s11, count);
        }
    }
    else {
        for (unsigned uid = 0; uid < AIE_KERNEL_NUMBER; uid++) {
            transfer_tile(tile_input_buffer_1[uid], uid,
                s0, s1, s2, s3, s4, s5,
                s6, s7, s8, s9, s10, s11, count);
        }
    }
}

void load_tile(data_bus* mem_in,int* tile_input_buffer, unsigned gid, unsigned uid, unsigned tile_width_number, unsigned tile_height_number) {

    unsigned tile_index_width = (gid * AIE_KERNEL_NUMBER + uid) % tile_width_number;
    unsigned tile_index_height = (gid * AIE_KERNEL_NUMBER + uid) / tile_width_number;

    unsigned offset_width = tile_index_width * (TILE_WIDTH - 2);
    unsigned offset_height = tile_index_height * (TILE_HEIGHT - 2);

    unsigned base = offset_height * IMG_WIDTH + offset_width;

    unsigned count = 0;

    data_bus mem_in_tmp;
    unsigned mem_in_index;

    if (tile_index_height >= 0 && tile_index_height < tile_height_number - 1
        && tile_index_width >= 0 && tile_index_width < tile_width_number - 1) {
        for (unsigned th = 0; th < TILE_HEIGHT; th++) {
            for (unsigned tw = 0; tw < TILE_WIDTH; tw++) {

                unsigned mem_in_index_uid = (th * IMG_WIDTH + tw + base) % DATA_NUM;
                unsigned mem_in_index_gid = (th * IMG_WIDTH + tw + base) / DATA_NUM;

                

                if (tw == 0 || mem_in_index_uid == 0) {
                    mem_in_tmp = mem_in[mem_in_index_gid];
                    //printf("====================================\nmem_in_index_uid = %d  mem_in_index_gid = %d \n=============================\n", mem_in_index_uid, mem_in_index_gid);
                }

                tile_input_buffer[count++] = mem_in_tmp.data[mem_in_index_uid];
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

                if (mem_in_index == -1)
                    tile_input_buffer[count++] = 0;
                else {
                    unsigned mem_in_index_gid = mem_in_index / DATA_NUM;
                    unsigned mem_in_index_uid = mem_in_index % DATA_NUM;
                    tile_input_buffer[count++] = mem_in[mem_in_index_gid].data[mem_in_index_uid];
                }
            }
        }
    }
}

void transfer_tile_stream(int* tile_input_buffer, int* s, unsigned* count) {
    int x;
    for (unsigned i = 0; i < TILE_ELEMENT; i++) {
        x = tile_input_buffer[i];
        s[(*count)++] = x;
    }
}

void transfer_tile(int* tile_input_buffer, unsigned uid,
    int* s0, int* s1, int* s2, int* s3,
    int* s4, int* s5, int* s6, int* s7,
    int* s8, int* s9, int* s10, int* s11, unsigned* count) {
    switch (uid) {
    case 0:
        transfer_tile_stream(tile_input_buffer, s0, &count[uid]);
        break;
    case 1:
        transfer_tile_stream(tile_input_buffer, s1, &count[uid]);
        break;
    case 2:
        transfer_tile_stream(tile_input_buffer, s2, &count[uid]);
        break;
    case 3:
        transfer_tile_stream(tile_input_buffer, s3, &count[uid]);
        break;
    case 4:
        transfer_tile_stream(tile_input_buffer, s4, &count[uid]);
        break;
    case 5:
        transfer_tile_stream(tile_input_buffer, s5, &count[uid]);
        break;
    case 6:
        transfer_tile_stream(tile_input_buffer, s6, &count[uid]);
        break;
    case 7:
        transfer_tile_stream(tile_input_buffer, s7, &count[uid]);
        break;
    case 8:
        transfer_tile_stream(tile_input_buffer, s8, &count[uid]);
        break;
    case 9:
        transfer_tile_stream(tile_input_buffer, s9, &count[uid]);
        break;
    case 10:
        transfer_tile_stream(tile_input_buffer, s10, &count[uid]);
        break;
    case 11:
        transfer_tile_stream(tile_input_buffer, s11, &count[uid]);
        break;
    default:
        transfer_tile_stream(tile_input_buffer, s0, &count[uid]);
        break;
    }
}

void cal_ref(int* input_buffer, unsigned width, unsigned height, int* kernel_coeff, int* ref_buffer) {
    unsigned padding_width = width + 2;
    unsigned padding_height = height + 2;
    unsigned padding_elements_number = padding_width * padding_height;
    auto* padding_buffer = new int[padding_elements_number];

    // ������ͼƬ���� padding
    // ���Ƚ� input_buffer ȫ�������� padding_buffer ���м䡰 ����
    for (unsigned i = 0; i < height; i++) {
        for (unsigned j = 0; j < width; j++) {
            padding_buffer[(i + 1) * padding_width + (j + 1)] = input_buffer[i * width + j];
        }
    }

    // �� padding_buffer �ĸ��ǵ�Ԫ�ظ�ֵ 
    padding_buffer[0] = padding_buffer[padding_width + 1];
    padding_buffer[padding_width - 1] = padding_buffer[2 * padding_width - 2];
    padding_buffer[(padding_height - 1) * padding_width] = padding_buffer[(padding_height - 2) * padding_width + 1];
    padding_buffer[padding_height * padding_width - 1] = padding_buffer[(padding_height - 1) * padding_width - 2];

    // �� padding_buffer ʣ�µ������߽��и�ֵ
    for (unsigned i = 1; i < padding_width - 1; i++) {
        padding_buffer[i] = padding_buffer[i + padding_width];
    }
    for (unsigned i = 1; i < padding_height - 1; i++) {
        padding_buffer[i * padding_width] = padding_buffer[i * padding_width + 1];
        padding_buffer[(i + 1) * padding_width - 1] = padding_buffer[(i + 1) * padding_width - 2];
    }
    for (unsigned i = 1; i < padding_width - 1; i++) {
        padding_buffer[(padding_height - 1) * padding_width + i] = padding_buffer[(padding_height - 2) * padding_width + i];
    }

    // �� padding ���ͼƬ���о�������
    unsigned width_loop = padding_width - 2;
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

    delete[] padding_buffer;
}

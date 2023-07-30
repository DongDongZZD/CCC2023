// Copyright (C) 2023 Advanced Micro Devices, Inc
//
// SPDX-License-Identifier: MIT

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

#define AIE_KERNEL_NUMBER 7
#define BUS_DWIDTH 256
#define DWIDTH 128
#define DATA_NUM (BUS_DWIDTH / 32)

typedef qdma_axis<DWIDTH, 0, 0, 0> data;

unsigned img_width = 3840;
unsigned img_height = 2160;
unsigned img_number = 1;
unsigned tile_width = 64;
unsigned tile_height = 32;
// Copyright (C) 2023 Advanced Micro Devices, Inc
//
// SPDX-License-Identifier: MIT

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

#define DWIDTH 32
#define AIE_KERNEL_NUMBER 14
typedef qdma_axis<DWIDTH, 0, 0, 0> data;

unsigned img_width = 720;
unsigned img_height = 480;
unsigned img_number = 2;
unsigned tile_width = 64;
unsigned tile_height = 32;

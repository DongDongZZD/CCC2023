// Copyright (C) 2023 Advanced Micro Devices, Inc
//
// SPDX-License-Identifier: MIT

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

#define AIE_KERNEL_NUMBER 12
#define BUS_DWIDTH 256
#define DWIDTH 32
#define DATA_NUM (BUS_DWIDTH / DWIDTH)

#define TILE_WIDTH  64
#define TILE_HEIGHT 32
#define TILE_ELEMENT TILE_WIDTH * TILE_HEIGHT

#define IMG_WIDTH 3840
#define IMG_HEIGHT 2160

typedef qdma_axis<DWIDTH, 0, 0, 0> data;
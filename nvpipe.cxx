/*
 * Copyright (c) 2016 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual
 * property and proprietary rights in and to this software,
 * related documentation and any modifications thereto.  Any use,
 * reproduction, disclosure or distribution of this software and
 * related documentation without an express license agreement from
 * NVIDIA CORPORATION is strictly prohibited.
 *
 */
#include <cmath>
#include "nvpipe.h"
#include "libnvpipecodec/nvpipecodec.h"
#include "libnvpipecodec/nvpipecodec264.h"
#include "libnvpipeutil/formatConversionCuda.h"
#include "libnvpipeutil/nvpipeError.h"

// profiling
#include <cuda_profiler_api.h>
#include <nvToolsExt.h>

nvpipe* nvpipe_create_instance(enum NVPipeCodecID id, uint64_t bitrate)
{
    nvpipe* codec;
    switch(id) {
    case NVPIPE_CODEC_ID_NULL:
        codec = (nvpipe*) calloc( sizeof(nvpipe), 1 );
        codec->type_ = id;
        codec->codec_ptr_ = NULL;
        break;
    case NVPIPE_CODEC_ID_H264_HARDWARE:
        {
        codec = (nvpipe*) calloc( sizeof(nvpipe), 1 );
        codec->type_ = NVPIPE_CODEC_ID_H264_HARDWARE;
        NvPipeCodec264* ptr = new NvPipeCodec264();
        ptr->setBitrate(bitrate);
        ptr->setCodecImplementation(NV_CODEC);
        codec->codec_ptr_ = ptr;
        break;
        }
    case NVPIPE_CODEC_ID_H264_SOFTWARE:
        {
        codec = (nvpipe*) calloc( sizeof(nvpipe), 1 );
        codec->type_ = NVPIPE_CODEC_ID_H264_SOFTWARE;
        NvPipeCodec264* ptr = new NvPipeCodec264();
        ptr->setBitrate(bitrate);
        ptr->setCodecImplementation(FFMPEG_LIBX);
        codec->codec_ptr_ = ptr;
        break;
        }
    default:
        printf("Unrecognised format enumerator id: %d\n", id);
    }

    return codec;
}

void nvpipe_destroy_instance( nvpipe *codec )
{
    if (codec == NULL)
        return;
    
    switch(codec->type_) {
    case NVPIPE_CODEC_ID_NULL:
        memset( codec, 0, sizeof(nvpipe) );
        free(codec);
        break;
    case NVPIPE_CODEC_ID_H264_HARDWARE:
    case NVPIPE_CODEC_ID_H264_SOFTWARE:
        delete (NvPipeCodec264*) codec->codec_ptr_;
        memset( codec, 0, sizeof(nvpipe) );
        free(codec);
        break;
    default:
        printf("Unrecognised format enumerator id: %d\n", codec->type_);
        memset( codec, 0, sizeof(nvpipe) );
        free(codec);
        break;
    }
}



NVPipeErrorID nvpipe_encode(  nvpipe *codec, 
                              void *input_buffer,
                              const size_t input_buffer_size,
                              void *output_buffer,
                              size_t* output_buffer_size,
                              const int width,
                              const int height,
                              enum NVPipeImageFormat format) {
    NVPipeErrorID = 0;

    if (codec == NULL)
        return static_cast<int>(NVPIPE_ERR_INVALID_NVPIPE_INSTANCE);

    NvPipeCodec *codec_ptr = static_cast<NvPipeCodec*> 
                                (codec->codec_ptr_);

    // profiling
    cudaProfilerStart();
    nvtxRangePushA("encodingSession");

    codec_ptr->setImageSize(width, height);
    codec_ptr->setInputFrameBuffer(input_buffer, input_buffer_size);
    NVPipeErrorID = codec_ptr->encode(output_buffer,
                                      *output_buffer_size,
                                      format);

    // profiling
    nvtxRangePop();
    cudaProfilerStop();

    return NVPipeErrorID;

}

NVPipeErrorID nvpipe_decode(  nvpipe *codec, 
                              void *input_buffer,
                              size_t input_buffer_size,
                              void *output_buffer,
                              size_t output_buffer_size,
                              int* width,
                              int* height,
                              enum NVPipeImageFormat format){
    NVPipeErrorID = 0;
    
    if (codec == NULL)
        return static_cast<int>(NVPIPE_ERR_INVALID_NVPIPE_INSTANCE);

    if ( input_buffer_size == 0 ) {
        return static_cast<int>(NVPIPE_ERR_INPUT_BUFFER_EMPTY_MEMORY);
    }

    NvPipeCodec *codec_ptr = static_cast<NvPipeCodec*> 
                                (codec->codec_ptr_);

    // profiling
    cudaProfilerStart();
    nvtxRangePushA("decodingSession");

    codec_ptr->setImageSize(*width, *height);
    codec_ptr->setInputPacketBuffer(input_buffer, input_buffer_size);
    NVPipeErrorID = codec_ptr->decode(  output_buffer,
                                        *width,
                                        *height,
                                        output_buffer_size,
                                        format);
    // profiling
    nvtxRangePop();
    cudaProfilerStop();

    return NVPipeErrorID;
}
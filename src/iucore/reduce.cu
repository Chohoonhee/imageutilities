#include <iudefs.h>
#include <iucutil.h>
#include <iucore/iutextures.cuh>
#include "transform.cu"


namespace iuprivate {

/* ***************************************************************************
 *  CUDA WRAPPERS
 * ***************************************************************************/

//-----------------------------------------------------------------------------
void cuReduce(iu::ImageGpu_32f_C1* src, iu::ImageGpu_32f_C1* dst,
                  IuInterpolationType interpolation, cudaStream_t stream)
{
  iu::Size<2> src_roi = src->size();
  iu::Size<2> dst_roi = dst->size();

  // x_/y_factor > 0 (for multiplication with dst coords in the kernel!)
  float x_factor = static_cast<float>(src_roi.width) /
      static_cast<float>(dst_roi.width);
  float y_factor = static_cast<float>(src_roi.height) /
      static_cast<float>(dst_roi.height);

  tex1_32f_C1__.addressMode[0] = cudaAddressModeClamp;
  tex1_32f_C1__.addressMode[1] = cudaAddressModeClamp;
  tex1_32f_C1__.normalized = false;

  // bind src image to texture and use as input for reduction
  cudaChannelFormatDesc channel_desc = cudaCreateChannelDesc<float>();
  cudaBindTexture2D(0, &tex1_32f_C1__, src->data(), &channel_desc,
                    src->width(), src->height(), src->pitch());

  // fragmentation
  unsigned int block_size = 16;
  dim3 dimBlock(block_size, block_size);
  dim3 dimGridOut(iu::divUp(dst->width(), dimBlock.x),
                  iu::divUp(dst->height(), dimBlock.y));

  switch(interpolation)
  {
  case IU_INTERPOLATE_NEAREST:
//  case IU_INTERPOLATE_CUBIC:
//    tex1_32f_C1__.filterMode = cudaFilterModePoint;
//    break;
  case IU_INTERPOLATE_LINEAR:
    tex1_32f_C1__.filterMode = cudaFilterModeLinear;
    break;
  }

  switch(interpolation)
  {
  case IU_INTERPOLATE_NEAREST:
  case IU_INTERPOLATE_LINEAR: // fallthrough intended
    cuTransformKernel_32f_C1
        <<< dimGridOut, dimBlock, 0, stream >>> (dst->data(), dst->stride(), dst->width(), dst->height(),
                                      x_factor, y_factor);
    break;
//  case IU_INTERPOLATE_CUBIC:
//    cuTransformCubicKernel_32f_C1
//        <<< dimGridOut, dimBlock, 0, stream >>> (dst->data(), dst->stride(), dst->width(), dst->height(),
//                                      x_factor, y_factor);
//    break;
//  case IU_INTERPOLATE_CUBIC_SPLINE:
//    cuTransformCubicSplineKernel_32f_C1
//        <<< dimGridOut, dimBlock, 0, stream >>> (dst->data(), dst->stride(), dst->width(), dst->height(),
//                                      x_factor, y_factor);
//    break;
  }

  cudaUnbindTexture(&tex1_32f_C1__);

  //iu::checkCudaErrorState(__FILE__, __FUNCTION__, __LINE__);
}



void cuReduce(iu::ImageGpu_32f_C4* src, iu::ImageGpu_32f_C4* dst,
                  IuInterpolationType interpolation, cudaStream_t stream)
{
  iu::Size<2> src_roi = src->size();
  iu::Size<2> dst_roi = dst->size();

  // x_/y_factor > 0 (for multiplication with dst coords in the kernel!)
  float x_factor = static_cast<float>(src_roi.width) /
      static_cast<float>(dst_roi.width);
  float y_factor = static_cast<float>(src_roi.height) /
      static_cast<float>(dst_roi.height);

  tex1_32f_C4__.addressMode[0] = cudaAddressModeClamp;
  tex1_32f_C4__.addressMode[1] = cudaAddressModeClamp;
  tex1_32f_C4__.normalized = false;

  // bind src image to texture and use as input for reduction
  cudaChannelFormatDesc channel_desc = cudaCreateChannelDesc<float4>();
  cudaBindTexture2D(0, &tex1_32f_C4__, src->data(), &channel_desc,
                    src->width(), src->height(), src->pitch());

  // fragmentation
  unsigned int block_size = 16;
  dim3 dimBlock(block_size, block_size);
  dim3 dimGridOut(iu::divUp(dst->width(), dimBlock.x),
                  iu::divUp(dst->height(), dimBlock.y));

  switch(interpolation)
  {
  case IU_INTERPOLATE_NEAREST:
//  case IU_INTERPOLATE_CUBIC:
//    tex1_32f_C1__.filterMode = cudaFilterModePoint;
//    break;
  case IU_INTERPOLATE_LINEAR:
    tex1_32f_C4__.filterMode = cudaFilterModeLinear;
    break;
  }

  switch(interpolation)
  {
  case IU_INTERPOLATE_NEAREST:
  case IU_INTERPOLATE_LINEAR: // fallthrough intended
    cuTransformKernel_32f_C4
        <<< dimGridOut, dimBlock, 0, stream >>> (dst->data(), dst->stride(), dst->width(), dst->height(),
                                      x_factor, y_factor);
    break;
//  case IU_INTERPOLATE_CUBIC:
//    cuTransformCubicKernel_32f_C1
//        <<< dimGridOut, dimBlock, 0, stream >>> (dst->data(), dst->stride(), dst->width(), dst->height(),
//                                      x_factor, y_factor);
//    break;
//  case IU_INTERPOLATE_CUBIC_SPLINE:
//    cuTransformCubicSplineKernel_32f_C1
//        <<< dimGridOut, dimBlock, 0, stream >>> (dst->data(), dst->stride(), dst->width(), dst->height(),
//                                      x_factor, y_factor);
//    break;
  }

  cudaUnbindTexture(&tex1_32f_C4__);

  //iu::checkCudaErrorState(__FILE__, __FUNCTION__, __LINE__);
}

} // namespace iuprivate


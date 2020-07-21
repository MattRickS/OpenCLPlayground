__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
                               CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void GaussianBlur(__read_only image2d_t src, __constant float *mask,
                           __private int maskRadius,
                           __write_only image2d_t dst) {
  const int2 pos = {get_global_id(0), get_global_id(1)};

  float4 sum = 0.0f;
  for (int y = -maskRadius; y <= maskRadius; y++) {
    for (int x = -maskRadius; x <= maskRadius; x++) {
      int maskIdx = (maskRadius * 2 + 1) * (y + maskRadius) + x + maskRadius;
      sum += mask[maskIdx] * read_imagef(src, sampler, pos + (int2)(x, y));
    }
  }

  write_imagef(dst, pos, sum);
}
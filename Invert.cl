__constant sampler_t sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

__kernel void Invert(__read_only image2d_t src, __write_only image2d_t dst) {
  const int2 pos = {get_global_id(0), get_global_id(1)};
  float4 color = read_imagef(src, sampler, pos);
  write_imagef(dst, pos, 1.0f - color);
}
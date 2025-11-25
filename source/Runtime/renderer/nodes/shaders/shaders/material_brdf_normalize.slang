// Normalize Sample Buffer to Texture
// Converts fixed-point atomic accumulation to normalized float texture

RWStructuredBuffer<uint> sample_buffer;  // Input: [weight, count] pairs
RWTexture2D<float4> sample_output;       // Output: normalized texture

struct NormalizeConstants {
    uint resolution;
    uint num_samples;
};

ConstantBuffer<NormalizeConstants> normalize_params;

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadID.xy;
    uint resolution = normalize_params.resolution;
    
    if (any(pixel >= resolution)) {
        return;
    }
    
    uint pixel_index = pixel.y * resolution + pixel.x;
    
    // Read accumulated values
    uint weight_fixed = sample_buffer[pixel_index * 2 + 0];
    uint sample_count = sample_buffer[pixel_index * 2 + 1];
    
    // Convert fixed-point back to float
    float weight = float(weight_fixed) / 65536.0;
    
    // Normalize by sample count
    float normalized_value = 0.0;
    if (sample_count > 0) {
        normalized_value = weight / float(sample_count);
    }
    
    // Output as grayscale (RGB = value, A = sample_count for debugging)
    float debug_count = float(sample_count) / float(normalize_params.num_samples);
    sample_output[pixel] = float4(normalized_value, normalized_value, normalized_value, debug_count);
}

import Scene.SceneTypes;

struct FetchCallableData {
    uint materialBlobID;
    uint material_params_index; // Index into material parameters buffer, set by data fetch callable
    uint shader_type_id;
    VertexInfo vertexInfo;
};

struct EvalCallableData {
    float4 color;
    float3 throughput;
    float3 L;
    float3 V;
    float3 sampledDir;
    float pdf;
    uint seed;
    uint eta_flipped;
    uint material_params_index; // Index into material parameters buffer, set by data fetch callable
    VertexInfo vertexInfo;
};
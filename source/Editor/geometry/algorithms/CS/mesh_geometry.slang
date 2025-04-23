import Scene.SceneTypes;

ConstantBuffer<MeshDesc> mesh;
ByteAddressBuffer g_vertexBuffer;  // Vertex buffer

// Helper function to load a vertex from the vertex buffer
StaticVertexData load_vertex(uint vertex_id)
{
    var vb_offset = mesh.vbOffset;
    var normal_offset = mesh.normalOffset;

    uint actual_vertex_id = vertex_id;
    if (mesh.ibOffset > 0) {
        actual_vertex_id = g_vertexBuffer.Load(mesh.ibOffset + vertex_id * 4);
    }

    float3 pos =
        asfloat(g_vertexBuffer.Load3(3 * actual_vertex_id * 4 + vb_offset));
    float3 normal;

    if (mesh.normalInterpolation == InterpolationType::FaceVarying) {
        normal =
            asfloat(g_vertexBuffer.Load3(3 * vertex_id * 4 + normal_offset));
    }
    else {
        normal = asfloat(
            g_vertexBuffer.Load3(3 * actual_vertex_id * 4 + normal_offset));
    }

    if (normal_offset == 0)
        normal = float3(0, 1, 0);

    float2 texCrd;
    if (mesh.texCrdInterpolation == InterpolationType::FaceVarying) {
        texCrd = asfloat(
            g_vertexBuffer.Load2(2 * vertex_id * 4 + mesh.texCrdOffset));
    }
    else {
        texCrd = asfloat(
            g_vertexBuffer.Load2(2 * actual_vertex_id * 4 + mesh.texCrdOffset));
    }

    uint subset_material_id = -1;
    if (mesh.subsetMatIdOffset != 0) {
        subset_material_id =
            g_vertexBuffer.Load(mesh.subsetMatIdOffset + vertex_id * 4);
    }

    StaticVertexData data = StaticVertexData(
        pos, normal, float4(1, 0, 0, 1), texCrd, 0, subset_material_id);
    return data;
}
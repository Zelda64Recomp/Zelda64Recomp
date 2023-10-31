void VSMain(
    in uint vert_id : SV_VertexID,
    out float4 pos : SV_Position
)
{
    const float2 translation = 0.1f;
    const float2 size = 0.1f;
    float2 extent = float2(vert_id & 1 ? 1.0f : 0.0f, vert_id & 2 ? 1.0f : 0.0f);
    pos = float4(extent * size + translation, 1.0f, 1.0f);
}

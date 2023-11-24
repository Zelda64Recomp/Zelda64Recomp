SamplerState gSampler : register(s1, space0);
Texture2D<float4> gTexture : register(t2, space1);

void PSMain(
    in float4 iColor : COLOR,
    in float2 iUV : TEXCOORD,
    out float4 oColor : SV_TARGET
)
{
    oColor = gTexture.SampleLevel(gSampler, iUV, 0) * iColor;
}

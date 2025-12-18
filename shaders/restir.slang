[[vk::binding(0, 0)]]
Sampler2D<float4> gPosition;
[[vk::binding(1, 0)]]
Sampler2D<float4> gNormal;

struct Light
{
    float4 position; // xyz, w=intensity
    float4 color;    // rgb, w=padding
};

[[vk::binding(2, 0)]]
StructuredBuffer<Light> lights;

[[vk::binding(3, 0)]]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 dispatchThreadID: SV_DispatchThreadID)
{
    int2 pixelCoord = int2(dispatchThreadID.xy);
    uint width, height;
    outputImage.GetDimensions(width, height);
    if (pixelCoord.x >= width || pixelCoord.y >= height) return;
    // use load not sample 
    float4 worldPos = gPosition.Load(int3(pixelCoord, 0));
    float4 normal = gNormal.Load(int3(pixelCoord, 0));

    if (worldPos.w > 0.5)
    {
        Light L = lights[0];

        float3 lightDir = normalize(L.position.xyz - worldPos.xyz);
        float NdotL = max(dot(normalize(normal.xyz), lightDir), 0.0);

        float3 finalColor = L.color.rgb * NdotL;

        outputImage[pixelCoord] = float4(finalColor, 1.0);
    }
    else
    {
        outputImage[pixelCoord] = float4(0.0, 0.0, 0.0, 1.0);
    }
}
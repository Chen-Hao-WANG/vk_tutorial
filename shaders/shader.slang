
struct VSInput
{
    float3 inPosition;
    float3 inColor;
    float2 inTexCoord;
    float3 inNormal;
};

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput
{
    // for rasterization
    float4 svPosition : SV_Position;
    // for ReSTIR world space position
    float3 worldPos;
    float3 fragColor;
    float2 fragTexCoord;
    float3 fragNormal;
};
struct PSOutput
{
    float4 color : SV_Target0;    // swapchain(color)
    float4 worldPos : SV_Target1; // world position
    float4 normal : SV_Target2;   // normal
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    // world position
    float4 worldPos = mul(ubo.model, float4(input.inPosition, 1.0));
    output.worldPos = worldPos.xyz;

    // clip space position
    output.svPosition = mul(ubo.proj, mul(ubo.view, worldPos));
    
    // normal in world space
    output.fragNormal = mul((float3x3)ubo.model, input.inNormal);

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}
Sampler2D texture;

[shader("fragment")]
PSOutput fragMain(VSOutput vertIn) : SV_TARGET
{
    PSOutput output;
    // target 0: color
    float3 albedo = vertIn.fragColor * texture.Sample(vertIn.fragTexCoord).rgb;
    output.color = float4(albedo, 1.0);
    // target 1: world position
    output.worldPos = float4(vertIn.worldPos, 1.0);
    // target 2: normal
    output.normal = float4(vertIn.fragNormal, 1.0);
    return output;
}

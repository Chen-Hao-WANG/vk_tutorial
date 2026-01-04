
struct PushConstants{
    float4x4 modelMatrix;
}
[[vk::push_constant]]
PushConstants pushConstant;
struct VSInput
{
    float3 inPosition;
    float3 inColor;
    float2 inTexCoord;
    float3 inNormal;
};

struct UniformBuffer
{
    float4x4 view;
    float4x4 proj;
};
[[vk::binding(0, 0)]]
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
    float4 color : SV_Target0;    // albedo
    float4 worldPos : SV_Target1; // world position
    float4 normal : SV_Target2;   // normal
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    // world position
    float4 worldPos = mul(pushConstant.modelMatrix, float4(input.inPosition, 1.0));
    output.worldPos = worldPos.xyz;

    // clip space position
    output.svPosition = mul(ubo.proj, mul(ubo.view, worldPos));
    
    // normal in world space
    output.fragNormal = mul((float3x3)pushConstant.modelMatrix, input.inNormal);

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}
[[vk::binding(1, 0)]]
Sampler2D texture;

[shader("fragment")]
PSOutput fragMain(VSOutput vertIn, bool isFrontFace : SV_IsFrontFace) : SV_TARGET{
    PSOutput output;
    // target 0: color
    float3 albedo = vertIn.fragColor * texture.Sample(vertIn.fragTexCoord).rgb;
    output.color = float4(albedo, 1.0);
    // target 1: world position
    output.worldPos = float4(vertIn.worldPos, 1.0);
    // target 2: normal
    float3 N = normalize(vertIn.fragNormal);
    if (!isFrontFace) 
    {
        N = -N;
    }
    output.normal = float4(N, 1.0);
        
    return output;
}

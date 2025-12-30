[[vk::binding(0, 0)]]
Sampler2D<float4> gPosition;
[[vk::binding(1, 0)]]
Sampler2D<float4> gNormal;
[[vk::binding(2, 0)]]
Sampler2D<float4> gAlbedo;
struct Light
{
    float4 position; // xyz, w=intensity
    float4 color;    // rgb, w=padding
};

[[vk::binding(3, 0)]]
StructuredBuffer<Light> lights;

[[vk::binding(4, 0)]]
RWTexture2D<float4> outputImage;
[[vk::binding(5, 0)]]
RaytracingAccelerationStructure tlas;
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
    float3 albedo = gAlbedo.Load(int3(pixelCoord, 0)).rgb;

    if (worldPos.w > 0.5)
    {
        Light L = lights[0];
        
        // Calculate Light Vector
        float3 lightVec = L.position.xyz - worldPos.xyz;
        float lightDist = length(lightVec);
        float3 lightDir = normalize(lightVec);
        
        float NdotL = max(dot(normalize(normal.xyz), lightDir), 0.0);
        
        // --- RAY QUERY SHADOWS START ---
        float shadow = 1.0;

        // Only cast a ray if the surface is facing the light
        if (NdotL > 0.0) 
        {
            // 1. Define the Ray
            RayDesc ray;
            ray.Origin = worldPos.xyz;
            ray.Direction = lightDir;
            ray.TMin = 0.05;          // Small bias to prevent self-shadowing (shadow acne)
            ray.TMax = lightDist;     // Distance to the light source

            // 2. Initialize RayQuery
            // RAY_FLAG_FORCE_OPAQUE: Treat all geometry as opaque (faster)
            // RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH: Stop as soon as we hit *anything* (we only care about visibility)
            RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
            
            // 3. Start Traversal
            q.TraceRayInline(
                tlas,               // The Top Level Acceleration Structure
                RAY_FLAG_NONE,      // Ray flags
                0xFF,               // Instance mask (0xFF matches everything)
                ray                 // The ray definition
            );

            // 4. Execute traversal loop
            q.Proceed();

            // 5. Check result
            // If the committed status is NOT "COMMITTED_NOTHING", it means we hit something blocking the light.
            if (q.CommittedStatus() != COMMITTED_NOTHING)
            {
                shadow = 0.1; // In shadow (ambient only)
            }
        }
        // --- RAY QUERY SHADOWS END ---

        float3 finalColor = albedo * (L.color.rgb * NdotL) * shadow;
        outputImage[pixelCoord] = float4(finalColor, 1.0);
    }
    else
    {
        outputImage[pixelCoord] = float4(0.0, 0.0, 0.0, 1.0);
    }
}
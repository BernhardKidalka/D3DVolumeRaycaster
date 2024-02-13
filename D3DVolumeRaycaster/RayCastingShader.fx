//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: RayCastingShader.fx
// Version: 1.0
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: HLSL
//
// Descrip: vertex- and pixel-shader for ray-casting based 3D MIP (Maximum Intensity Projection) rendering.
//
//------------------------------------------------------------------------------------------------------
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------
// Textures and sampler states
//------------------------------------------------------------------------------------------------------
Texture3D<float>  texVolumeData     : register(t0);
Texture2D<float4> texCubeFrontFaces : register(t1);
Texture2D<float4> texCubeBackFaces  : register(t2); 
SamplerState      linearTexSampler  : register(s0);

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
// consumed by vertex-shader
cbuffer ConstantBufferVS : register(b0)
{
    matrix matrixWVP;
}

// consumed by raycasting and debug pixel-shader
cbuffer ConstantBufferPS : register(b0)
{
    float2 canvasPixResolution;
    float raycastStepSize;
    uint raycastMaxSamples;
}

// consumed by debug pixel-shader only
cbuffer ConstantBufferDebugPS : register(b1)
{
    uint raySetupRenderMode;
}

//--------------------------------------------------------------------------------------
// Struct defining vertex shader output
//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Ray Casting Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS_RAYCASTING(float4 Pos : POSITION)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(Pos, matrixWVP);
    return output;
}

//--------------------------------------------------------------------------------------
// Ray Casting Pixel Shader (3D MIP)
//--------------------------------------------------------------------------------------
float4 PS_RAYCASTING(VS_OUTPUT input) : SV_Target
{
    // calculate 2D texture coordinates in pixel-space for position look-up
    float2 tex = input.Pos.xy * canvasPixResolution;
    // lookup ray entry end exit position in respective 2D textures
    float3 posRayEntry = (float3)texCubeFrontFaces.SampleLevel(linearTexSampler, tex, 0);
    float3 posRayExit = (float3)texCubeBackFaces.SampleLevel(linearTexSampler, tex, 0);

    // calculate normalized ray vector
    float3 vecRayNorm = normalize(posRayExit - posRayEntry);

    // calculate sampling step size
    float3 sampleStep = raycastStepSize * vecRayNorm;

    // start at cube front-face position (we sample front-to-back)
    float3 posData = posRayEntry;

    // initialize MIP value
    float maxSampleValue = 0.0;
    
    for (uint idx = 0; idx < raycastMaxSamples; idx++)
    {
        // note : use the 'SampleLevel' method instead of 'Sample' to avoid gradient calculation to pixel neighborhood for LOD calculations;
        // -> our 3D texture has only one mipmap level so we can set LOD level to zero; 
        // -> LOD = Level Of Detail, used to correctly interpolate between two adjacent mipmaps depending on depth value, if required;
        maxSampleValue = max(maxSampleValue, texVolumeData.SampleLevel(linearTexSampler, posData, 0));
        posData += sampleStep;
    }
    return float4(maxSampleValue, maxSampleValue, maxSampleValue, 1.0);
}

//--------------------------------------------------------------------------------------
// Ray Casting Setup Pixel Shader - intended for producing debug images
// - cube front-faces (ray entry position)
// - cube back-faces  (ray exit position)
// - ray direction vector 
//--------------------------------------------------------------------------------------
float4 PS_RAYSETUP(VS_OUTPUT input) : SV_Target
{
    // calculate 2D texture coordinates in pixel-space for position look-up
    float2 tex = input.Pos.xy * canvasPixResolution;
    // lookup ray entry end exit position in respective 2D textures
    float3 posRayEntry = (float3)texCubeFrontFaces.SampleLevel(linearTexSampler, tex, 0);
    float3 posRayExit = (float3)texCubeBackFaces.SampleLevel(linearTexSampler, tex, 0);

    float4 fragmentColor = float4(1.0, 1.0, 1.0, 1.0);
    
    // shade fragment color depending on debug shader mode ...
    if (1 == raySetupRenderMode)
    {
        fragmentColor = float4(posRayEntry, 1.0);
    }
    else if (2 == raySetupRenderMode)
    {
        fragmentColor = float4(posRayExit, 1.0);
    }
    else
    {
        fragmentColor = float4(posRayExit - posRayEntry, 1.0);
    }
    
    return fragmentColor;
}

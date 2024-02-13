//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode) 
//    File: RaySetupShader.fx
// Version: 1.0
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: HLSL
//
// Descrip: vertex- and pixel-shader for ray casting setup rendering 
//          (render cube back-faces and front-faces textures).
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

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix matrixWVP;
}

//--------------------------------------------------------------------------------------
// Struct defining vertex shader output
//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;   // position in screenspace (pixel-space)
    float3 Tex : TEXCOORD0;     // texture coordinates == position in model space
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(Pos, matrixWVP);
    // shift model space coordinates (-0.5 .. 0.5) to normalized texture coordinates (0.0 .. 1.0)
    output.Tex = Pos.xyz + float3(0.5, 0.5, 0.5);
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    // use interpolated texture coordinates as RGB colors; set alpha channel to 1.0 (full opaque)
    return float4(input.Tex, 1.0);
}

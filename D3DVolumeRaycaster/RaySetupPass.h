//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: RaySetupPass.h
// Version: 1.0 
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: C++
//
// Descrip: include file for implementation of RaySetupPass functionality. Renders cube back-faces and 
//          front-faces to separate render targets, which are then used to calculate ray direction
//          for the ray casting pass.
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
#pragma once

#include "stdafx.h"

namespace D3D11_VOLUME_RAYCASTER
{
    // constant buffer for passing data to HLSL vertex- and pixel-shader
    struct ConstantBuffer
    {
        DirectX::XMMATRIX matrixWVP;    // concatenated world-view-projection matrix
    };

    class RaySetupPass
    {
    public:
        // constructor / desctructor
        RaySetupPass();
        virtual ~RaySetupPass();

        // avoid usage of copy constructor and =operator ...
        RaySetupPass(RaySetupPass const&) = delete;
        RaySetupPass& operator= (RaySetupPass const&) = delete;

        // initialize ray setup pass renderer - create Direct3D resources
        bool Initialize(ID3D11Device* pD3DDevice, UINT canvasWidth, UINT canvasHeight);
        // release all allocated resources 
        void Release();
        // update hook (timing, animation, ...)
        void Update();
        // render back-faces and front-faces of bounding cube to two separate 2D textures
        void Render(ID3D11DeviceContext* pImmediateContext, const DirectX::XMMATRIX* pMatrixWVP, UINT indexCount);
        // resize handler - recreates render targets 
        bool OnResize(ID3D11Device* pD3DDevice, UINT canvasWidth, UINT canvasHeight);
        // get resource views to front-face and back-face texture
        void GetTextureResourceViews(ID3D11ShaderResourceView *texCubeFacesRV[2]);

    private:

        // create Vertex-Shader, Pixel-Shader and input layout objects
        bool createShaderObjectsAndInputLayout(ID3D11Device* pD3DDevice);
        // create constant buffers used to pass uniform data to the shader stages
        bool createConstantBuffers(ID3D11Device* pD3DDevice);
        // create rasterizer state objects for back- and front-face culling
        bool createRasterizerStates(ID3D11Device* pD3DDevice);
        // create texture and corresponding sampler state objects
        bool createTextureResources(ID3D11Device* pD3DDevice, UINT canvasWidth, UINT canvasHeight);

        // ------------------------------------------------------------------------------------------------------------

        // constant buffer for parameter transfer to shader
        ID3D11Buffer*				pConstantBuffer_;
        // vertex and pixel shader for rendering back- and front-faces of cube (bounding box)
        ID3D11VertexShader*         pRaySetupVertexShader_ = nullptr;
        ID3D11InputLayout*          pRaySetupVertexLayout_ = nullptr;
        ID3D11PixelShader*          pRaySetupPixelShader_ = nullptr;
        // 2D texture resources for back- and front-faces
        ID3D11Texture2D*			texCubeFaces_[2];
        ID3D11ShaderResourceView*	texCubeFacesRV_[2];
        ID3D11RenderTargetView*		texCubeFacesRTV_[2];
        // rasterizer states
        ID3D11RasterizerState*		pCullBackRasterizerState_ = nullptr;
        ID3D11RasterizerState*		pCullFrontRasterizerState_ = nullptr;
    };
}

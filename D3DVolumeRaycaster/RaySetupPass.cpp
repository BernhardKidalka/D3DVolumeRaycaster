//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: RaySetupPass.cpp
// Version: 1.0
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: C++
//
// Descrip: implementation of RaySetupPass functionality.
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

#include "stdafx.h"
#include "RaySetupPass.h"

using namespace DirectX;

namespace D3D11_VOLUME_RAYCASTER
{
    //------------------------------------------------------------------------------------------------------
    // Default constructor
    //------------------------------------------------------------------------------------------------------
    RaySetupPass::RaySetupPass()
    {
    }

    //------------------------------------------------------------------------------------------------------
    // Destructor
    //------------------------------------------------------------------------------------------------------
    RaySetupPass::~RaySetupPass()
    {
    }

    //------------------------------------------------------------------------------------------------------
    // Create Vertex-Shader, Pixel-Shader and input layout objects
    //------------------------------------------------------------------------------------------------------
    bool RaySetupPass::createShaderObjectsAndInputLayout(ID3D11Device* pD3DDevice)
    {
        HRESULT hr = S_OK;

        // compile the vertex shader
        ID3DBlob* pVSBlob = nullptr;
        hr = CompileShaderFromFile(L"RaySetupShader.fx", "VS", "vs_5_0", &pVSBlob);
        if (FAILED(hr))
        {
            MessageBox(
                nullptr,
                L"The FX file RaySetupShader.fx cannot be compiled.  Please run this executable from the directory that contains the FX file.",
                L"Error",
                MB_OK
            );
            return false;
        }

        // create the vertex shader
        hr = pD3DDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pRaySetupVertexShader_);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pVSBlob);
            return false;
        }

        // define the input layout
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        UINT numElements = ARRAYSIZE(layoutDesc);

        // create the input layout
        hr = pD3DDevice->CreateInputLayout(
            layoutDesc,
            numElements,
            pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(),
            &pRaySetupVertexLayout_
        );
        SAFE_RELEASE(pVSBlob);
        if (FAILED(hr))
        {
            return false;
        }

        // compile the pixel shader
        ID3DBlob* pPSBlob = nullptr;
        hr = CompileShaderFromFile(L"RaySetupShader.fx", "PS", "ps_5_0", &pPSBlob);
        if (FAILED(hr))
        {
            MessageBox(
                nullptr,
                L"The FX file RaySetupShader.fx cannot be compiled.  Please run this executable from the directory that contains the FX file.",
                L"Error",
                MB_OK);
            return false;
        }

        // create the pixel shader
        hr = pD3DDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pRaySetupPixelShader_);
        SAFE_RELEASE(pPSBlob);
        if (FAILED(hr))
        {
            return false;
        }
        
        return true;
    }

    //------------------------------------------------------------------------------------------------------
    // Create constant buffers used to pass uniform data to the shader stages
    //------------------------------------------------------------------------------------------------------
    bool RaySetupPass::createConstantBuffers(ID3D11Device* pD3DDevice)
    {
        HRESULT hr = S_OK;
        
        // create constant buffer for shader parameter
        D3D11_BUFFER_DESC bufferDsc = { 0 };
        bufferDsc.Usage = D3D11_USAGE_DEFAULT;
        bufferDsc.ByteWidth = sizeof(ConstantBuffer);
        bufferDsc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDsc.CPUAccessFlags = 0;
        hr = pD3DDevice->CreateBuffer(&bufferDsc, NULL, &pConstantBuffer_);
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Create rasterizer state objects for back- and front-face culling
    //------------------------------------------------------------------------------------------------------
    bool RaySetupPass::createRasterizerStates(ID3D11Device* pD3DDevice)
    {
        HRESULT hr = S_OK;

        // create rasterizer state for back-face culling
        D3D11_RASTERIZER_DESC rasterStateDsc;
        ZeroMemory(&rasterStateDsc, sizeof(rasterStateDsc));
        rasterStateDsc.FillMode = D3D11_FILL_SOLID;
        rasterStateDsc.CullMode = D3D11_CULL_BACK;
        rasterStateDsc.DepthClipEnable = true;
        hr = pD3DDevice->CreateRasterizerState(&rasterStateDsc, &pCullBackRasterizerState_);
        if (FAILED(hr))
        {
            return false;
        }

        // create raterizer state for front-face culling
        ZeroMemory(&rasterStateDsc, sizeof(rasterStateDsc));
        rasterStateDsc.FillMode = D3D11_FILL_SOLID;
        rasterStateDsc.CullMode = D3D11_CULL_FRONT;
        rasterStateDsc.DepthClipEnable = true;
        hr = pD3DDevice->CreateRasterizerState(&rasterStateDsc, &pCullFrontRasterizerState_);
        if (FAILED(hr))
        {
            return false;
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // create texture and corresponding sampler state objects
    //------------------------------------------------------------------------------------------------------
    bool RaySetupPass::createTextureResources(ID3D11Device* pD3DDevice, UINT canvasWidth, UINT canvasHeight)
    {
        HRESULT hr = S_OK;

        // create 2D texture resources for back- and front-faces
        D3D11_TEXTURE2D_DESC texDsc = { 0 };
        texDsc.ArraySize = 1;
        texDsc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDsc.Usage = D3D11_USAGE_DEFAULT;
        texDsc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        texDsc.Width = canvasWidth;
        texDsc.Height = canvasHeight;
        texDsc.MipLevels = 1;
        texDsc.SampleDesc.Count = 1;
        texDsc.CPUAccessFlags = 0;

        for (int idx = 0; idx < 2; idx++)
        {
            // create 2D texture
            hr = pD3DDevice->CreateTexture2D(&texDsc, nullptr, &texCubeFaces_[idx]);
            if (FAILED(hr))
            {
                return false;
            }
            // create shader resource view
            hr = pD3DDevice->CreateShaderResourceView(texCubeFaces_[idx], nullptr, &texCubeFacesRV_[idx]);
            if (FAILED(hr))
            {
                return false;
            }
            // create render target view
            hr = pD3DDevice->CreateRenderTargetView(texCubeFaces_[idx], nullptr, &texCubeFacesRTV_[idx]);
            if (FAILED(hr))
            {
                return false;
            }
        }
        
        return true;
    }

    //------------------------------------------------------------------------------------------------------
    // Initialize ray setup pass renderer - create Direct3D resources
    //------------------------------------------------------------------------------------------------------
    bool RaySetupPass::Initialize(ID3D11Device* pD3DDevice, UINT canvasWidth, UINT canvasHeight)
    {
        assert(pD3DDevice);
        assert(canvasWidth > 0);
        assert(canvasHeight > 0);
        
        // create vertex-shader, pixel-shader and input layout
        if (!createShaderObjectsAndInputLayout(pD3DDevice)) return false;
        
        // create constant buffers used for passing uniform data to shader stages
        if (!createConstantBuffers(pD3DDevice)) return false;

        // create needed rasterizer state objects for culling
        if (!createRasterizerStates(pD3DDevice)) return false;
        
        // create all textures with corresponding sampler states
        if (!createTextureResources(pD3DDevice, canvasWidth, canvasHeight)) return false;
        
        return true;
    }

    //------------------------------------------------------------------------------------------------------
    // Release all allocated resources 
    //------------------------------------------------------------------------------------------------------
    void RaySetupPass::Release()
    {
        SAFE_RELEASE(pConstantBuffer_);
        SAFE_RELEASE(pCullFrontRasterizerState_);
        SAFE_RELEASE(pCullBackRasterizerState_);

        for (int idx = 0; idx < 2; idx++)
        {
            SAFE_RELEASE(texCubeFacesRTV_[idx]);
            SAFE_RELEASE(texCubeFacesRV_[idx]);
            SAFE_RELEASE(texCubeFaces_[idx]);
        }

        SAFE_RELEASE(pRaySetupPixelShader_);
        SAFE_RELEASE(pRaySetupVertexLayout_);
        SAFE_RELEASE(pRaySetupVertexShader_);
    }

    //------------------------------------------------------------------------------------------------------
    // Update hook (timing, animation, ...)
    //------------------------------------------------------------------------------------------------------
    void RaySetupPass::Update()
    {
        // currently nothing to do here ...
    }

    //------------------------------------------------------------------------------------------------------
    // Render back-faces and front-faces of bounding cube to two separate 2D textures
    //------------------------------------------------------------------------------------------------------
    void RaySetupPass::Render(ID3D11DeviceContext* pImmediateContext, const XMMATRIX* pMatrixWVP, UINT indexCount)
    {
        // set vertex input layout
        pImmediateContext->IASetInputLayout(pRaySetupVertexLayout_);

        // update constant buffer - pass actual world-view-projection matrix to vertex-shader
        ConstantBuffer cb;
        cb.matrixWVP = *pMatrixWVP;
        pImmediateContext->UpdateSubresource(pConstantBuffer_, 0, nullptr, &cb, 0, 0);

        // set vertex shader
        pImmediateContext->VSSetShader(pRaySetupVertexShader_, nullptr, 0);
        pImmediateContext->VSSetConstantBuffers(0, 1, &pConstantBuffer_);

        // set the pixel shader
        pImmediateContext->PSSetShader(pRaySetupPixelShader_, nullptr, 0);

        // clear render targets
        pImmediateContext->ClearRenderTargetView(texCubeFacesRTV_[0], Colors::Black);
        pImmediateContext->ClearRenderTargetView(texCubeFacesRTV_[1], Colors::Black);

        // front-face culling (render back-faces of cube == ray exit positions)
        pImmediateContext->RSSetState(pCullFrontRasterizerState_);
        pImmediateContext->OMSetRenderTargets(1, &texCubeFacesRTV_[1], nullptr);
        pImmediateContext->DrawIndexed(indexCount, 0, 0);

        // back-face culling (render front-faces of cube == ray entry positions)
        pImmediateContext->RSSetState(pCullBackRasterizerState_);
        pImmediateContext->OMSetRenderTargets(1, &texCubeFacesRTV_[0], nullptr);
        pImmediateContext->DrawIndexed(indexCount, 0, 0);
    }

    //------------------------------------------------------------------------------------------------------
    // Resize handler - recreates texture resources and render targets 
    //------------------------------------------------------------------------------------------------------
    bool RaySetupPass::OnResize(ID3D11Device* pD3DDevice, UINT canvasWidth, UINT canvasHeight)
    {
        assert(pD3DDevice);
        assert(canvasWidth > 0);
        assert(canvasHeight > 0);

        // don't forget to release texture resources before re-creation is initiated!
        for (int idx = 0; idx < 2; idx++)
        {
            SAFE_RELEASE(texCubeFacesRTV_[idx]);
            SAFE_RELEASE(texCubeFacesRV_[idx]);
            SAFE_RELEASE(texCubeFaces_[idx]);
        }

        // re-create textures and render targets with new canvas dimensions
        bool bRetVal = createTextureResources(pD3DDevice, canvasWidth, canvasHeight);

        return bRetVal;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Get resource views to front-face and back-face texture
    //------------------------------------------------------------------------------------------------------
    void RaySetupPass::GetTextureResourceViews(ID3D11ShaderResourceView *texCubeFacesRV[2])
    {
        texCubeFacesRV[0] = texCubeFacesRV_[0];
        texCubeFacesRV[1] = texCubeFacesRV_[1];
    }
}

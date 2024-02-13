//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: RayCastRenderer.cpp
// Version: 1.0
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: C++
//
// Descrip: implementation of the RayCastRenderer functionality.
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
#include "RayCastRenderer.h"

using namespace DirectX;
using namespace std;

namespace D3D11_VOLUME_RAYCASTER
{
    //--------------------------------------------------------------------------------------
    // RayCastRenderer implementation 
    //--------------------------------------------------------------------------------------
    
    //------------------------------------------------------------------------------------------------------
    // Default constructor
    //------------------------------------------------------------------------------------------------------
    RayCastRenderer::RayCastRenderer()
    {
    }

    //------------------------------------------------------------------------------------------------------
    // Destructor
    //------------------------------------------------------------------------------------------------------
    RayCastRenderer::~RayCastRenderer()
    {
    }

    //------------------------------------------------------------------------------------------------------
    // Create Direct3D device, device context and DXGI swapchain
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createDeviceAndSwapChain()
    {
        HRESULT hr = S_OK;

        UINT createDeviceFlags = 0;
//#ifdef _DEBUG
//        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif
        D3D_DRIVER_TYPE driverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        UINT numDriverTypes = ARRAYSIZE(driverTypes);

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };
        UINT numFeatureLevels = ARRAYSIZE(featureLevels);

        // create device and context ...
        for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
        {
            driverType_ = driverTypes[driverTypeIndex];
            hr = D3D11CreateDevice(
                nullptr,
                driverType_,
                nullptr,
                createDeviceFlags,
                featureLevels,
                numFeatureLevels,
                D3D11_SDK_VERSION,
                &pD3DDevice_,
                &featureLevel_,
                &pImmediateContext_
            );
            if (SUCCEEDED(hr))
            {
                break;
            }
        }
        if (FAILED(hr))
        {
            return false;
        }
        
        // obtain DXGI factory from device ...
        IDXGIFactory1* dxgiFactory = nullptr;
        {
            IDXGIDevice* dxgiDevice = nullptr;
            hr = pD3DDevice_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
            if (SUCCEEDED(hr))
            {
                IDXGIAdapter* adapter = nullptr;
                hr = dxgiDevice->GetAdapter(&adapter);
                if (SUCCEEDED(hr))
                {
                    hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                    SAFE_RELEASE(adapter);
                }
                SAFE_RELEASE(dxgiDevice);
            }
        }
        if (FAILED(hr))
        {
            return false;
        }
        
        // create swap chain ...
        DXGI_SWAP_CHAIN_DESC scDesc { 0 };
        scDesc.BufferCount = 1;
        scDesc.BufferDesc.Width = canvasWidth_;
        scDesc.BufferDesc.Height = canvasHeight_;
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.BufferDesc.RefreshRate.Numerator = 60;
        scDesc.BufferDesc.RefreshRate.Denominator = 1;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.OutputWindow = canvasHWND_;
        scDesc.SampleDesc.Count = 1;
        scDesc.SampleDesc.Quality = 0;
        scDesc.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(pD3DDevice_, &scDesc, &pSwapChain_);
        
        if (FAILED(hr))
        {
            return false;
        }

        // the Ray-Caster renderer doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
        // of the associated canvas/window
        dxgiFactory->MakeWindowAssociation(canvasHWND_, DXGI_MWA_NO_ALT_ENTER);
                
        SAFE_RELEASE(dxgiFactory);
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Create render target view and bind it to the Output-Merger stage
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createAndBindRenderTargetView()
    {
        HRESULT hr = S_OK;

        assert(pSwapChain_);
        assert(pD3DDevice_);
        assert(pImmediateContext_);

        // create the render target view attached to the backbuffer of the swap-chain
        ID3D11Texture2D* pBackBuffer = nullptr;
        hr = pSwapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
        if (FAILED(hr))
        {
            return false;
        }

        hr = pD3DDevice_->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView_);
        SAFE_RELEASE(pBackBuffer);
        if (FAILED(hr))
        {
            return false;
        }
        // bind the render target view to the pipeline (Output-Merger stage)
        pImmediateContext_->OMSetRenderTargets(1, &pRenderTargetView_, nullptr);

        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Set the rendering viewport. This method needs to be called on initialization and every time the
    // hosting window is resized.
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::setViewport()
    {
        assert(pImmediateContext_);

        D3D11_VIEWPORT viewPort;
        viewPort.Width = static_cast<FLOAT>(canvasWidth_);
        viewPort.Height = static_cast<FLOAT>(canvasHeight_);
        viewPort.MinDepth = 0.0f;
        viewPort.MaxDepth = 1.0f;
        viewPort.TopLeftX = 0;
        viewPort.TopLeftY = 0;

        pImmediateContext_->RSSetViewports(1, &viewPort);
    }
    
    //------------------------------------------------------------------------------------------------------
    // Create Vertex-Shader, Pixel-Shader and input layout objects.  
    // The creation of the input layout needs access to the vertex shader object (input signature for IA stage); 
    // That's why it is combined with the shader object creation in this method.
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createShaderObjectsAndInputLayout()
    {
        HRESULT hr = S_OK;

        assert(pD3DDevice_);
        assert(pImmediateContext_);
        
        // compile the ray-casting vertex shader
        ID3DBlob* pVSBlob = nullptr;
        hr = CompileShaderFromFile(L"RayCastingShader.fx", "VS_RAYCASTING", "vs_5_0", &pVSBlob);
        if (FAILED(hr))
        {
            MessageBox(
                nullptr,
                L"The FX file RayCastingShader.fx cannot be compiled.  Please run this executable from the directory that contains the FX file.",
                L"Error",
                MB_OK
            );
            return false;
        }

        // create the ray-casting vertex shader
        hr = pD3DDevice_->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pRayCastingVS_);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pVSBlob);
            return false;
        }

        // define the input layout (we need only the POSITION attribute for the vertex)
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        UINT numElements = ARRAYSIZE(layoutDesc);

        // create the input layout
        hr = pD3DDevice_->CreateInputLayout(
            layoutDesc,
            numElements,
            pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(),
            &pVertexLayout_
        );
        SAFE_RELEASE(pVSBlob);
        if (FAILED(hr))
        {
            return false;
        }
        // set the input layout at the Input Assembler stage
        pImmediateContext_->IASetInputLayout(pVertexLayout_);

        // compile the ray-casting pixel shader
        ID3DBlob* pPSBlob = nullptr;
        hr = CompileShaderFromFile(L"RayCastingShader.fx", "PS_RAYCASTING", "ps_5_0", &pPSBlob);
        if (FAILED(hr))
        {
            MessageBox(
                nullptr,
                L"The FX file RayCastingShader.fx cannot be compiled.  Please run this executable from the directory that contains the FX file.",
                L"Error",
                MB_OK);
            return false;
        }

        // create the ray-casting pixel shader
        hr = pD3DDevice_->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pRayCastingPS_);
        SAFE_RELEASE(pPSBlob);
        if (FAILED(hr))
        {
            return false;
        }
        
        // compile the ray-setup debug pixel shader
        hr = CompileShaderFromFile(L"RayCastingShader.fx", "PS_RAYSETUP", "ps_5_0", &pPSBlob);
        if (FAILED(hr))
        {
            MessageBox(
                nullptr,
                L"The FX file RayCastingShader.fx cannot be compiled.  Please run this executable from the directory that contains the FX file.",
                L"Error",
                MB_OK);
            return false;
        }

        // create the ray-setup debug pixel shader
        hr = pD3DDevice_->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pRaySetupDebugPS_);
        SAFE_RELEASE(pPSBlob);
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Create vertex and index buffer used for rendering the proxy geometry (simple cube)  
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createVertexAndIndexBuffer()
    {
        HRESULT hr = S_OK;

        assert(pD3DDevice_);
        assert(pImmediateContext_);
        
        // setup cube geometry for indexed rendering :
        // we need 8 vertices for a cube with 6 faces; each face has 2 triangles (clock-wise winding order)
        // -> 6 * 2 * 3 = 36 indices
        
        // create vertex buffer ...
        VertexPos vertices[] =
        {
            { XMFLOAT4(-0.5f,  0.5f, -0.5f, 1.0f) },
            { XMFLOAT4( 0.5f,  0.5f, -0.5f, 1.0f) },
            { XMFLOAT4( 0.5f,  0.5f,  0.5f, 1.0f) },
            { XMFLOAT4(-0.5f,  0.5f,  0.5f, 1.0f) },
            { XMFLOAT4(-0.5f, -0.5f, -0.5f, 1.0f) },
            { XMFLOAT4( 0.5f, -0.5f, -0.5f, 1.0f) },
            { XMFLOAT4( 0.5f, -0.5f,  0.5f, 1.0f) },
            { XMFLOAT4(-0.5f, -0.5f,  0.5f, 1.0f) }
        };

        vertexCount_ = ARRAYSIZE(vertices);

        D3D11_BUFFER_DESC bufferDesc = { 0 };
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(VertexPos) * vertexCount_;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        
        D3D11_SUBRESOURCE_DATA InitData = { 0 };
        InitData.pSysMem = vertices;
        hr = pD3DDevice_->CreateBuffer(&bufferDesc, &InitData, &pVertexBuffer_);
        if (FAILED(hr))
        {
            return false;
        }
        
        // set vertex buffer
        UINT stride = sizeof(VertexPos);
        UINT offset = 0;
        pImmediateContext_->IASetVertexBuffers(0, 1, &pVertexBuffer_, &stride, &offset);

        // create index buffer ...
        WORD indices[] =
        {
            3,1,0,
            2,1,3,

            0,5,4,
            1,5,0,

            3,4,7,
            0,4,3,

            1,6,5,
            2,6,1,

            2,7,6,
            3,7,2,

            6,4,5,
            7,4,6,
        };
        
        indexCount_ = ARRAYSIZE(indices);

        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(WORD) * indexCount_;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        InitData.pSysMem = indices;
        hr = pD3DDevice_->CreateBuffer(&bufferDesc, &InitData, &pIndexBuffer_);
        if (FAILED(hr))
        {
            return false;
        }
        
        // set index buffer
        pImmediateContext_->IASetIndexBuffer(pIndexBuffer_, DXGI_FORMAT_R16_UINT, 0);

        // set primitive topology
        pImmediateContext_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Create constant buffers used to pass uniform data to the shader stages
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createConstantBuffers()
    {
        HRESULT hr = S_OK;

        assert(pD3DDevice_);

        // create the (empty) constant buffer for VS
        D3D11_BUFFER_DESC bufferDescVS { 0 };
        bufferDescVS.Usage = D3D11_USAGE_DEFAULT;
        bufferDescVS.ByteWidth = sizeof(ConstantBufferVS);
        bufferDescVS.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDescVS.CPUAccessFlags = 0;

        hr = pD3DDevice_->CreateBuffer(&bufferDescVS, nullptr, &pConstantBufferVS_);
        if (FAILED(hr))
        {
            return false;
        }
        
        // create the (empty) constant buffer for PS
        D3D11_BUFFER_DESC bufferDescPS { 0 };
        bufferDescPS.Usage = D3D11_USAGE_DEFAULT;
        bufferDescPS.ByteWidth = sizeof(ConstantBufferPS);
        bufferDescPS.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDescPS.CPUAccessFlags = 0;

        hr = pD3DDevice_->CreateBuffer(&bufferDescPS, nullptr, &pConstantBufferPS_);
        if (FAILED(hr))
        {
            return false;
        }
        
        // create the (empty) constant buffer for debug PS
        D3D11_BUFFER_DESC bufferDescDbgPS { 0 };
        bufferDescDbgPS.Usage = D3D11_USAGE_DEFAULT;
        bufferDescDbgPS.ByteWidth = sizeof(ConstantBufferPS);
        bufferDescDbgPS.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDescDbgPS.CPUAccessFlags = 0;

        hr = pD3DDevice_->CreateBuffer(&bufferDescDbgPS, nullptr, &pConstantBufferDebugPS_);
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------------------------------------
    // Set view matrix (defined through eye position, look-at vector and up-vector) depending on given camera distance
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::setViewMatrix(float cameraDistance)
    {
        // re-initialize the view matrix
        XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, cameraDistance, 0.0f);
        XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        matrixView_ = XMMatrixLookAtLH(Eye, At, Up);
    }
    
    //------------------------------------------------------------------------------------------------------
    // Set the projection matrix. This method needs to be called on initialization and every time the
    // hosting window is resized.
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::setProjectionMatrix()
    {
        matrixProjection_ = XMMatrixPerspectiveFovLH(XM_PIDIV4, canvasWidth_ / static_cast<FLOAT>(canvasHeight_), 0.01f, 10.0f);
    }
    
    //------------------------------------------------------------------------------------------------------
    // Calculate/Update combined World-View-Projection matrix
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::calcWorldViewProjectionMatrix()
    {
        matrixWVP_ = matrixWorld_ * matrixRotate_ * matrixView_ * matrixProjection_; // concatenation order for left-handed coordinate system
    }

    //------------------------------------------------------------------------------------------------------
    // Calculate bounding volume scale matrix depending of dimensions of volume raw data
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::calcScaleMatrix(UINT dimX, UINT dimY, UINT dimZ)
    {
        // get the dimension with maximum value - this will map to the max norm value 1.0f
        UINT maxDimValue = dimX;
        
        if (dimY > maxDimValue)
        {
            maxDimValue = dimY;
        }
        if (dimZ > maxDimValue)
        {
            maxDimValue = dimZ;
        }
        
        float scaleValues[3] = { 1.0f, 1.0f, 1.0f };
        scaleValues[0] = static_cast<float>(dimX) / maxDimValue;
        scaleValues[1] = static_cast<float>(dimY) / maxDimValue;
        scaleValues[2] = static_cast<float>(dimZ) / maxDimValue;
        
        matrixScale_ = XMMatrixScaling(scaleValues[0], scaleValues[1], scaleValues[2]);
    }

    //------------------------------------------------------------------------------------------------------
    // Load volume raw data to internal buffer
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::loadVolumeData(char* dataFileName, UINT volColumns, UINT volRows, UINT volSlices)
    {
        bool bRetVal = false;
        
        ifstream volDataFile(dataFileName, ifstream::in | ifstream::binary);
        volColumns_ = volColumns;
        volRows_ = volRows;
        volSlices_ = volSlices;

        size_t expectedSize = volColumns_ * volRows_ * volSlices_;

        if (volDataFile)
        {
            // get length of file 
            volDataFile.seekg(0, volDataFile.end);
            size_t length = volDataFile.tellg();
            volDataFile.seekg(0, volDataFile.beg);

            assert(length == expectedSize);

            pVolumeData_ = new char[length];

            // read data block
            volDataFile.read(pVolumeData_, length);

            if (volDataFile)
            {
                bRetVal = true;
            }
            volDataFile.close();
        }
        if (bRetVal)
        {
            calcScaleMatrix(volColumns_, volRows_, volSlices_);
        }
        else
        {
            matrixScale_ = XMMatrixIdentity();
        }
        
        return bRetVal;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Create pipeline state objects for the fixed-function units of the Direct3D 11 pipeline
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createPipelineStateObjects()
    {
        HRESULT hr = S_OK;

        assert(pD3DDevice_);

        // create rasterizer state for wireframe rendering (backface culling on)
        D3D11_RASTERIZER_DESC rsDesc;
        ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
        rsDesc.FillMode = D3D11_FILL_WIREFRAME;
        rsDesc.CullMode = D3D11_CULL_BACK;
        rsDesc.FrontCounterClockwise = false;
        rsDesc.DepthClipEnable = true;

        hr = pD3DDevice_->CreateRasterizerState(&rsDesc, &pWireFrameRS_);
        if (FAILED(hr))
        {
            return false;
        }

        // create rasterizer state for wireframe rendering (backface culling off)
        ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
        rsDesc.FillMode = D3D11_FILL_WIREFRAME;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.FrontCounterClockwise = false;
        rsDesc.DepthClipEnable = true;

        hr = pD3DDevice_->CreateRasterizerState(&rsDesc, &pWireFrameNoCullingRS_);
        if (FAILED(hr))
        {
            return false;
        }

        // create rasterizer state for solid rendering (backface culling on)
        ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_BACK;
        rsDesc.FrontCounterClockwise = false;
        rsDesc.DepthClipEnable = true;

        hr = pD3DDevice_->CreateRasterizerState(&rsDesc, &pSolidRS_);
        if (FAILED(hr))
        {
            return false;
        }

        // create rasterizer state for double-sided solid rendering (backface culling off)
        ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.FrontCounterClockwise = false;
        rsDesc.DepthClipEnable = true;

        hr = pD3DDevice_->CreateRasterizerState(&rsDesc, &pSolidNoCullingRS_);
        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------------------------------------
    // Create 3D texture for volume raw data        
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createVolumeTexture()
    {
        HRESULT hr = S_OK;
        
        assert(pD3DDevice_);
        assert(pVolumeData_);

        // create 3D texture for volume data
        D3D11_TEXTURE3D_DESC texDesc { 0 };
        texDesc.Width = volColumns_;
        texDesc.Height = volRows_;
        texDesc.Depth = volSlices_;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8_UNORM;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        // initialize texel data with loaded volume raw data
        D3D11_SUBRESOURCE_DATA tex3DRawData { 0 };
        tex3DRawData.pSysMem = pVolumeData_;
        tex3DRawData.SysMemPitch = volColumns_;                 // -> row pitch in bytes
        tex3DRawData.SysMemSlicePitch = volRows_ * volColumns_; // -> slice pitch in bytes

        hr = pD3DDevice_->CreateTexture3D(&texDesc, &tex3DRawData, &p3DTexture_);
        if (FAILED(hr))
        {
            return false;
        }

        // loaded raw data buffer can be deleted now 
        SAFE_DELETE_ARRAY(pVolumeData_);

        return true;
    }

    //------------------------------------------------------------------------------------------------------
    // Create texture and corresponding sampler state objects
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createTextureAndSamplerObjects()
    {
        HRESULT hr = S_OK;

        assert(pD3DDevice_);

        if (!createVolumeTexture()) return false;

        // create texture sampler state
        D3D11_SAMPLER_DESC samplerDesc;
        ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.BorderColor[0] = 0.0f;
        samplerDesc.BorderColor[1] = 0.0f;
        samplerDesc.BorderColor[2] = 0.0f;
        samplerDesc.BorderColor[3] = 0.0f;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        hr = pD3DDevice_->CreateSamplerState(&samplerDesc, &pLinearTexSamplerState_);
        if (FAILED(hr))
        {
            return false;
        }
        
        pImmediateContext_->PSSetSamplers(0, 1, &pLinearTexSamplerState_);
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Create shader resource views for volume ray casting
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::createRaycastShaderResourceViews()
    {
        HRESULT hr = S_OK;

        assert(pD3DDevice_);

        hr = pD3DDevice_->CreateShaderResourceView(p3DTexture_, nullptr, &pRaycastShaderResView_);
        if (FAILED(hr))
        {
            return false;
        }
        return true;
    }

    //------------------------------------------------------------------------------------------------------
    // Get the camera distance (= z position of camera)
    //------------------------------------------------------------------------------------------------------
    float RayCastRenderer::GetCameraDistance()
    {
        return cameraDistance_;
    }

    //------------------------------------------------------------------------------------------------------
    // Set the camera distance (= z position of camera)
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::SetCameraDistance(float cameraDistance)
    {
        cameraDistance_ = cameraDistance;
        // new camera distance means update of view matrix
        setViewMatrix(cameraDistance);
        // update world-view-projection matrix as view matrix has changed
        calcWorldViewProjectionMatrix();
    }

    //------------------------------------------------------------------------------------------------------
    // GUI callback to get the camera distance
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::guiCallbackGetCameraDistance(void* value, void* clientData)
    {
        *static_cast<float*>(value) = static_cast<RayCastRenderer*>(clientData)->GetCameraDistance();
    }

    //------------------------------------------------------------------------------------------------------
    // GUI callback to set the camera distance
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::guiCallbackSetCameraDistance(const void* value, void* clientData)
    {
        static_cast<RayCastRenderer*>(clientData)->SetCameraDistance(*(const float*)value);
    }
    
    //------------------------------------------------------------------------------------------------------
    // GUI callback for button 'CT Head' click handler -> load demo dataset CT_head_c256_r256_s225.raw
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::guiCallbackBtnDataCTHead(void *clientData)
    {
        if (!static_cast<RayCastRenderer*>(clientData)->LoadDataset(VOLUME_DATASET::CT_HEAD))
        {
            MessageBox(nullptr, L"Unable to load volume dataset CT Head. Ray Casting will fail!", L"Error", MB_OK);
        }
    }
    
    //------------------------------------------------------------------------------------------------------
    // GUI callback for button 'CT Head Angio' click handler -> load demo dataset CTA_c512_r512_s79.raw
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::guiCallbackBtnDataCTHeadAngio(void *clientData)
    {
        if (!static_cast<RayCastRenderer*>(clientData)->LoadDataset(VOLUME_DATASET::CT_HEAD_ANGIO))
        {
            MessageBox(nullptr, L"Unable to load volume dataset CT Head Angio. Ray Casting will fail!", L"Error", MB_OK);
        }
    }

    //------------------------------------------------------------------------------------------------------
    // GUI callback for button 'MR Abdomen' click handler -> load demo dataset MR_abdomen_c384_r512_s80.raw
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::guiCallbackBtnDataMRAbdomen(void *clientData)
    {
        if (!static_cast<RayCastRenderer*>(clientData)->LoadDataset(VOLUME_DATASET::MR_ABDOMEN))
        {
            MessageBox(nullptr, L"Unable to load volume dataset MR Abdomen. Ray Casting will fail!", L"Error", MB_OK);
        }
    }
    
    //------------------------------------------------------------------------------------------------------
    // GUI callback for button 'MR Head TOF Angio' click handler -> load demo dataset MR_TOF_Angio_c416_r512_s112.raw
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::guiCallbackBtnDataMRHeadTOFAngio(void *clientData)
    {
        if (!static_cast<RayCastRenderer*>(clientData)->LoadDataset(VOLUME_DATASET::MR_HEAD_TOF))
        {
            MessageBox(nullptr, L"Unable to load volume dataset MR Head TOF Angio. Ray Casting will fail!", L"Error", MB_OK);
        }
    }
    
    //------------------------------------------------------------------------------------------------------
    // Load given dataset for volume rendering        
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::LoadDataset(VOLUME_DATASET volumeDataset)
    {
        bool retVal = true;
        
        // reset world, scale and rotate matrix
        matrixWorld_ = XMMatrixIdentity();
        matrixScale_ = XMMatrixIdentity();
        matrixRotate_ = XMMatrixIdentity();
        
        // load volume raw data - scale matrix will be correctly initialized depending on volume dimensions
        switch (volumeDataset)
        {
        case VOLUME_DATASET::CT_HEAD :
            if (!loadVolumeData("..\\..\\data\\CT_head_c256_r256_s225.raw", 256, 256, 225)) return false;
            break;
        case VOLUME_DATASET::CT_HEAD_ANGIO :
            if (!loadVolumeData("..\\..\\data\\CTA_c512_r512_s79.raw", 512, 512, 79)) return false;
            break;
        case VOLUME_DATASET::MR_ABDOMEN :
            if (!loadVolumeData("..\\..\\data\\MR_abdomen_c384_r512_s80.raw", 384, 512, 80)) return false;
            break;
        case VOLUME_DATASET::MR_HEAD_TOF :
            if (!loadVolumeData("..\\..\\data\\MR_TOF_Angio_c416_r512_s112.raw", 416, 512, 112)) return false;
            break;
        default:
            if (!loadVolumeData("..\\..\\data\\MR_abdomen_c384_r512_s80.raw", 384, 512, 80)) return false;
            break;
        }
        
        // update world matrix
        matrixWorld_ *= matrixScale_;

        // 3D texture for volume data as well as shader resource view for ray casting shader need to be re-created
        // -> release these resources first
        SAFE_RELEASE(p3DTexture_);
        SAFE_RELEASE(pRaycastShaderResView_);

        // re-create 3D texture
        if (!createVolumeTexture()) return false;

        // re-create shader resource view
        if (!createRaycastShaderResourceViews()) return false;
        
        return retVal;
    }

    //------------------------------------------------------------------------------------------------------
    // Initialize GUI controls
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::initGUI()
    {
        bool retVal = true;

        // initialize AntTweakBar which provides UI controls in combination with Direct3D 11 rendering context
        // see http://anttweakbar.sourceforge.net/doc/
        if (!TwInit(TW_DIRECT3D11, pD3DDevice_))
        {
            return false;
        }

        // provide window canvas dimensions to GUI system
        TwWindowSize(canvasWidth_, canvasHeight_);

        // create a tweak bar
        TwBar *guiBar = TwNewBar("Settings");
        TwDefine(" GLOBAL help='Ray-Caster Renderer Test Viewer' "); // message added to the help bar
        int guiBarSize[2] = { 300, 520 };
        TwSetParam(guiBar, nullptr, "size", TW_PARAM_INT32, 2, guiBarSize);
        
        // rendering settings
        TwAddVarRW(guiBar, "Wireframe Mode", TW_TYPE_BOOLCPP, &renderWireframe_, "group=Rendering key=w");
        TwAddVarRW(guiBar, "Disable Culling", TW_TYPE_BOOLCPP, &disableCulling_, "group=Rendering key=c");
        TwAddSeparator(guiBar, nullptr, "group=Rendering");
        TwAddVarRW(guiBar, "Render Mode", TW_TYPE_UINT32, &renderMode_, "group=Rendering min=0 max=3 keyincr=Right keydecr=Left");
        TwAddButton(guiBar, "CommentRenderMode", nullptr, nullptr, "label='0=MIP,1=Front-Faces,2=Back-Faces,3=Ray Direction' group=Rendering");
        TwAddSeparator(guiBar, nullptr, "group=Rendering");
        TwAddVarCB(
            guiBar, 
            "Camera Distance", 
            TW_TYPE_FLOAT, 
            guiCallbackSetCameraDistance, 
            guiCallbackGetCameraDistance, 
            this, 
            "group=Rendering min=-6 max=-0.75 step=0.01 keyincr=+ keydecr=-");
        TwAddSeparator(guiBar, nullptr, nullptr);
        // raycasting settings
        TwAddVarRW(guiBar, "Sampling Step Size", TW_TYPE_FLOAT, &raycastStepSize_, "group=Ray-Casting min=0.0001 max=0.1 step=0.0001");
        TwAddVarRW(guiBar, "Maximum Samples per Ray", TW_TYPE_UINT32, &raycastMaxSamples_, "group=Ray-Casting min=10 max=800");
        TwAddSeparator(guiBar, nullptr, nullptr);
        // animation settings
        TwAddVarRW(guiBar, "Animate", TW_TYPE_BOOLCPP, &doAnimation_, "group=Animation key=a");
        TwAddVarRW(guiBar, "Animation Speed", TW_TYPE_FLOAT, &animationSpeed_, "group=Animation min=0.0 max=4.0 step=0.01 keyincr=Up keydecr=Down");
        TwAddVarRW(guiBar, "Target Frame-Rate (FPS)", TW_TYPE_UINT32, &targetFPS_, "group=Animation min=5 max=120");
        TwAddVarRW(guiBar, "Lock to Target Frame-Rate", TW_TYPE_BOOLCPP, &lockToTargetFPS_, "group=Animation key=l");
        TwAddVarRW(guiBar, "Rotation", TW_TYPE_QUAT4F, &quatRotation_, "opened=true axisz=-z group=Animation");
        TwAddVarRW(guiBar, "Rotate X", TW_TYPE_BOOLCPP, &rotateX_, "group=Animation key=x");
        TwAddVarRW(guiBar, "Rotate Y", TW_TYPE_BOOLCPP, &rotateY_, "group=Animation key=y");
        TwAddVarRW(guiBar, "Rotate Z", TW_TYPE_BOOLCPP, &rotateZ_, "group=Animation key=z");
        TwAddSeparator(guiBar, nullptr, nullptr);
        // dataset settings
        TwAddButton(guiBar, "CTHead", guiCallbackBtnDataCTHead, this, "group=Dataset label='CT Head'");
        TwAddButton(guiBar, "CTHeadAngio", guiCallbackBtnDataCTHeadAngio, this, "group=Dataset label='CT Head Angio'");
        TwAddButton(guiBar, "MRAbdomen", guiCallbackBtnDataMRAbdomen, this, "group=Dataset label='MR Abdomen'");
        TwAddButton(guiBar, "MRHeadTOF", guiCallbackBtnDataMRHeadTOFAngio, this, "group=Dataset label='MR Head TOF Angio'");

        return retVal;
    }

    //------------------------------------------------------------------------------------------------------
    // Initialize Ray-Caster renderer - create Direct3D device and swap chain
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::Initialize(HWND canvasHWND)
    {
        HRESULT hr = S_OK;
        
        if (!loadVolumeData("..\\..\\data\\MR_TOF_Angio_c416_r512_s112.raw", 416, 512, 112))
        {
            MessageBox(
                nullptr,
                L"Unable to load volume raw data. Ray Casting will fail!",
                L"Error",
                MB_OK
            );
        }
        
        // store window handle of rendering canvas
        canvasHWND_ = canvasHWND;

        RECT rc;
        GetClientRect(canvasHWND_, &rc);
        UINT _canvasWidth = rc.right - rc.left;
        UINT _canvasHeight = rc.bottom - rc.top;
        
        // create Direct3D device, device context and DXGI swapchain
        if (!createDeviceAndSwapChain()) return false;
        
        // create a render target view
        if (!createAndBindRenderTargetView()) return false;

        // set initial viewport
        setViewport();

        // create vertex-shader, pixel-shader and input layout
        if (!createShaderObjectsAndInputLayout()) return false;

        // setup geometry : create vertex and index buffer for indexed rendering
        if (!createVertexAndIndexBuffer()) return false;

        // create constant buffers used for passing uniform data to shader stages
        if (!createConstantBuffers()) return false;
        
        // create pipeline state objects for the fixed-function units of the D3D11 pipeline 
        if (!createPipelineStateObjects()) return false;

        // create all textures with corresponding sampler states
        if (!createTextureAndSamplerObjects()) return false;
        
        // create the shader resource view for ray casting through the volume data
        if (!createRaycastShaderResourceViews()) return false;

        // setup transformation matrices ...
        // note : we work with a left-handed coordinate system
        // -> matrices are row-major aligned
        // -> polygon definition in clockwise (CW) order means front-face; backface culling is ON per default
        // NOTE : HLSL expects column-major aligned matrices; that's why all matrices are transposed before
        //        they are passed to the vertex shader

        // initialize world-view-projection matrix
        matrixWVP_ = XMMatrixIdentity();
        // initialize the world matrix
        matrixWorld_ = XMMatrixIdentity();
        // scale the unit cube to volume boundaries
        matrixWorld_ *= matrixScale_;
        // initialize rotation matrix
        matrixRotate_ = XMMatrixIdentity();

        // initialize the view matrix with initial camera distance
        setViewMatrix(cameraDistance_);

        // initialize the projection matrix
        setProjectionMatrix();
        // update world-view-projection matrix
        calcWorldViewProjectionMatrix();
        
        // initialize the ray setup controller which renders cube back-faces and front-faces to separate render targets
        if (!raySetupPass_.Initialize(pD3DDevice_, _canvasWidth, _canvasHeight)) return false;
        
        // initialize rotation quaternion to identity
        quatRotation_[0] = 0.0f;
        quatRotation_[1] = 0.0f;
        quatRotation_[2] = 0.0f;
        quatRotation_[3] = 1.0f;
        
        // initialize GUI
        if (!initGUI())
        {
            return false;
        }

        // finally initialize performance counters
        QueryPerformanceFrequency(&perfCounterFreq_);
        QueryPerformanceCounter(&lastPerfCounter_);

        return true;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Release all allocated resources 
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::Release()
    {
        // release GUI resources
        TwTerminate();
        // reset pipeline state
        if (pImmediateContext_) pImmediateContext_->ClearState();
        // rlease resources of ray setup controller
        raySetupPass_.Release();
        // release Direct3D COM objects ...
        SAFE_RELEASE(pRaycastShaderResView_);
        SAFE_RELEASE(pLinearTexSamplerState_);
        SAFE_RELEASE(p3DTexture_);
        SAFE_RELEASE(pSolidNoCullingRS_);
        SAFE_RELEASE(pSolidRS_);
        SAFE_RELEASE(pWireFrameNoCullingRS_);
        SAFE_RELEASE(pWireFrameRS_);
        SAFE_RELEASE(pConstantBufferDebugPS_);
        SAFE_RELEASE(pConstantBufferPS_);
        SAFE_RELEASE(pConstantBufferVS_);
        SAFE_RELEASE(pVertexBuffer_);
        SAFE_RELEASE(pIndexBuffer_);
        SAFE_RELEASE(pVertexLayout_);
        SAFE_RELEASE(pRayCastingVS_);
        SAFE_RELEASE(pRayCastingPS_);
        SAFE_RELEASE(pRaySetupDebugPS_);
        SAFE_RELEASE(pRenderTargetView_);
        SAFE_RELEASE(pSwapChain_);
        SAFE_RELEASE(pImmediateContext_);
        SAFE_RELEASE(pD3DDevice_);
        // release internal buffer and other resources
        SAFE_DELETE_ARRAY(pVolumeData_);
    }

    //------------------------------------------------------------------------------------------------------
    // Update hook (timing, animation, ...)
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::Update()
    {
        if (nullptr == pImmediateContext_ || nullptr == pD3DDevice_ || IsIconic(canvasHWND_))
        {
            // renderer not yet initialized or window is minimized - do nothing
            return;
        }
        
        // update target render time first (depends on GUI parameter - relevant for "locked" frame rate rendering)
        targetRenderTime_ = 1.0 / targetFPS_;

        if (lockToTargetFPS_ && deltaTimeMSec_ > 1.0)
        {
            // locked to target frame rate - if we are too fast we let sleep the render thread to "cool down" the GPU
            Sleep(static_cast<DWORD>(deltaTimeMSec_));
        }

        // measure rendering timing (raw frame time in ms + frames per second (FPS) + average FPS) ...
        double frameTime = 0.0;
        QueryPerformanceCounter(&currentPerfCounter_);
        if (lockToTargetFPS_ && deltaTimeMSec_ > 1.0)
        { 
            // frame rate is locked and our hardware would still be capable to render faster than the target render time
            frameTime = targetRenderTime_;
        }
        else
        {
            // hardware renders as fast as possible (maybe also slower as the given target render time) - measure timing
            frameTime = (double)(currentPerfCounter_.QuadPart - lastPerfCounter_.QuadPart) / perfCounterFreq_.QuadPart;
        }
        lastPerfCounter_ = currentPerfCounter_;

        elapsedTime_ += frameTime;              // _elapsedTime since simulation start
        double currentFPS = 1.0 / frameTime;    // current FPS jitters depending on background load and position of bounding cube
                                                // calculate average FPS which gives a smoother measure of rendering performance 
        sumFPS_ += currentFPS;
        frameCounter_++;
        averageFPS_ = sumFPS_ / (frameCounter_ > 0 ? frameCounter_ : 60); // in case of frameCounter overflow we assume 60 FPS
                                                                          // dump timing info in title bar of hosting window
        const size_t bufferSize = 256;
        char charBuffer[bufferSize] = { 0 };
        sprintf_s(
            charBuffer,
            bufferSize,
            "D3D11 Volume Ray-Caster - frame time : %4.2f ms, FPS : %4.1f, average FPS : %4.1f - total time : %4.2f s",
            1000.0f * frameTime,
            currentFPS,
            averageFPS_,
            elapsedTime_);
        SetWindowTextA(canvasHWND_, charBuffer);

        if (doAnimation_) // == auto-rotation mode
        {
            // calculate rotation angle in radians 
            // (based on measured frame time, to ensure consistent animation behaviour on different hardware)
            float rotAngle = XM_PI * static_cast<float>(frameTime) * animationSpeed_;
            // rotate volume depending on GUI settings ...
            if (rotateZ_) matrixRotate_ *= XMMatrixRotationZ(rotAngle);
            if (rotateY_) matrixRotate_ *= XMMatrixRotationY(rotAngle);
            if (rotateX_) matrixRotate_ *= XMMatrixRotationX(rotAngle);
            
            // get rotation quaternion from rotation matrix (needed to update track-ball control in GUI)
            XMVECTOR quatRotation = XMQuaternionRotationMatrix(matrixRotate_);
            quatRotation_[0] = XMVectorGetX(quatRotation);
            quatRotation_[1] = XMVectorGetY(quatRotation);
            quatRotation_[2] = XMVectorGetZ(quatRotation);
            quatRotation_[3] = XMVectorGetW(quatRotation);
        }
        else // manual rotation using trackball control
        {
            // update rotation matrix from rotation quaternion
            XMVECTOR quatRotation = XMVectorSet(quatRotation_[0], quatRotation_[1], quatRotation_[2], quatRotation_[3]);
            matrixRotate_ = XMMatrixRotationQuaternion(quatRotation);
        }
        
        // update world-view-projection matrix
        calcWorldViewProjectionMatrix();

        // update ray controller state
        raySetupPass_.Update();
    }

    //------------------------------------------------------------------------------------------------------
    // Render frame
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::Render()
    {
        if (nullptr == pImmediateContext_ || nullptr == pD3DDevice_ || IsIconic(canvasHWND_))
        {
            // renderer not yet initialized or window is minimized - do nothing
            return;
        }
        if (nullptr == pRayCastingVS_ || nullptr == pRayCastingPS_)
        {
            // shader compilation failed - pipeline is not usable
            return;
        }

        // clear the back buffer
        pImmediateContext_->ClearRenderTargetView(pRenderTargetView_, Colors::Black);

        XMMATRIX transposedMatrixWVP = XMMatrixTranspose(matrixWVP_);

        ///////////////////////////////////////////////////////////////////////
        // ray setup render pass (render results to 2D textures) ...

        // render back- and front-faces textures needed for ray setup
        raySetupPass_.Render(pImmediateContext_, &transposedMatrixWVP, indexCount_);
        // get resource views to back- and front-faces textures needed for ray-casting
        ID3D11ShaderResourceView *texCubeFacesRV[2] = { nullptr, nullptr };
        raySetupPass_.GetTextureResourceViews(texCubeFacesRV);

        ///////////////////////////////////////////////////////////////////////
        // ray-casting render pass ... 

        // bind the default render target view to the pipeline (Output-Merger stage)
        pImmediateContext_->OMSetRenderTargets(1, &pRenderTargetView_, nullptr);
        
        // set rasterizer state to wireframe mode if required
        if (renderWireframe_)
        {
            if (disableCulling_)
            {
                pImmediateContext_->RSSetState(pWireFrameNoCullingRS_);
            }
            else
            {
                pImmediateContext_->RSSetState(pWireFrameRS_);
            }
        }
        else // solid shading with or without backface culling
        {
            if (disableCulling_)
            {
                pImmediateContext_->RSSetState(pSolidNoCullingRS_);
            }
            else
            {
                pImmediateContext_->RSSetState(pSolidRS_);
            }
        }

        // update constant buffer for VS and PS
        ConstantBufferVS cbVS;
        cbVS.matrixWVP = transposedMatrixWVP;
        pImmediateContext_->UpdateSubresource(pConstantBufferVS_, 0, nullptr, &cbVS, 0, 0);

        ConstantBufferPS cbPS;
        cbPS.canvasPixelResolution[0] = 1.0f / canvasWidth_;
        cbPS.canvasPixelResolution[1] = 1.0f / canvasHeight_;
        cbPS.raycastStepSize = raycastStepSize_;
        cbPS.raycastMaxSamples = raycastMaxSamples_;
        pImmediateContext_->UpdateSubresource(pConstantBufferPS_, 0, nullptr, &cbPS, 0, 0);

        // set vertex- and pixel-shader
        pImmediateContext_->VSSetShader(pRayCastingVS_, nullptr, 0);
        pImmediateContext_->VSSetConstantBuffers(0, 1, &pConstantBufferVS_);
        pImmediateContext_->PSSetConstantBuffers(0, 1, &pConstantBufferPS_);

        if (0 == renderMode_) // default render mode : 3D MIP
        {
            pImmediateContext_->PSSetShader(pRayCastingPS_, nullptr, 0);
        }
        else // debug render mode : 1 = front-face, 2 = back-face, 3 = ray vector
        {
            ConstantBufferDebugPS cbDbgPS;
            cbDbgPS.raySetupMode = renderMode_;
            pImmediateContext_->UpdateSubresource(pConstantBufferDebugPS_, 0, nullptr, &cbDbgPS, 0, 0);
            pImmediateContext_->PSSetShader(pRaySetupDebugPS_, nullptr, 0);
            pImmediateContext_->PSSetConstantBuffers(1, 1, &pConstantBufferDebugPS_);
        }

        // set texture resources
        pImmediateContext_->PSSetShaderResources(0, 1, &pRaycastShaderResView_);
        pImmediateContext_->PSSetShaderResources(1, 2, texCubeFacesRV);

        pImmediateContext_->DrawIndexed(indexCount_, 0, 0);
        
        // unbind texture resources
        ID3D11ShaderResourceView* nullResView[3] = { nullptr, nullptr, nullptr };
        pImmediateContext_->PSSetShaderResources(0, 3, nullResView);
        
        // render UI controls
        TwDraw();

        // promote back buffer to front buffer (swap buffers)
        pSwapChain_->Present(0, 0);
        
        postRenderHook();
    }
    
    //------------------------------------------------------------------------------------------------------
    // Post-render hook which is called immediately after frame is rendered
    //------------------------------------------------------------------------------------------------------
    void RayCastRenderer::postRenderHook()
    {
        // post-render timing ...
        QueryPerformanceCounter(&currentPerfCounter_);
        renderTime_ = (double)(currentPerfCounter_.QuadPart - lastPerfCounter_.QuadPart) / perfCounterFreq_.QuadPart;

        if (lockToTargetFPS_)
        {
            deltaTimeMSec_ = (targetRenderTime_ - renderTime_) * 1000.0;
#ifdef _DEBUG
            const size_t bufferSize = 256;
            char charBuffer[bufferSize] = { 0 };
            sprintf_s(
                charBuffer,
                bufferSize,
                "target render time : %4.2f ms, render time : %4.2f ms, delta time : %4.2f ms\n",
                1000.0f * targetRenderTime_,
                1000.0f * renderTime_,
                deltaTimeMSec_);
            OutputDebugStringA(charBuffer);
#endif
        }
    }

    //------------------------------------------------------------------------------------------------------
    // Resize handler - called by the hosting application every time the window size has changed  
    // - resizes swap chain and recreates render target
    // - resets the viewport
    // - resets the projection matrix
    //------------------------------------------------------------------------------------------------------
    bool RayCastRenderer::OnResize()
    {
        HRESULT hr = S_OK;

        // renderer not yet initialized - do nothing
        if (pD3DDevice_ == nullptr) return true;

        RECT rc;
        GetClientRect(canvasHWND_, &rc);
        canvasWidth_ = rc.right - rc.left;
        canvasHeight_ = rc.bottom - rc.top;

        if (0 == canvasWidth_) canvasWidth_ = 1;
        if (0 == canvasHeight_) canvasHeight_ = 1;

        // ensure to release render target view if already initialized
        // -> avoid memory leak
        // -> resize swap chain buffers would fail if associated resources are not yet released!
        SAFE_RELEASE(pRenderTargetView_);

        // resize the swap chain
        hr = pSwapChain_->ResizeBuffers(1, canvasWidth_, canvasHeight_, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        if (FAILED(hr))
        {
            return false;
        }

        // re-create the render target view and bind it to OM stage of Direct3D pipeline
        if (!createAndBindRenderTargetView())
        {
            return false;
        }

        // reset viewport and projection matrix as window size and aspect ratio have changed
        setViewport();
        setProjectionMatrix();
        // update world-view-projection matrix
        calcWorldViewProjectionMatrix();

        bool bRetVal = raySetupPass_.OnResize(pD3DDevice_, canvasWidth_, canvasHeight_);

        return bRetVal;
    }
    
    //------------------------------------------------------------------------------------------------------
    // Message handler callback
    //------------------------------------------------------------------------------------------------------
    int CALLBACK RayCastRenderer::HandleMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // route message to AntTweakBar
        return TwEventWin(wnd, message, wParam, lParam);
    }
}

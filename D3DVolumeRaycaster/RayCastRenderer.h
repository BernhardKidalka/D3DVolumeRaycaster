//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: RayCastRenderer.h
// Version: 1.0 
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: C++
//
// Descrip: include file for implementation of the RayCastRenderer functionality.
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
#include "RaySetupPass.h"
#include "../extern/include/AntTweakBar.h"

namespace D3D11_VOLUME_RAYCASTER
{
    //--------------------------------------------------------------------------------------
    // Structures (vertex formats, constant buffers, ...)
    //--------------------------------------------------------------------------------------
    
    // vertex format : POSITION
    struct VertexPos
    {
        DirectX::XMFLOAT4 Position;
    };

    // constant buffer for passing data to HLSL ray casting vertex-shader
    struct ConstantBufferVS
    {
        DirectX::XMMATRIX matrixWVP;        // concatenated world-view-projection matrix
    };
    
    // constant buffer for passing data to HLSL ray casting pixel-shader
    struct ConstantBufferPS
    {
        float canvasPixelResolution[2];     // pixel-space resolution in x- and y-direction
        float raycastStepSize;              // sampling step size for ray casting
        UINT  raycastMaxSamples;            // maximum number of ray casting samples
    };

    // constant buffer for passing data to HLSL debug pixel-shader
    struct ConstantBufferDebugPS
    {
        UINT raySetupMode;  // debug render mode : 1 = front-face, 2 = back-face, 3 = ray vector
        UINT padding[3];    // pad constant buffer content to 16 byte
    };
    
    // enum for identifying the volume dataset to load
    enum class VOLUME_DATASET
    {
        CT_HEAD = 0,
        CT_HEAD_ANGIO,
        MR_ABDOMEN,
        MR_HEAD_TOF
    };
    
    class RayCastRenderer
    {
    public:
        // constructor / desctructor
        RayCastRenderer(void);
        virtual ~RayCastRenderer();

        // avoid usage of copy constructor and =operator ...
        RayCastRenderer(RayCastRenderer const&) = delete;
        RayCastRenderer& operator= (RayCastRenderer const&) = delete;

        // ------------------------------------------------------------------------------------------------------------
        // API ...
        
        // initialize RayCastRenderer - create Direct3D device and swap chain
        bool Initialize(HWND canvasHWND);
        // release all allocated resources 
        void Release();
        // update hook (timing, animation, ...)
        void Update();
        // render a frame
        void Render();
        // resize handler - resizes swap chain and recreates render target 
        bool OnResize();
        // message handler callback
        int CALLBACK HandleMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);

        // ------------------------------------------------------------------------------------------------------------
        
        // get the camera distance (= z position of camera)
        float GetCameraDistance();
        // set the camera distance (= z position of camera)
        void SetCameraDistance(float cameraDistance);
        // load given dataset for volume rendering        
        bool LoadDataset(VOLUME_DATASET volumeDataset);
        
    protected:

        // create Direct3D device, device context and DXGI swapchain
        bool createDeviceAndSwapChain();
        // create render target view and bind it to the Output-Merger stage
        bool createAndBindRenderTargetView();
        // set rendering viewport; needs to be called on initialization and every time the hosting window is resized
        void setViewport();
        // create Vertex-Shader, Pixel-Shader and input layout objects
        bool createShaderObjectsAndInputLayout();
        // create vertex and index buffer used for rendering the proxy geometry (simple cube)  
        bool createVertexAndIndexBuffer();
        // create constant buffers used to pass uniform data to the shader stages
        bool createConstantBuffers();
        // set view matrix (defined through eye position, look-at vector and up-vector) depending on given camera distance
        void setViewMatrix(float cameraDistance);
        // set projection matrix; needs to be called on initialization and every time the hosting window is resized
        void setProjectionMatrix();
        // calculate/update combined World-View-Projection matrix
        void calcWorldViewProjectionMatrix();
        // load volume raw data to internal buffer
        bool loadVolumeData(char* dataFileName, UINT volColumns, UINT volRows, UINT volSlices);
        // create pipeline state objects for the fixed-function units of the Direct3D 11 pipeline
        bool createPipelineStateObjects();
        // create 3D texture for volume raw data        
        bool createVolumeTexture();
        // create texture and corresponding sampler state objects
        bool createTextureAndSamplerObjects();
        // create shader resource views for volume ray casting
        bool createRaycastShaderResourceViews();
        // calculate bounding volume scale matrix depending of dimensions of volume raw data
        void calcScaleMatrix(UINT dimX, UINT dimY, UINT dimZ);
        // post-render hook which is called immediately after frame is rendered
        void postRenderHook();
        
        // ------------------------------------------------------------------------------------------------------------
        // GUI handling methods/callbacks ...

        // initialize GUI controls
        bool initGUI();
        // GUI callback to get the camera distance
        static void TW_CALL guiCallbackGetCameraDistance(void* value, void* clientData);
        // GUI callback to set the camera distance
        static void TW_CALL guiCallbackSetCameraDistance(const void* value, void* clientData);
        // GUI callback for button 'CT Head' click handler -> load demo dataset CT_head_c256_r256_s225.raw
        static void TW_CALL guiCallbackBtnDataCTHead(void *clientData);
        // GUI callback for button 'CT Head Angio' click handler -> load demo dataset CTA_c512_r512_s79.raw
        static void TW_CALL guiCallbackBtnDataCTHeadAngio(void *clientData);
        // GUI callback for button 'MR Abdomen' click handler -> load demo dataset MR_abdomen_c384_r512_s80.raw
        static void TW_CALL guiCallbackBtnDataMRAbdomen(void *clientData);
        // GUI callback for button 'MR Head TOF Angio' click handler -> load demo dataset MR_TOF_Angio_c416_r512_s112.raw
        static void TW_CALL guiCallbackBtnDataMRHeadTOFAngio(void *clientData);

        // ------------------------------------------------------------------------------------------------------------
        
        HWND                        canvasHWND_ = nullptr;
        UINT                        canvasWidth_ = 1200;
        UINT                        canvasHeight_ = 800;

        D3D_DRIVER_TYPE             driverType_ = D3D_DRIVER_TYPE_NULL;
        D3D_FEATURE_LEVEL           featureLevel_ = D3D_FEATURE_LEVEL_11_0;
        
        ID3D11Device*               pD3DDevice_ = nullptr;
        ID3D11DeviceContext*        pImmediateContext_ = nullptr;
        IDXGISwapChain*             pSwapChain_ = nullptr;
        
        ID3D11RenderTargetView*     pRenderTargetView_ = nullptr;
        
        ID3D11VertexShader*         pRayCastingVS_ = nullptr;
        ID3D11PixelShader*          pRayCastingPS_ = nullptr;
        ID3D11PixelShader*          pRaySetupDebugPS_ = nullptr;

        ID3D11InputLayout*          pVertexLayout_ = nullptr;
        
        ID3D11Buffer*               pVertexBuffer_ = nullptr;
        ID3D11Buffer*               pIndexBuffer_ = nullptr;
        ID3D11Buffer*               pConstantBufferVS_ = nullptr;
        ID3D11Buffer*               pConstantBufferPS_ = nullptr;
        ID3D11Buffer*               pConstantBufferDebugPS_ = nullptr;

        ID3D11Texture3D*            p3DTexture_ = nullptr;
        ID3D11SamplerState*         pLinearTexSamplerState_ = nullptr;
        ID3D11ShaderResourceView*   pRaycastShaderResView_ = nullptr;
        
        ID3D11RasterizerState*      pWireFrameRS_ = nullptr;
        ID3D11RasterizerState*      pWireFrameNoCullingRS_ = nullptr;
        ID3D11RasterizerState*      pSolidRS_ = nullptr;
        ID3D11RasterizerState*      pSolidNoCullingRS_ = nullptr;

        DirectX::XMMATRIX           matrixWorld_;           // needs to be passed every frame on animation/transformation - otherwise constant
        DirectX::XMMATRIX           matrixView_;            // only needs to be passed on view setup changes (e.g. new camera position) 
        DirectX::XMMATRIX           matrixProjection_;      // only needs to be passed when projection params change (view frustum setup, window resize)
        DirectX::XMMATRIX           matrixWVP_;             // concatenated world-view-projection matrix
        DirectX::XMMATRIX           matrixScale_;           // scale matrix used to scale the unit-cube to the dimension ratio of the raw volume data
        DirectX::XMMATRIX           matrixRotate_;          // rotation matrix controlled by rotation quaternion

        UINT                        vertexCount_ = 0;
        UINT                        indexCount_ = 0;
        
        LARGE_INTEGER               perfCounterFreq_;
        LARGE_INTEGER               lastPerfCounter_;
        LARGE_INTEGER               currentPerfCounter_;
        UINT64                      frameCounter_ = 0;
        
        double      elapsedTime_ = 0.0;             // elapsed time since start of rendering
        double      sumFPS_ = 0.0;                  // the summed FPS numbers
        double      averageFPS_ = 0.0;              // average FPS number over elapsed time
        double      renderTime_ = 0.0;              // the measured raw render time for rendering one frame
        UINT        targetFPS_ = 60;                // the target frame-rate to achieve in locked mode
        double      targetRenderTime_ = 1.0 / 60;   // the target render time to achieve the target FPS
        double      deltaTimeMSec_ = 0.0;           // = _targetRenderTime - _renderTime in ms
        bool        lockToTargetFPS_ = false;       // lock-down frame rate to target FPS (default: 60 FPS)

        char*       pVolumeData_ = nullptr;
        UINT        volColumns_ = 1;
        UINT        volRows_ = 1;
        UINT        volSlices_ = 1;

        float       cameraDistance_ = -3.0f;
        bool        renderWireframe_ = false;
        bool        disableCulling_ = false;
        
        bool        doAnimation_ = true;
        bool        rotateX_ = true;
        bool        rotateY_ = true;
        bool        rotateZ_ = false;
        float       animationSpeed_ = 0.5f;
        float       quatRotation_[4];          // quaternion for interactive rotation via trackball control

        float       raycastStepSize_ = 0.003f; // sampling step size for ray casting
        UINT        raycastMaxSamples_ = 550;  // maximum number of ray casting samples
        UINT        renderMode_ = 0;           // render mode : 0 = 3D MIP (default), 1 = front-face, 2 = back-face, 3 = ray vector
        
        RaySetupPass    raySetupPass_;  // the render pass to create the ray vector setup
    };
}

//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: stdafx.cpp
// Version: 1.0
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: C++
//
// Descrip: source file that includes just the standard includes; 
//          D3DVolumeRaycaster.pch will be the pre-compiled header; 
//          stdafx.obj will contain the pre-compiled type information
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

// NOTE: reference any additional headers you need in stdafx.h and not in this file!

namespace D3D11_VOLUME_RAYCASTER
{
    // global utility functions ...

    //------------------------------------------------------------------------------------------------------
    // Compile HLSL shaders with D3DCompile infrastructure
    // note : starting with Visual Studio 2012, it is possible to generate pre-compiled .cso files
    //        (cso = compiled shader object)
    //------------------------------------------------------------------------------------------------------
    HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
    {
        HRESULT hr = S_OK;

        DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
        // Setting this flag improves the shader debugging experience, but still allows 
        // the shaders to be optimized and to run exactly the way they will run in 
        // the release configuration of this program.
        dwShaderFlags |= D3DCOMPILE_DEBUG;

        // Disable optimizations to further improve shader debugging
        dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        ID3DBlob* pErrorBlob = nullptr;
        hr = D3DCompileFromFile(
            szFileName,
            nullptr,
            nullptr,
            szEntryPoint,
            szShaderModel,
            dwShaderFlags,
            0,
            ppBlobOut,
            &pErrorBlob
        );
        if (FAILED(hr))
        {
            if (pErrorBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
                SAFE_RELEASE(pErrorBlob);
            }
            return hr;
        }
        SAFE_RELEASE(pErrorBlob);

        return S_OK;
    }
}

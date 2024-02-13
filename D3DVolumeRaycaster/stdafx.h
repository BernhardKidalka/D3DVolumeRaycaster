//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: stdafx.h
// Version: 1.0
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: C++
//
// Descrip: include file for standard system include files, or project specific include files that 
// are used frequently, but are changed infrequently
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

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// DirectX SDK ... 
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
// standard includes
#include <memory>
#include <fstream>


namespace D3D11_VOLUME_RAYCASTER
{
    // macro definitions ...
    #define SAFE_DELETE(x) { if(x) delete (x); (x) = nullptr; }
    #define SAFE_DELETE_ARRAY(x) { if(x) delete [] (x); (x) = nullptr; }

    #define SAFE_RELEASE(x) { if(x) (x)->Release(); (x) = nullptr; }
    
    // global utility functions ...

    //------------------------------------------------------------------------------------------------------
    // Compile HLSL shaders with D3DCompile infrastructure
    // note : starting with Visual Studio 2012, it is possible to generate pre-compiled .cso files
    //        (cso = compiled shader object)
    //------------------------------------------------------------------------------------------------------
    HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
}

//------------------------------------------------------------------------------------------------------
//
// Project: Direct3D 11 based Volume Ray-Caster (3D MIP rendering mode)
//    File: D3DVolumeRaycasterMain.cpp
// Version: 1.0 
//  Author: B. Kidalka
//    Date: 2024-02-13
//
//    Lang: C++
//
// Descrip: Main entry point for Volume Ray-Caster application.
//
// Note   : based on DirectX SDK sample "Tutorial04 (Rotating Cube)" in section Direct3D11 Tutorials
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

using namespace D3D11_VOLUME_RAYCASTER;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
namespace
{
    std::unique_ptr<RayCastRenderer> g_RayCaster;

    HINSTANCE                        g_hInst = nullptr;
    HWND                             g_hWnd = nullptr;
};

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//--------------------------------------------------------------------------------------
// Register window class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // register window class ...
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"D3DVolumeRaycasterWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_APPLICATION);
    if (!RegisterClassEx(&wcex))
    {
        return E_FAIL;
    }
    // create window ...
    g_hInst = hInstance;
    RECT rc = { 0, 0, 1200, 800 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(
        L"D3DVolumeRaycasterWindowClass", 
        L"D3D11 Volume Ray-Caster",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT, 
        rc.right - rc.left, 
        rc.bottom - rc.top, 
        nullptr, 
        nullptr, 
        hInstance,
        nullptr);

    if (!g_hWnd)
    {
        return E_FAIL;
    }
    
    ShowWindow(g_hWnd, nCmdShow);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Main windows message callback procedure. Handles incoming windows messages
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    
    if (g_RayCaster.get() != nullptr)
    {
        // message routing to Ray-Caster renderer ... (so far hooked for GUI controls)
        if (g_RayCaster->HandleMessage(hWnd, message, wParam, lParam))
        {
            // message alreay handled
            return 0;
        }
    }

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        if (g_RayCaster.get() != nullptr)
        {
            g_RayCaster->Update();
            g_RayCaster->Render();
        }
        EndPaint(hWnd, &ps);
        break;
    case WM_SIZE:
        if (g_RayCaster.get() != nullptr)
        {
            g_RayCaster->OnResize();
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//--------------------------------------------------------------------------------------
// Entry point to the application. Initializes everything and goes into a message 
// processing loop. Idle time is used to render via Ray-Caster.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    g_RayCaster = std::make_unique<RayCastRenderer>();

    if (FAILED(InitWindow(hInstance, nCmdShow)))
    {
        return 0;
    }

    if (!g_RayCaster->Initialize(g_hWnd))
    {
        MessageBox(nullptr, L"Initialization of Ray-Caster Renderer failed!", L"ERROR", MB_OK);
        g_RayCaster->Release();
    }

    // main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_RayCaster->Update();
            g_RayCaster->Render();
        }
    }

    g_RayCaster->Release();

    return (int)msg.wParam;
}

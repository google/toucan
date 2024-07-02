// Copyright 2023 The Toucan Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <api.h>  // generated by generate_bindings

#include <sys/timeb.h>
#include <windows.h>

#include <memory>

#include <webgpu/webgpu_cpp.h>

#include "api_internal.h"

namespace Toucan {

namespace {

void PrintDeviceError(WGPUErrorType, const char* message, void*) { OutputDebugString(message); }

unsigned ToToucanEventModifiers(WPARAM wParam) {
  unsigned result = 0;

  if (wParam & MK_SHIFT) { result |= Shift; }
  if (wParam & MK_CONTROL) { result |= Control; }
  return result;
}

}  // namespace

struct Window {
  Window(HWND w, const uint32_t sz[2]) : wnd(w) {
    size[0] = sz[0];
    size[1] = sz[1];
  }
  HWND     wnd;
  uint32_t size[2];
};

static int gNumWindows = 0;
static uint32_t gScreenSize[2];

static LRESULT CALLBACK mainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  LONG rc = 0L;

  switch (message) {
    case WM_CLOSE:
      ::DestroyWindow(hWnd);
      break;
    case WM_DESTROY:
      if (--gNumWindows == 0) { ::PostQuitMessage(0); }
      break;
    case WM_SIZE:
      if (Window* w = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA))) {
        RECT rect;
        GetWindowRect(hWnd, &rect);
        w->size[0] = rect.right - rect.left;
        w->size[1] = rect.bottom - rect.top;
      }
    default:
      rc = DefWindowProc(hWnd, message, wParam, lParam);
      break;
  }
  return rc;
}

static void registerMainWindowClass() {
  static bool registered = false;
  if (!registered) {
    WNDCLASS wc;
    wc.lpszClassName = "mainWndClass";
    wc.lpfnWndProc = mainWndProc;
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.hInstance = nullptr;
    wc.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = ::GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = "mainMenu";
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    RegisterClass(&wc);
    registered = true;
  }
}

Window* Window_Window(const int32_t* position, const uint32_t* size) {
  registerMainWindowClass();
  RECT  r = {0, 0, static_cast<LONG>(size[0]), static_cast<LONG>(size[1])};
  DWORD style = WS_OVERLAPPEDWINDOW;
  bool  hasMenuBar = false;
  ::AdjustWindowRect(&r, style, hasMenuBar);
  HWND hwnd =
      ::CreateWindow("mainWndClass", "Main Window", style, position[0], position[1],
                     r.right - r.left, r.bottom - r.top, nullptr, nullptr, nullptr, nullptr);
  if (!hwnd) return nullptr;

  ::ShowWindow(hwnd, SW_SHOW);

  Window* w = new Window(hwnd, size);
  SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(w));
  gNumWindows++;
  return w;
}

const uint32_t* Window_GetSize(Window* This) {
  return This->size;
}

const uint32_t* System_GetScreenSize() {
  gScreenSize[0] = static_cast<uint32_t>(GetSystemMetrics(SM_CXSCREEN));
  gScreenSize[1] = static_cast<uint32_t>(GetSystemMetrics(SM_CYSCREEN));
  return gScreenSize;
}

void Window_Destroy(Window* This) { delete This; }

Device* Device_Device() {
  wgpu::Device device = CreateDawnDevice(wgpu::BackendType::D3D12, PrintDeviceError);
  if (!device) { return nullptr; }
  return new Device(device);
}

wgpu::TextureFormat GetPreferredSwapChainFormat() { return wgpu::TextureFormat::BGRA8Unorm; }

SwapChain* SwapChain_SwapChain(int qualifiers, Type* format, Device* device, Window* window) {
  wgpu::SurfaceConfiguration config;
  config.device = device->device;
  config.format = ToDawnTextureFormat(format);
  config.usage = wgpu::TextureUsage::RenderAttachment;
  config.width = window->size[0];
  config.height = window->size[1];
  config.presentMode = wgpu::PresentMode::Fifo;

  wgpu::SurfaceDescriptorFromWindowsHWND winSurfaceDesc;
  winSurfaceDesc.hwnd = window->wnd;
  wgpu::SurfaceDescriptor surfaceDesc;
  surfaceDesc.nextInChain = &winSurfaceDesc;

  static wgpu::Instance instance = wgpu::CreateInstance({});
  wgpu::Surface         surface = instance.CreateSurface(&surfaceDesc);
  surface.Configure(&config);
  return new SwapChain(surface, device->device, {config.width, config.height, 1}, config.format, nullptr);
}

bool System_IsRunning() { return gNumWindows > 0; }

bool System_HasPendingEvents() {
  MSG msg;
  return ::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE) == TRUE;
}

Event* System_GetNextEvent() {
  Event* event = new Event();
  event->type = EventType::Unknown;
  MSG msg;
  GetMessage(&msg, nullptr, 0, 0);
  TranslateMessage(&msg);
  DispatchMessage(&msg);

  LONG rc = 0L;
  event->mousePos[0] = LOWORD(msg.lParam);
  event->mousePos[1] = HIWORD(msg.lParam);
  event->modifiers = ToToucanEventModifiers(msg.wParam);
  event->button = 0;
  switch (msg.message) {
    case WM_LBUTTONDOWN:
      event->button = 0;
      event->type = EventType::MouseDown;
      break;
    case WM_LBUTTONUP:
      event->button = 0;
      event->type = EventType::MouseUp;
      break;
    case WM_RBUTTONDOWN:
      event->button = 2;
      event->type = EventType::MouseDown;
      break;
    case WM_RBUTTONUP:
      event->button = 2;
      event->type = EventType::MouseUp;
      break;
    case WM_MOUSEMOVE: event->type = EventType::MouseMove; break;
    case WM_QUIT: gNumWindows = 0; break;
    default: break;
  }
  return event;
}

double System_GetCurrentTime() {
  struct _timeb t;
  _ftime(&t);
  return static_cast<double>(t.time) + t.millitm / 1000.0;
}

};  // namespace Toucan

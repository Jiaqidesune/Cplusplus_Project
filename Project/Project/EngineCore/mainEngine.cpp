#include <windows.h>
#include <stdint.h>
#include<Xinput.h>

typedef int8_t int8; // 
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8; // unsigned char
typedef uint16_t uint16; //unsigned short
typedef uint32_t uint32; //unsigned int
typedef uint64_t uint64; //long long

#define internal static
#define local_persist static
#define global_variable static 

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

#define X_INPUT_GET_STATE(name) DWORD name(DWORD dwUserIndex,XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(f_x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return 0;
}
global_variable f_x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(f_x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return 0;
}
global_variable f_x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


internal void
Win32LoadXInput(void) {
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if (XInputLibrary) {
        XInputGetState = (f_x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (f_x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

win32_window_dimension
Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return Result;
}

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset)
{
    uint8_t* Row = (uint8_t*)Buffer.Memory;

    for (int Y = 0; Y < Buffer.Height; Y++)
    {
        uint32_t* Pixel = (uint32_t*)Row;
        for (int X = 0; X < Buffer.Width; X++)
        {
            // Memory Order: BB GG RR XX
            // 0xXXRRGGBB
            uint8_t Blue = X + BlueOffset;
            uint8_t Green = Y + GreenOffset;

            *Pixel++ = ((Green << 8) | Blue);

        }
        Row += Buffer.Pitch;
    }
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,
    win32_offscreen_buffer *Buffer)
{
    StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY);
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
    if (Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BytesPerPixel = 4;
    int BitmapMemorysize = Width * Height * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorysize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = BytesPerPixel * Width;
}

LRESULT CALLBACK MainWindowCallback(HWND Window,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT Result = 0;
    switch (Message) {
    case WM_DESTROY:
    {
        GlobalRunning = false;
    } break;

    case WM_CLOSE:
    {
        GlobalRunning = false;
    } break;

    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("ACTIVATEAPP\n");
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32 VKCode = wParam;
        bool WasDown = ((lParam & (1 << 30)) != 0);
        bool IsDown = ((lParam & (1 << 31)) == 0);
        if(WasDown != IsDown)
        {
            switch (VKCode)
            {
            case 'W':
            {
                //OutputDebugStringA("W\n");
            } break;
            case 'D':
            {
                //OutputDebugStringA("D");
            } break;
            }
        }
     
    }

    case WM_PAINT:
    {
        OutputDebugStringA("WM Paint");
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackbuffer);
        EndPaint(Window, &Paint);
    } break;

    default:
    {
        OutputDebugStringA("DEFAULT\n");
        Result = DefWindowProc(Window, Message, wParam, lParam);
    }
    }

    return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CmdLine,
    int CmdShow)
{
    WNDCLASS WindowClass = {};
    Win32LoadXInput();

    //TODO: check if owndc,hredraw,vredraw matter
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    // HICON     hIcon;
    WindowClass.lpszClassName = L"KawaiiDesuneWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
            L"Kawaii Desune",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );
        if (WindowHandle) {

            GlobalRunning = true;
            int XOffset = 0;
            int YOffset = 0;

            HDC DeviceContext = GetDC(WindowHandle);
            Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);

            while (GlobalRunning)
            {
                MSG Message;
                while (PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                // Poll message from controller
                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++) {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                        XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                        int16_t StickX = Pad->sThumbLX;
                        int16_t StickY = Pad->sThumbLY;
                    }
                    else {

                    }
                }

                RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);
                win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackbuffer);

                XOffset++;
                YOffset += 2;
            }
        }
        else {
            //TODO logging
        }
    }
    else {
        //TODO logging
    }

    return 0;
}
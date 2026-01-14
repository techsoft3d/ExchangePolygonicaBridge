#ifdef _WIN32

#include "WindowsWindow.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <locale>

#include <pg/pgrender.h>

WindowsWindow::WindowData WindowsWindow::windowData = { 0, 0, 0, TRUE, 0, "" };

LRESULT CALLBACK WindowsWindow::WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (hWnd == windowData.window) {
        switch (message) {
            case WM_LBUTTONDBLCLK:
                windowData.loopEvents = FALSE;
                break;
                
            case WM_MOUSEMOVE:
                switch (wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) {
                    case MK_LBUTTON:
                        PFViewportOrbit(windowData.vp,
                                      -1 * (LOWORD(lParam) - LOWORD(windowData.mousePos)),
                                      HIWORD(lParam) - HIWORD(windowData.mousePos));
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case MK_RBUTTON:
                        PFViewportZoom(windowData.vp, pow(1.01, HIWORD(lParam) - HIWORD(windowData.mousePos)));
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    // Add more cases as needed...
                }
                windowData.mousePos = lParam;
                break;
                
            case WM_PAINT:
                PFDrawableRender(windowData.drawable, windowData.vp, PV_RENDER_MODE_SOLID);
                if (!windowData.text.empty())
                    SetWindowTextA(windowData.window, windowData.text.c_str());
                break;
                
            case WM_KEYDOWN:
                windowData.loopEvents = FALSE;
                break;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

WindowHandle WindowsWindow::Create(const std::string& caption, int x, int y, int width, int height) {
    LPCWSTR wndClass =L"Polygonica Render Window";
    WNDCLASS wc;
    RECT winsize;
    HINSTANCE hInstance;
    DWORD style;

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WinProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = wndClass;
    
    RegisterClass(&wc);
    
    winsize.left = x;
    winsize.top = y;
    winsize.right = x + width;
    winsize.bottom = y + height;
    
    style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    AdjustWindowRectEx(&winsize, style, FALSE, 0);
    
    hInstance = (HINSTANCE)GetModuleHandle(NULL);
    
	// convert caption to wide string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring  wcaption = converter.from_bytes(caption.c_str());

    HWND window = CreateWindow(wndClass,
		                       wcaption.c_str(),
                              style,
                              winsize.left, winsize.top,
                              (winsize.right - winsize.left),
                              (winsize.bottom - winsize.top),
                              NULL, NULL,
                              hInstance, NULL);
    
    SetWindowPos(window, HWND_TOP,
                winsize.left, winsize.top,
                (winsize.right - winsize.left),
                (winsize.bottom - winsize.top),
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    return window;
}

int WindowsWindow::Register(WindowHandle window, PTDrawable drawable, PTViewport vp) {
    windowData.window = window;
    windowData.drawable = drawable;
    windowData.vp = vp;
    return 0;
}

int WindowsWindow::Mouse(const std::string& text, PTNat32* x, PTNat32* y) {
    if (windowData.window) {
        InvalidateRect(windowData.window, NULL, FALSE);
        windowData.text = text;
        windowData.loopEvents = TRUE;
        while (windowData.loopEvents) {
            MSG msg;
            GetMessage(&msg, windowData.window, 0, 0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            *x = LOWORD(windowData.mousePos);
            *y = HIWORD(windowData.mousePos);
        }
    } else {
        printf("%s", text.c_str());
        if (getchar() == EOF)
            return -1;
    }
    return 0;
}

int WindowsWindow::Text(const std::string& text) {
    PTNat32 dummy_x, dummy_y;
    return Mouse(text, &dummy_x, &dummy_y);
}

PTStatus WindowsWindow::Destroy(WindowHandle window) {
    if (!window)
        return PV_STATUS_BAD_CALL;
        
    if (!DestroyWindow(window))
        return PV_STATUS_BAD_CALL;
        
    return PV_STATUS_OK;
}

int WindowsWindow::DrawableCreate(PTEnvironment environment, WindowHandle window, PTDrawableOpts* options, PTDrawable* drawable)
{
	int status = PFDrawableCreate(environment, window, options, drawable);
	return status;
}



#endif // _WIN32

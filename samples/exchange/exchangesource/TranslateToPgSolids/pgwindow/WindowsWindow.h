#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#ifdef _WIN32

#include "IWindow.h"
#include <windows.h>

// Implementation of IWindow interface for Windows platform

class WindowsWindow : public IWindow {
private:
    struct WindowData {
        HWND window;
        PTDrawable drawable;
        PTViewport vp;
        BOOL loopEvents;
        LPARAM mousePos;
        std::string text;
    };
    
    static WindowData windowData;
    static LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
public:
    WindowHandle Create(const std::string& caption, int x, int y, int width, int height) override;
    int Register(WindowHandle window, PTDrawable drawable, PTViewport vp) override;
    int Mouse(const std::string& text, PTNat32* x, PTNat32* y) override;
    int Text(const std::string& text) override;
    PTStatus Destroy(WindowHandle window) override;
    int DrawableCreate(PTEnvironment environment, WindowHandle window, PTDrawableOpts* options, PTDrawable* drawable) override;

};

#endif // _WIN32
#endif // WINDOWS_WINDOW_H

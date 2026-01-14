#ifndef WINDOW_INTERFACE_H
#define WINDOW_INTERFACE_H

#include "pg/pgrender.h"
#include "pg/pgmacros.h"
#include <string>

// Platform-agnostic window handle type
#ifdef _WIN32
    #include <windows.h>
    using WindowHandle = HWND;
#else
    #include <X11/Xlib.h>
    using WindowHandle = Window;
#endif
#include <memory>

// Cross-platform window interface for polygonica sample code
class IWindow {
public:
    virtual ~IWindow() = default;
    
    // Pure virtual methods that must be implemented by platform-specific classes
    virtual WindowHandle Create(const std::string& caption, int x, int y, int width, int height) = 0;
    virtual int Register(WindowHandle window, PTDrawable drawable, PTViewport vp) = 0;
    virtual int Mouse(const std::string& text, PTNat32* x, PTNat32* y) = 0;
    virtual int Text(const std::string& text) = 0;
    virtual PTStatus Destroy(WindowHandle window) = 0;
    virtual int DrawableCreate( PTEnvironment environment, WindowHandle window,  PTDrawableOpts *options, PTDrawable * drawable) = 0;
};

// Factory function to create platform-specific window implementation
std::unique_ptr<IWindow> CreatePlatformWindow();

#endif // WINDOW_INTERFACE_H

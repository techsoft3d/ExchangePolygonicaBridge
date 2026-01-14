#ifndef X11_WINDOW_H
#define X11_WINDOW_H

#ifndef _WIN32

#include "IWindow.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>

class X11Window : public IWindow {
private:
    struct WindowData {
        Display* display;
        Window window;
        GC gc;
        PTDrawable drawable;
        PTViewport vp;
    };
    
    static WindowData windowData;
    static int scn;
    static XWindowAttributes attribs;
    
    Display* GetDisplay(const char* display);
    int Redraw(const std::string& text);
    
public:
    WindowHandle Create(const std::string& caption, int x, int y, int width, int height) override;
    int Register(WindowHandle window, PTDrawable drawable, PTViewport vp) override;
    int Mouse(const std::string& text, PTNat32* x, PTNat32* y) override;
    int Text(const std::string& text) override;
    PTStatus Destroy(WindowHandle window) override;
    int DrawableCreate( PTEnvironment environment, WindowHandle window,  PTDrawableOpts *options,PTDrawable * drawable);
};

#endif // !_WIN32
#endif // X11_WINDOW_H

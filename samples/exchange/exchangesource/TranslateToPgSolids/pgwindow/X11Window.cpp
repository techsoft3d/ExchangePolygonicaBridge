#ifndef _WIN32

#include "X11Window.h"
#include <string.h>
#include <cmath>

X11Window::WindowData X11Window::windowData = { 0, 0, 0, 0, 0 };
int X11Window::scn = 0;
XWindowAttributes X11Window::attribs;

Display* X11Window::GetDisplay(const char* display) {
    if (!windowData.display)
        windowData.display = XOpenDisplay(display);
    return windowData.display;
}

int X11Window::Redraw(const std::string& text) {
    if (windowData.drawable && windowData.vp) {
        PFDrawableRender(windowData.drawable, windowData.vp, PV_RENDER_MODE_SOLID);
        if (!text.empty())
            XDrawString(windowData.display, windowData.window, windowData.gc, 
                       10, 10, text.c_str(), text.length());
    }
    return 0;
}

WindowHandle X11Window::Create(const std::string& caption, int x, int y, int width, int height) {
    XSizeHints hints;
    Display* display = GetDisplay(NULL);
    
    hints.x = x;
    hints.y = y;
    hints.width = width;
    hints.height = height;
    hints.flags = PPosition | PSize;
    
    windowData.window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                           hints.x, hints.y,
                                           hints.width, hints.height,
                                           7, BlackPixel(display, scn),
                                           WhitePixel(display, scn));
    
    if (!windowData.window) {
        fprintf(stderr, "Failed to create window\n");
        return 0;
    }
    
    XGCValues gcValues;
    XSetStandardProperties(display, windowData.window, caption.c_str(), caption.c_str(), None,
                          NULL, 0, &hints);
    
    const long EVENT_MASK = ExposureMask | ButtonPressMask | ButtonReleaseMask | 
                           ButtonMotionMask | PointerMotionMask | KeyPressMask;
    XSelectInput(display, windowData.window, EVENT_MASK);
    
    gcValues.foreground = WhitePixel(display, scn);
    windowData.gc = XCreateGC(display, windowData.window, GCForeground, &gcValues);
    
    XMapRaised(display, windowData.window);
    XSync(display, FALSE);
    XGetWindowAttributes(display, windowData.window, &attribs);
    
    return windowData.window;
}

int X11Window::Register(WindowHandle window, PTDrawable drawable, PTViewport vp) {
    windowData.window = window;
    windowData.drawable = drawable;
    windowData.vp = vp;
    return 0;
}

int X11Window::Mouse(const std::string& text, PTNat32* x, PTNat32* y) {
    XEvent event;
    int loopEvents = -1;
    Time lastTime = 0;
    const Time dblClickMs = 300;
    
    Redraw(text);
    
    if (windowData.display) {
        while (loopEvents) {
            XNextEvent(windowData.display, &event);
            
            switch (event.type) {
                case KeyPress:
                    loopEvents = 0;
                    break;
                    
                case ButtonPress:
                    if (event.xbutton.button == Button1) {
                        if (event.xbutton.time - lastTime < dblClickMs) {
                            loopEvents = 0;
                            lastTime = 0;
                        } else {
                            lastTime = event.xbutton.time;
                        }
                    }
                    break;
                    
                case MotionNotify:
                    {
                        int deltaX = event.xmotion.x - *x;
                        int deltaY = event.xmotion.y - *y;
                        
                        *x = event.xmotion.x;
                        *y = event.xmotion.y;
                        
                        switch (event.xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
                            case Button1Mask:
                                PFViewportOrbit(windowData.vp, deltaX * -1, deltaY);
                                Redraw(text);
                                break;
                            case Button3Mask:
                                PFViewportZoom(windowData.vp, pow(1.01, deltaY));
                                Redraw(text);
                                break;
                            // Add more cases...
                        }
                    }
                    break;
                    
                case Expose:
                    Redraw(text);
                    break;
            }
        }
    } else {
        printf("%s", text.c_str());
        getchar();
    }
    
    return 0;
}

int X11Window::Text(const std::string& text) {
    PTNat32 dummy_x, dummy_y;
    return Mouse(text, &dummy_x, &dummy_y);
}

PTStatus X11Window::Destroy(WindowHandle window) {
    if (!window || !windowData.display)
        return PV_STATUS_BAD_CALL;
        
    XDestroyWindow(windowData.display, window);
    return PV_STATUS_OK;
}

int X11Window::DrawableCreate( PTEnvironment environment, WindowHandle window,  PTDrawableOpts *options,PTDrawable * drawable)
{
    int status = PV_STATUS_OK;
    status = PFDrawableCreate(environment, GetDisplay(NULL), window, options, drawable);
    return status;
}

#endif // !_WIN32


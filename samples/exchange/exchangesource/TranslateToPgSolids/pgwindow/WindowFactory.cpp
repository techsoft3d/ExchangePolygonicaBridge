#include "IWindow.h"

#ifdef _WIN32
#include "WindowsWindow.h"
#else
#include "X11Window.h"
#endif

std::unique_ptr<IWindow> CreatePlatformWindow() {
#ifdef _WIN32
    return std::make_unique<WindowsWindow>();
#else
    return std::make_unique<X11Window>();
#endif
}

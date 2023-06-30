/*
 *   $Id: //pg/RELEASE/31/FCS/external_demos/examples/include/window.h#1 $
 *
 *   $File: //pg/RELEASE/31/FCS/external_demos/examples/include/window.h $
 *   $Revision: #1 $
 *   $Change: 166877 $
 *   $Author: nigel $
 *   $DateTime: 2021/11/01 15:03:48 $
 *
 *   Description:
 *
 *      'Utility' file for Polygonica examples, to create and handle a window.
 *
 *   Copyright Notice:
 *
 *      $Copyright: MachineWorks Ltd. 1990-2020, 2021$
 *      All rights reserved.
 *
 *      This software and its associated documentation contains proprietary,
 *      confidential and trade secret information of MachineWorks Limited 
 *      and may not be (a) disclosed to third parties, (b) copied in any form,
 *      or (c) used for any purpose except as specifically permitted in 
 *      writing by MachineWorks Ltd.
 *
 *      This software is provided "as is" without express or implied
 *      warranty.
 */

#ifndef PG_WINDOW_HDR
#define PG_WINDOW_HDR

#include "pg/pgrender.h"
#include "pg/pgmacros.h"

#if PG_OS == PG_OS_UNIX

/* Required to get the relevant #includes for the XWindow functions */
#include    <X11/Xlib.h>
#include    <X11/Xutil.h>
#include    <X11/X.h>
#include <string.h>

#endif

#define CHECK_STATUS( fn )                      \
   do {                                         \
      if ( (status = (fn) ) != PV_STATUS_OK )   \
         return status;                         \
   } while (0)

#if PG_OS == PG_OS_WIN
struct _PgWindow {
   HWND _window;
   PTDrawable _drawable;
   PTViewport _vp;
   BOOL _loopEvents;
   LPARAM _mousePos;
   LPCWSTR /*char **/ _text;
} __pgWindowData = { 0, 0, 0, TRUE };

/* Default windows event handler */
static LRESULT CALLBACK winProc(HWND hWnd, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
   if (hWnd == __pgWindowData._window)
   {
      switch (message)
      {
         case WM_LBUTTONDBLCLK:
            __pgWindowData._loopEvents = FALSE;
            break;

         case WM_MOUSEMOVE:
            switch (wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON))
            {
               case MK_LBUTTON:
                  PFViewportOrbit(__pgWindowData._vp,
                                  -1 * (LOWORD(lParam) - LOWORD(__pgWindowData._mousePos)),
                                  HIWORD(lParam) - HIWORD(__pgWindowData._mousePos));
                  InvalidateRect(hWnd, NULL, FALSE);
                  break;
               case MK_RBUTTON:
                  PFViewportZoom(__pgWindowData._vp, pow(1.01, HIWORD(lParam) - HIWORD(__pgWindowData._mousePos)));
                  InvalidateRect(hWnd, NULL, FALSE);
                  break;
               case MK_MBUTTON:
                  {
                     PTPoint        from;
                     PTPoint        to;
                     PTVector       up;
                     PTViewportProj proj;
                     double         fov;
                     RECT           rect;
                     int            small_side;
                     double         units_per_pixel;

                     PFViewportGetPinhole(__pgWindowData._vp, from, to, up, &proj, &fov);
                     GetClientRect(hWnd, &rect);
                     small_side = (rect.right < rect.bottom) ? rect.right : rect.bottom;
                     if (small_side > 0)
                     {
                        if (proj == PV_PROJ_ORTHOGRAPHIC)
                           units_per_pixel = fov / small_side;
                        else
                        {
                           PTVector view_vec;
                           double distance_from_eye_to_focus, pixels_wide_at_focus;
                              
                           PMPointSubtract (view_vec, to, from);
                           distance_from_eye_to_focus = PMVectorLength (view_vec);    
                           pixels_wide_at_focus =  distance_from_eye_to_focus * 2.0 * tan(fov * PG_PI_OVER_ONE_EIGHTY / 2.0);
                           units_per_pixel = pixels_wide_at_focus / small_side;
                        }
                        PFViewportTrack(__pgWindowData._vp,
                                        units_per_pixel * (LOWORD(lParam) - LOWORD(__pgWindowData._mousePos)) * -1,
                                        units_per_pixel * (HIWORD(lParam) - HIWORD(__pgWindowData._mousePos)),
                                        0.0);
                     }
                     InvalidateRect(hWnd, NULL, FALSE);
                     break;
                  }
            }
            __pgWindowData._mousePos = lParam;
            break;

         case WM_PAINT:
            PFDrawableRender(__pgWindowData._drawable, __pgWindowData._vp, PV_RENDER_MODE_SOLID);
            if (__pgWindowData._text)
			   SetWindowText(__pgWindowData._window, __pgWindowData._text);
			break;

         case WM_KEYDOWN:
            __pgWindowData._loopEvents = FALSE;
            break;
      }
   }
   return(DefWindowProc(hWnd, message, wParam, lParam));
}

int PgWindowRegister(HWND window, PTDrawable drawable, PTViewport vp)
{
   __pgWindowData._window = window;
   __pgWindowData._drawable = drawable;
   __pgWindowData._vp = vp;
   return 0;
}

int PgWindowMouse(LPCWSTR /*char **/ text, PTNat32 *x, PTNat32 *y)
{
   if (__pgWindowData._window) {
      InvalidateRect(__pgWindowData._window, NULL, FALSE);
      __pgWindowData._text = text;
      __pgWindowData._loopEvents = TRUE;
      while (__pgWindowData._loopEvents)
      {
         MSG msg;
         GetMessage(&msg, __pgWindowData._window, 0, 0);
         TranslateMessage(&msg);
         DispatchMessage(&msg);
         *x = LOWORD(__pgWindowData._mousePos);
         *y = HIWORD(__pgWindowData._mousePos);
      }
   }
   else {
      printf("%s", text);
      if (getchar() == EOF)
         return -1;
   }
   return 0;
}

/* Destroy the Polygonica window */
PTStatus PgWindowDestroy(HWND window)
{
   if (! window)
      return PV_STATUS_BAD_CALL;

   if (! DestroyWindow(window))
      return PV_STATUS_BAD_CALL;

   return PV_STATUS_OK;
}

/* Create a Polygonica window */
HWND PgWindowCreate(LPCWSTR /*char **/ caption, 
                    PTNat32 x, PTNat32 y,
                    PTNat32 width, PTNat32 height )
{
   HWND      window;
   
   LPCWSTR /*char*/      wnd_class/*[]*/ = L"Polygonica Render Window";
   WNDCLASS  wc;
   RECT        winsize;
   HINSTANCE h_instance;
   DWORD style;

   wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc = winProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = NULL;
   wc.hIcon = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = wnd_class;
  
   RegisterClass(&wc);
  
   winsize.left = x;
   winsize.top = y;
   winsize.right = x + width;
   winsize.bottom = y + height;

   style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  
   AdjustWindowRectEx(&winsize, style, FALSE, 0);

   h_instance = (HINSTANCE) GetModuleHandle( NULL );

   window = CreateWindow(wnd_class,
                         caption,
                         style,
                         winsize.left, winsize.top,
                         (winsize.right - winsize.left),
                         (winsize.bottom - winsize.top),
                         (HWND)NULL, (HMENU)NULL,
                         h_instance, NULL);
  
   SetWindowPos( window, HWND_TOP, 
                 winsize.left, winsize.top,
                 (winsize.right - winsize.left),
                 (winsize.bottom - winsize.top), 
                 SWP_NOACTIVATE | SWP_SHOWWINDOW );

   return window;
}

#endif /* PG_OS == PG_OS_NT */
#if PG_OS == PG_OS_UNIX

#define EVENT_MASK                              \
   ExposureMask |                               \
   ButtonPressMask |                            \
   ButtonReleaseMask |                          \
   ButtonMotionMask |                           \
   PointerMotionMask |                          \
   KeyPressMask                                 \



struct _PgWindow {
   Display * _display;   
   Window _window;
   GC _gc;   
   PTDrawable _drawable;
   PTViewport _vp;
} __pgWindowData = { 0, 0, 0, 0, 0 };

static int scn = 0;
static XWindowAttributes attribs;

Display * PgWindowGetDisplay (char * display)
{
   if (! __pgWindowData._display)
      __pgWindowData._display = XOpenDisplay(display);
   return __pgWindowData._display;   
}


PTStatus PgWindowDestroy(Window window)
{
   if (! window)
      return PV_STATUS_BAD_CALL;

   if (! __pgWindowData._display)
      return PV_STATUS_BAD_CALL;

   XDestroyWindow(__pgWindowData._display, window);

   return PV_STATUS_OK;
}

Window PgWindowCreate(char * caption, 
                      PTNat32 x, PTNat32 y,
                      PTNat32 width, PTNat32 height )
{
   XSizeHints hints;
   Display * display = PgWindowGetDisplay (NULL);  

   hints.x = (int)x;
   hints.y = (int)y;
   hints.width = (uint)width;
   hints.height = (uint)height;
   hints.flags = PPosition | PSize;
   
   __pgWindowData._window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                                hints.x, hints.y,
                                                hints.width, hints.height,
                                                7, BlackPixel(display, scn),
                                                WhitePixel(display, scn));
   if ( !__pgWindowData._window )
   {
      fprintf( stderr, "Failed to raise window\n" );
   }
   else
   {
      XGCValues gcValues;
      
      XSetStandardProperties(display, __pgWindowData._window, caption, caption, None,
                             (char **)NULL, (int)0, &hints);
      
      XSelectInput(display, __pgWindowData._window, EVENT_MASK );
      

      gcValues.foreground = WhitePixel(display, scn);
      __pgWindowData._gc = XCreateGC(display, __pgWindowData._window, GCForeground, &gcValues);
      
      XMapRaised(display, __pgWindowData._window);
      XSync(display, FALSE);
      
      XGetWindowAttributes(display, __pgWindowData._window, &attribs );   
   }
   return __pgWindowData._window;

}



PTStatus PgWindowResize(Window window,
                        PTNat32 new_width,
                        PTNat32 new_height )
{
   XSizeHints hints;
  
   hints.width = (int)new_width;
   hints.height = (int)new_height;
   hints.flags = PSize;



   XResizeWindow( __pgWindowData._display, window, new_width, new_height );
   XSetNormalHints( __pgWindowData._display, window, &hints );
   XSync( __pgWindowData._display, FALSE );


   return PV_STATUS_OK;
}

PTStatus PgWindowMove (Window window,
                       PTNat32  x,
                       PTNat32  y)

{
   XSizeHints hints;

   hints.x = (int)x;
   hints.y = (int)y;
   hints.flags = PPosition;

   XMoveWindow( __pgWindowData._display, window, hints.x, hints.y );
   XSetNormalHints( __pgWindowData._display, window, &hints );
   XSync( __pgWindowData._display, FALSE );

   return PV_STATUS_OK;
}

int PgWindowRegister(Window window, PTDrawable drawable, PTViewport vp)
{
   __pgWindowData._window = window;
   __pgWindowData._drawable = drawable;
   __pgWindowData._vp = vp;
   return 0;
}

static int Redraw (char *text)
{
   if (__pgWindowData._drawable && __pgWindowData._vp)
   {
      PFDrawableRender(__pgWindowData._drawable, __pgWindowData._vp, PV_RENDER_MODE_SOLID);
      if (text)
         XDrawString( __pgWindowData._display, __pgWindowData._window, __pgWindowData._gc, 10, 10, text, strlen(text));
   }
   return 0;   
}

int PgWindowMouse(char * text, PTNat32 *x, PTNat32 *y)
{
   XEvent event;  
   int loopEvents = -1;
   Time lastTime;
   const Time dbl_click_ms = 300;
   
   Redraw (text);
   
   if (__pgWindowData._display)
   {
      while (loopEvents)
      {
         XNextEvent(__pgWindowData._display, &event);
      
         switch (event.type)
         {
            case KeyPress:
               loopEvents = 0;
               break;
            case ButtonPress:
               if (event.xbutton.button == Button1)
               {
                  if (event.xbutton.time - lastTime < dbl_click_ms)
                  {
                     loopEvents = 0;
                     lastTime = 0;
                  }
                  else
                     lastTime = event.xbutton.time;
               }            
               break;
            case MotionNotify:
               {
                  int delta_x = (int) event.xmotion.x - (int) *x;
                  int delta_y = (int) event.xmotion.y - (int) *y;
               
                  *x = event.xmotion.x;
                  *y = event.xmotion.y;
                  switch (event.xmotion.state & (Button1Mask | Button2Mask | Button3Mask))
                  {
                     case Button1Mask:
                        PFViewportOrbit(__pgWindowData._vp, delta_x * -1, delta_y);
                        Redraw (text);                  
                        break;
                     case Button3Mask:
                        PFViewportZoom(__pgWindowData._vp, pow(1.01, delta_y));
                        Redraw (text);                     
                        break;
                     case Button2Mask:
                        {
                           Window root;
                           int x, y;
                           unsigned int width, height;
                           unsigned int border_width;
                           unsigned int depth;
                           Status success = XGetGeometry(__pgWindowData._display, __pgWindowData._window,
                                                         &root, &x, &y, &width, &height, &border_width, &depth);
                           if (success)
                           {
                              PTPoint         from;
                              PTPoint         to;
                              PTVector        up;
                              PTViewportProj proj;
                              double         fov;

                              PFViewportGetPinhole(__pgWindowData._vp, from, to, up, &proj, &fov);
                              int small_side;
                              double units_per_pixel ;
                              small_side = (width < height) ? width : height;
                              if (proj == PV_PROJ_ORTHOGRAPHIC)
                                 units_per_pixel = fov / small_side;
                              else
                              {
                                 PTVector view_vec;
                              
                                 PMPointSubtract (view_vec, to, from);
                                 double distance_from_eye_to_focus = PMVectorLength (view_vec);
                              
                                 double pixels_wide_at_focus =  distance_from_eye_to_focus * 2.0 * tan(fov * PG_PI_OVER_ONE_EIGHTY / 2.0);
                              
                                 units_per_pixel = pixels_wide_at_focus / small_side;
                              }
                           
                           
                              PFViewportTrack(__pgWindowData._vp,
                                              units_per_pixel * delta_x * -1,
                                              units_per_pixel * delta_y,
                                              0.0);
                     
                     
                              Redraw (text);
                           }
                        }
                        break;                  
                  }
               }
               break;
            case Expose:
               Redraw (text);
               break;
         }
      }
   }
   else {
      printf("%s", text);
      getchar();
   }
   
   return 0;   
}


#endif /* PG_OS == PG_OS_UNIX */

int PgWindowText(LPCWSTR /*char **/text)
{
   PTNat32 dummy_x, dummy_y;

   return PgWindowMouse (text, &dummy_x, &dummy_y);  
}

/* Setup SHOW_ERROR_MESSAGE now, before 'error.h' tries to set it to printf */
#ifndef SHOW_ERROR_MESSAGE
#define SHOW_ERROR_MESSAGE PgWindowText
#endif /* SHOW_ERROR_MESSAGE */

#include "./error_wide_strings.h"

#endif /* PG_WINDOW_HDR */

/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "win.h"
#include "user_private.h"
#include "controls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nonclient);

#define SC_ABOUTWINE            (SC_SCREENSAVE+1)


static void adjust_window_rect( RECT *rect, DWORD style, BOOL menu, DWORD exStyle, NONCLIENTMETRICSW *ncm )
{
    int adjust = 0;

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE)
        adjust = 1; /* for the outer frame always present */
    else if ((exStyle & WS_EX_DLGMODALFRAME) || (style & (WS_THICKFRAME|WS_DLGFRAME)))
        adjust = 2; /* outer */

    if (style & WS_THICKFRAME)
        adjust += ncm->iBorderWidth + ncm->iPaddedBorderWidth; /* The resize border */

    if ((style & (WS_BORDER|WS_DLGFRAME)) || (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= ncm->iSmCaptionHeight + 1;
        else
            rect->top -= ncm->iCaptionHeight + 1;
    }
    if (menu) rect->top -= ncm->iMenuHeight + 1;

    if (exStyle & WS_EX_CLIENTEDGE)
        InflateRect(rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
}


/***********************************************************************
 *		DrawCaption (USER32.@) Draws a caption bar
 */
BOOL WINAPI DrawCaption( HWND hwnd, HDC hdc, const RECT *rect, UINT flags )
{
    return NtUserDrawCaptionTemp( hwnd, hdc, rect, 0, 0, NULL, flags & 0x103f );
}


/***********************************************************************
 *		DrawCaptionTempA (USER32.@)
 */
BOOL WINAPI DrawCaptionTempA (HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont,
                              HICON hIcon, LPCSTR str, UINT uFlags)
{
    LPWSTR strW;
    INT len;
    BOOL ret = FALSE;

    if (!(uFlags & DC_TEXT) || !str)
        return NtUserDrawCaptionTemp( hwnd, hdc, rect, hFont, hIcon, NULL, uFlags );

    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    if ((strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, str, -1, strW, len );
        ret = NtUserDrawCaptionTemp( hwnd, hdc, rect, hFont, hIcon, strW, uFlags );
        HeapFree( GetProcessHeap (), 0, strW );
    }
    return ret;
}


/***********************************************************************
 *		AdjustWindowRect (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    return AdjustWindowRectEx( rect, style, menu, 0 );
}


/***********************************************************************
 *		AdjustWindowRectEx (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AdjustWindowRectEx( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
    NONCLIENTMETRICSW ncm;

    TRACE("(%s) %08lx %d %08lx\n", wine_dbgstr_rect(rect), style, menu, exStyle );

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );

    adjust_window_rect( rect, style, menu, exStyle, &ncm );
    return TRUE;
}


/***********************************************************************
 *		AdjustWindowRectExForDpi (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AdjustWindowRectExForDpi( LPRECT rect, DWORD style, BOOL menu,
                                                        DWORD exStyle, UINT dpi )
{
    NONCLIENTMETRICSW ncm;

    TRACE("(%s) %08lx %d %08lx %u\n", wine_dbgstr_rect(rect), style, menu, exStyle, dpi );

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );

    adjust_window_rect( rect, style, menu, exStyle, &ncm );
    return TRUE;
}


LRESULT NC_HandleNCMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    POINT pt;

    TRACE("hwnd=%p wparam=%#Ix lparam=%#Ix\n", hwnd, wParam, lParam);

    if (wParam != HTHSCROLL && wParam != HTVSCROLL)
        return 0;

    WIN_GetRectangles(hwnd, COORDS_CLIENT, &rect, NULL);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    ScreenToClient(hwnd, &pt);
    pt.x -= rect.left;
    pt.y -= rect.top;
    SCROLL_HandleScrollEvent(hwnd, wParam == HTHSCROLL ? SB_HORZ : SB_VERT, WM_NCMOUSEMOVE, pt);
    return 0;
}

LRESULT NC_HandleNCMouseLeave(HWND hwnd)
{
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    POINT pt = {0, 0};

    TRACE("hwnd=%p\n", hwnd);

    if (style & WS_HSCROLL)
        SCROLL_HandleScrollEvent(hwnd, SB_HORZ, WM_NCMOUSELEAVE, pt);
    if (style & WS_VSCROLL)
        SCROLL_HandleScrollEvent(hwnd, SB_VERT, WM_NCMOUSELEAVE, pt);

    return 0;
}


/***********************************************************************
 *           NC_TrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void NC_TrackScrollBar( HWND hwnd, WPARAM wParam, POINT pt )
{
    INT scrollbar;

    if ((wParam & 0xfff0) == SC_HSCROLL)
    {
        if ((wParam & 0x0f) != HTHSCROLL) return;
	scrollbar = SB_HORZ;
    }
    else  /* SC_VSCROLL */
    {
        if ((wParam & 0x0f) != HTVSCROLL) return;
	scrollbar = SB_VERT;
    }
    SCROLL_TrackScrollBar( hwnd, scrollbar, pt );
}


/***********************************************************************
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LRESULT NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if (!NtUserMessageCall( hwnd, WM_SYSCOMMAND, wParam, lParam, 0, NtUserDefWindowProc, FALSE ))
        return 0;

    switch (wParam & 0xfff0)
    {
    case SC_VSCROLL:
    case SC_HSCROLL:
        {
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            NC_TrackScrollBar( hwnd, wParam, pt );
        }
        break;

    case SC_TASKLIST:
        WinExec( "taskman.exe", SW_SHOWNORMAL );
        break;

    case SC_SCREENSAVE:
        if (wParam == SC_ABOUTWINE)
        {
            HMODULE hmodule = LoadLibraryA( "shell32.dll" );
            if (hmodule)
            {
                BOOL (WINAPI *aboutproc)(HWND, LPCSTR, LPCSTR, HICON);
                extern const char * CDECL wine_get_version(void);
                char app[256];

                sprintf( app, "Wine %s", wine_get_version() );
                aboutproc = (void *)GetProcAddress( hmodule, "ShellAboutA" );
                if (aboutproc) aboutproc( hwnd, app, NULL, 0 );
                FreeLibrary( hmodule );
            }
        }
        break;
    }
    return 0;
}

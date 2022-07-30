#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <shobjidl.h>
#include <d2d1.h>

ID2D1Factory* pFactory;
ID2D1HwndRenderTarget* pRenderTarget;
ID2D1SolidColorBrush* pBrush;
D2D1_POINT_2F zero;
D2D1_POINT_2F xEnd;
D2D1_POINT_2F yEnd;

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

void    CalculateLayout();
HRESULT CreateGraphicsResources(HWND m_hwnd);
void    DiscardGraphicsResources();
void    OnPaint(HWND m_hwnd);
void    Resize(HWND m_hwnd);

 
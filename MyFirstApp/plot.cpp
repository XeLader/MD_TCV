#include <d2d1.h>

ID2D1Factory* pFactory;
ID2D1HwndRenderTarget* pRenderTarget;
ID2D1SolidColorBrush* pBrush;
D2D1_POINT_2F zero;
D2D1_POINT_2F xEnd;
D2D1_POINT_2F yEnd;


void    CalculateLayout();
HRESULT CreateGraphicsResources(HWND m_hwnd);
void    DiscardGraphicsResources();
void    OnPaint(HWND m_hwnd);
void    Resize(HWND m_hwnd);

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

void CalculateLayout()
{
    if (pRenderTarget != NULL)
    {
        D2D1_SIZE_F size = pRenderTarget->GetSize();
        const float x = size.width;
        const float y = size.height;
        zero = D2D1::Point2F(20.0, y - 20);
        xEnd = D2D1::Point2F(x - 10.0, y - 20);
        yEnd = D2D1::Point2F(20, 10);
    }
}

HRESULT CreateGraphicsResources(HWND m_hwnd)
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(0.3f, 0.3f, 0.3f);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

            if (SUCCEEDED(hr))
            {
                CalculateLayout();
            }
        }
    }
    return hr;
}

void DiscardGraphicsResources() {
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

void OnPaint(HWND m_hwnd) {
    HRESULT hr = CreateGraphicsResources(m_hwnd);
    if (SUCCEEDED(hr))
    {
        pRenderTarget->BeginDraw();

        pRenderTarget->DrawLine(zero, xEnd, pBrush, 0.5f);
        pRenderTarget->DrawLine(zero, yEnd, pBrush, 0.5f);

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
    }
}

void Resize(HWND m_hwnd) {
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);
        CalculateLayout();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}


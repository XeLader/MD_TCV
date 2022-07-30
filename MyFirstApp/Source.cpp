// HelloWindowsDesktop.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite")

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <shobjidl.h>
#include <d2d1.h>
#include <dwrite.h>
#include <commdlg.h>
#include <winuser.h>
#include <windowsx.h>
#include "resource.h"
#include "Edit.h"



// Global variables
HRESULT hr;
IFileOpenDialog* pFileOpen;
IFileSaveDialog* pFileSave;
IShellItem* pItem;
TCHAR AttenText[] = _T("Choose correlation file");
TCHAR PathText[] = _T("You've chosen:");
PWSTR pszFilePath;
FILE* fp;
FILE* fr;
ID2D1Factory* pFactory;
IDWriteFactory* pDWriteFactory;
IDWriteTextFormat* pTextFormat, *pTextSmallFormat;
ID2D1HwndRenderTarget* pRenderTarget;
ID2D1SolidColorBrush* pBrush;
D2D1_POINT_2F zero, xEnd, yEnd;

static const WCHAR msc_fontName[] = L"Verdana";
static const FLOAT msc_fontSize = 36;

float* JJ;
int N,s,p;
BOOL datatype = FALSE;
double dt=0, T, V;
TCHAR dtstr[16],Tstr[32], Vstr[32], TCoV[64];
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

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("TransFlux PRO");

// Stored instance handle for use in Win32 API calls such as FindResource
HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SetTDProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("TransFlux PRO by sanya XD"),
            NULL);

        return 1;
    }

    //Initialize COM library 
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | 
    COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
       // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindowEx explained:
    // WS_EX_OVERLAPPEDWINDOW : An optional extended window style.
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 700,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("TransFlux PRO by sanya XD"),
            NULL);

        return 1;
    }

    //Create OPEN button
    HWND hwndButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"OPEN correlation data",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10,         // x position 
        10,         // y position 
        170,        // Button width
        50,        // Button height
        hWnd,     // Parent window
        (HMENU) 1,       //
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);      // Pointer not needed.

    //Create SAVE button
    HWND hwndCrButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"CREATE file for Excel",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        200,         // x position 
        10,         // y position 
        170,        // Button width
        50,        // Button height
        hWnd,     // Parent window
        (HMENU)2,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);      // Pointer not needed.

    //Create button to set timestep
    HWND hwndTdButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"SET parameters",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        390,         // x position 
        10,         // y position 
        170,        // Button width
        50,        // Button height
        hWnd,     // Parent window
        (HMENU)3,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);      // Pointer not needed.


    // Create radio button
    HWND hwndButtonRad = CreateWindow(
    L"BUTTON",  // Predefined class; Unicode assumed 
        L"Therm.Cond",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,  // Styles 
        577,         // x position 
        10,         // y position 
        117,        // Button width
        35,        // Button height
        hWnd,     // Parent window
        (HMENU)5,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
     HWND hwndButtonRad2 = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Viscosity",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
        577,         // x position
        35,         // y position 
        117,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        (HMENU)6,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

     // Create group box
     HWND hwndButtonGroup = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Data type:",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX | WS_GROUP,  // Styles 
         575,         // x position 
         3,         // y position 
         120,        // Button width
         58,        // Button height
         hWnd,     // Parent window
         (HMENU)4,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     CheckRadioButton(hWnd, 5, 6, 5);

     HWND hControl = CreateWindowEx(WS_EX_LEFT | WS_EX_CLIENTEDGE | WS_EX_CONTEXTHELP,    //Extended window styles.
         WC_EDIT,
         NULL,
         WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER    // Window styles.
         | ES_NUMBER | ES_LEFT,                     // Edit control styles.
         725, 65,
         70, 20,
         hWnd,
         (HMENU)7,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     Edit_LimitText(hControl,6);
     Edit_SetReadOnly(hControl,TRUE);
     hControl = CreateWindowEx(WS_EX_LEFT | WS_EX_LTRREADING,
         UPDOWN_CLASS,
         NULL,
         WS_CHILDWINDOW | WS_VISIBLE
         | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK| UDS_NOTHOUSANDS,
         0, 0,
         0, 0,         // Set to zero to automatically size to fit the buddy window.
         hWnd,
         (HMENU)8,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     SendMessage(hControl, UDM_SETRANGE, 0, MAKELPARAM(0, 0));
     SetDlgItemInt(hWnd, 7, 0, FALSE);

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Cool app will be here soon!!!");
    
    switch (message)
    {
    case WM_CREATE:
        
        if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
        {
            return -1;  // Fail CreateWindowEx.
        }
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&pDWriteFactory)
        );
             //Create a DirectWrite factory.

        if (SUCCEEDED(hr))
        {
            // Create a DirectWrite text format object.
            hr = pDWriteFactory->CreateTextFormat(
                msc_fontName,
                NULL,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_ULTRA_EXPANDED,
                msc_fontSize,
                L"", //locale
                &pTextFormat
            );
        }
        if (SUCCEEDED(hr))
        {
            // Create a DirectWrite text format object.
            hr = pDWriteFactory->CreateTextFormat(
                msc_fontName,
                NULL,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_ULTRA_EXPANDED,
                12,
                L"", //locale
                &pTextSmallFormat
            );
        }
        if (SUCCEEDED(hr))
        {
            // Center the text horizontally and vertically.
            pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            pTextSmallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            pTextSmallFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        return hr;
        break;
    case WM_PAINT:
        p = GetDlgItemInt(hWnd, 7, NULL, FALSE);
        if (p > N)
        {
            p = N;
            SetDlgItemInt(hWnd, 7, p, FALSE);
        }
        if (p < 0)
        {
            p = 0;
            SetDlgItemInt(hWnd, 7, p, FALSE);
        }
        hdc = BeginPaint(hWnd, &ps);
        if (datatype != (IsDlgButtonChecked(hWnd, 6) == BST_CHECKED)) {
            datatype = (IsDlgButtonChecked(hWnd, 6) == BST_CHECKED);
            if (dt != 0 && T != 0 && V != 0 && s != 0) {
                swprintf_s(TCoV, 64, L"%.10f", trapJJ(datatype, dt, T, V, s, p));
            }
        }
        
        OnPaint(hWnd);
        
        TextOut(hdc,
            5, 70,
            greeting, _tcslen(greeting));
        TextOut(hdc,
            700, 65,
            L"p = ", _tcslen(L"p = "));
        if (pszFilePath == NULL)
            TextOut(hdc,
                5, 90,
                AttenText, _tcslen(AttenText));
        else {
            TextOut(hdc,
                5, 90,
                PathText, _tcslen(PathText));
            TextOut(hdc,
                110, 90,
                pszFilePath, _tcslen(pszFilePath));
        }
        if (TCoV[0] == NULL)
            TextOut(hdc,
                5, 110,
                L"Enter all required data to calculate Therm.Cond./Vis.", _tcslen(L"Enter all required data to calculate Therm.Cond./Vis."));
        else if(datatype) {
            swprintf_s(TCoV, 64, L"%.10f", trapJJ(datatype, dt, T, V, s, p));
            TextOut(hdc,
                5, 110,
                L"Viscosity: ", _tcslen(L"Viscosity: "));
            TextOut(hdc,
                80, 110,
                TCoV, _tcslen(TCoV));
        }
        else {
            swprintf_s(TCoV, 64, L"%.10f", trapJJ(datatype, dt, T, V, s, p));
            TextOut(hdc,
                5, 110,
                L"Thermal Conductivity: ", _tcslen(L"Thermal Conductivity: "));
            TextOut(hdc,
                150, 110,
                TCoV, _tcslen(TCoV));
        }
        if(dt == 0.0)
            TextOut(hdc,
                700, 10,
                L"Timestep: --", _tcslen(L"Timestep: --"));
        else {
            TextOut(hdc,
                700, 10,
                L"Timestep: ", _tcslen(L"Timestep: "));
            TextOut(hdc,
                790, 10,
                dtstr, _tcslen(dtstr));
        }
        if (T == 0.0)
            TextOut(hdc,
                700, 28,
                L"Temperature: --", _tcslen(L"Temperature: --"));
        else {
            TextOut(hdc,
                700, 28,
                L"Temperature: ", _tcslen(L"Temperature: "));
            TextOut(hdc,
                790, 28,
                Tstr, _tcslen(Tstr));
        }
        if (V == 0.0)
            TextOut(hdc,
                700, 46,
                L"Volume: --", _tcslen(L"Volume: --"));
        else {
            TextOut(hdc,
                700, 46,
                L"Volume: ", _tcslen(L"Volume: "));
            TextOut(hdc,
                790, 46,
                Vstr, _tcslen(Vstr));
        }


        // End application-specific layout section.

        EndPaint(hWnd, &ps);


        break;
    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        break;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
            /*MessageBox(NULL,
                _T("Uraaa, eto ebannaya hainya works!"),
                _T("Windows Desktop Guided Tour"),
                NULL);*/
            switch (LOWORD(wParam)) {
            case 1:
                {
                    // Create the FileOpenDialog object.
                    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
                    // Show the Open dialog box.
                    hr = pFileOpen->Show(NULL);

                    // Get the file name from the dialog box.
                    if (SUCCEEDED(hr))
                    {
                        hr = pFileOpen->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {

                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                            // Display the file name to the user.
                            if (SUCCEEDED(hr))
                            {
                                MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                                _wfopen_s(&fp, pszFilePath, L"r");
                                JJ = edit(fp);
                                N = JJlength();
                                p = N;
                                SetDlgItemInt(hWnd, 7, p, FALSE);
                                SendDlgItemMessageA(hWnd, 8, UDM_SETRANGE, 0, MAKELPARAM(p, 0));
                                SendDlgItemMessageA(hWnd, 7, EM_SETREADONLY, FALSE, NULL);
                                s = SampInt();
                                if (JJ!=NULL)
                                    MessageBoxW(NULL, _T("Data has been processed successfully"), L"You're cool!", MB_OK);

                                if (dt != 0 && T != 0 && V != 0)
                                    swprintf_s(TCoV, 64, L"%.10f", trapJJ(datatype, dt, T, V, s,p));
                            }
                            pItem->Release();
                        }
                        
                    }
                    SafeRelease(&pFileOpen);
                }
                break;
            case 2:
                {

                    // Create the FileOpenDialog object.
                    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
                    // Show the Open dialog box.
                    hr = pFileSave->Show(NULL);

                    // Get the file name from the dialog box.
                    if (SUCCEEDED(hr))
                    {
                        hr = pFileSave->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {

                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                            // Display the file name to the user.
                            if (SUCCEEDED(hr))
                            {
                                MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                                _wfopen_s(&fr, pszFilePath, L"w");
                                save(fr,dt);
                                MessageBoxW(NULL, _T("File was saved successfully"), L"Excel will be pleased", MB_OK);
                            }
                            pItem->Release();
                        }
                    }
                }
                break;
            case 3:
                DialogBoxA(hInst,                   // application instance
                    MAKEINTRESOURCEA(IDD_SETDT), // dialog box resource
                    hWnd,                          // owner window
                    SetTDProc);   
                break;
            }
        }
        RECT rc;
        GetClientRect(hWnd, &rc);
        RedrawWindow(hWnd,&rc,NULL, RDW_INVALIDATE);
        break;

    case WM_SIZE:
        Resize(hWnd);
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}
void CalculateLayout()
{
    if (pRenderTarget != NULL)
    {
        D2D1_SIZE_F size = pRenderTarget->GetSize();
        const float x = size.width;
        const float y = size.height;
        zero = D2D1::Point2F(30.0, y - 30);
        xEnd = D2D1::Point2F( x-30.0, y - 30);
        yEnd = D2D1::Point2F(30, 140);
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
        WCHAR sc_helloWorld[] = L"Choose correlation data to plot graph";
        WCHAR index[32];
        float xm;
        float ym;
        D2D1_SIZE_F size = pRenderTarget->GetSize();
        const int x = size.width;
        const int y = size.height;
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(1.f, 1.f, 1.f));
        pRenderTarget->DrawLine(zero, xEnd, pBrush, 2.0f);
        pRenderTarget->DrawLine(zero, yEnd, pBrush, 2.0f);
        if (dt != NULL && p!=NULL) {
            for (int i = 1; i <= 10; i++)
            {
                swprintf_s(index, 32, L"%lf", dt*p*s*i/10.0);
                xm = 30 + (x - 60.0) * i / 10;
                pRenderTarget->DrawText(
                    index,
                    7,
                    pTextSmallFormat,
                    D2D1::RectF(xm-27, y, xm+27, y-30),
                    pBrush);
            }
        }
        for (int i = 1; i <= 10; i++)
        {
            swprintf_s(index,14, L"%lf", (1 - (i - 1) / 10.0));
            xm = 30 + (x - 60.0) * i / 10;
            ym = 160 + (y - 190.0) * (i - 1) / 10;
            pRenderTarget->DrawLine(D2D1::Point2F(xm, y - 35), D2D1::Point2F(xm, y - 25), pBrush, 1.0f);
            pRenderTarget->DrawLine(D2D1::Point2F(25, ym), D2D1::Point2F(35, ym), pBrush, 1.0f);
            pRenderTarget->DrawLine(D2D1::Point2F(xm, y - 35), D2D1::Point2F(xm, 140), pBrush, 0.5f);
            pRenderTarget->DrawLine(D2D1::Point2F(x-30, ym), D2D1::Point2F(35, ym), pBrush, 0.5f);
            pRenderTarget->DrawText(
                index,
                3,
                pTextSmallFormat,
                D2D1::RectF(0, ym - 8, 30, ym + 6),
                pBrush);
        }
        if (p == NULL)
            pRenderTarget->DrawText(
                sc_helloWorld,
                ARRAYSIZE(sc_helloWorld) - 1,
                pTextFormat,
                D2D1::RectF(x / 2 - 200, 200, x / 2 + 200, 220),
                pBrush
            );
        else {
            WCHAR eq[100];
            swprintf_s(eq, 100, L"p = %i, s = %i     ", p,s);
            pRenderTarget->DrawText(
                eq,
                20,
                pTextFormat,
                D2D1::RectF(x / 2 - 200, 160, x / 2 +200, 200),
                pBrush
            );
        }


        if (JJ!=NULL){   
            D2D1_POINT_2F start, end;
            int intrvl = max(1, p * 5 / (x - 60));
            float Npoints = p / intrvl;
            float step = (x - 60)/ Npoints;
            end = D2D1::Point2F(30.f, 160.f);
            for (int i = 1; i <= Npoints; i++) {
                start = end;
                end = D2D1::Point2F(30.f+step*i, (y-30)-(y-190)*(JJ[i*intrvl]/JJ[0]));
                pRenderTarget->DrawLine(start, end, pBrush, 1.5f);
            }
        }

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

INT_PTR CALLBACK SetTDProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR lpszTimestep[16];
    WORD cchTimestep;
    TCHAR lpszTemperature[16];
    WORD cchTemperature;
    TCHAR lpszVolume[32];
    WORD cchVolume;
    
    switch (message){
     case WM_COMMAND:
         switch (wParam)
         {
         case IDOK:
             // Get number of characters. 
             cchTimestep = (WORD)SendDlgItemMessage(hDlg,
                 IDC_EDIT1,
                 EM_LINELENGTH,
                 (WPARAM)0,
                 (LPARAM)0);

             if (cchTimestep == 0)
             {
                 MessageBox(hDlg,
                     L"No timestep entered.",
                     L"Error",
                     MB_OK);
             }
             else {
                 *((LPWORD)lpszTimestep) = cchTimestep;
                 TCHAR dtTest[64];
                 // Get the characters. 
                 SendDlgItemMessage(hDlg,
                     IDC_EDIT1,
                     EM_GETLINE,
                     (WPARAM)0,       // line 0 
                     (LPARAM)lpszTimestep);

                 // Null-terminate the string. 
                 lpszTimestep[cchTimestep] = 0;
                 LPTSTR endPtr;
                 dt = _tcstod(lpszTimestep, &endPtr);
                 swprintf_s(dtstr, 16, L"%lf", dt);
                 swprintf_s(dtTest, 64, L"Check timestep: %lf", dt);
                 MessageBox(hDlg,
                     dtTest,
                     L"I hope I got it right:",
                     MB_OK);
             }
            
             cchTemperature = (WORD)SendDlgItemMessage(hDlg,
                 IDC_EDIT2,
                 EM_LINELENGTH,
                 (WPARAM)0,
                 (LPARAM)0);

             if (cchTemperature == 0)
             {
                 MessageBox(hDlg,
                     L"No temperature entered.",
                     L"Error",
                     MB_OK);
             }
             else {
                 *((LPWORD)lpszTemperature) = cchTemperature;
                 TCHAR TTest[64];
                 // Get the characters. 
                 SendDlgItemMessage(hDlg,
                     IDC_EDIT2,
                     EM_GETLINE,
                     (WPARAM)0,       // line 0 
                     (LPARAM)lpszTemperature);

                 // Null-terminate the string. 
                 lpszTemperature[cchTemperature] = 0;
                 LPTSTR endPtr;
                 T = _tcstod(lpszTemperature, &endPtr);
                 swprintf_s(TTest, 64, L"Check temperature: %lf", T);
                 swprintf_s(Tstr, 32, L"%lf", T);
                 MessageBox(hDlg,
                     TTest,
                     L"I hope I got it right:",
                     MB_OK);
             }


             cchVolume = (WORD)SendDlgItemMessage(hDlg,
                 IDC_EDIT3,
                 EM_LINELENGTH,
                 (WPARAM)0,
                 (LPARAM)0);

             if (cchVolume == 0)
             {
                 MessageBox(hDlg,
                     L"No timestep entered.",
                     L"Error",
                     MB_OK);
             }
             else {
                 *((LPWORD)lpszVolume) = cchVolume;
                 TCHAR VTest[64];
                 // Get the characters. 
                 SendDlgItemMessage(hDlg,
                     IDC_EDIT3,
                     EM_GETLINE,
                     (WPARAM)0,       // line 0 
                     (LPARAM)lpszVolume);

                 // Null-terminate the string. 
                 lpszVolume[cchVolume] = 0;
                 LPTSTR endPtr;
                 V = _tcstod(lpszVolume, &endPtr);
                 swprintf_s(VTest, 64, L"Check volume: %lf", V);

                 MessageBox(hDlg,
                     VTest,
                     L"I hope I got it right:",
                     MB_OK);
             }
             if (dt != 0 && T != 0 && V != 0 && s != 0)
                 swprintf_s(TCoV, 64, L"%.10f", trapJJ(datatype, dt, T, V, s,p));
             EndDialog(hDlg, TRUE);
             return TRUE;
         case IDC_BUTTON1:
             PWSTR pszFilePathScan;
             // Create the FileOpenDialog object.
             hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                 IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
             // Show the Open dialog box.
             hr = pFileOpen->Show(NULL);
             TCHAR Test[128];
             // Get the file name from the dialog box.
             if (SUCCEEDED(hr))
             {
                 hr = pFileOpen->GetResult(&pItem);
                 if (SUCCEEDED(hr))
                 {

                     hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePathScan);

                     // Display the file name to the user.
                     if (SUCCEEDED(hr))
                     {
                         MessageBoxW(NULL, pszFilePathScan, L"File Path", MB_OK);
                         _wfopen_s(&fp, pszFilePathScan, L"r");
                         scan(fp, &dt,&T,&V);
                         swprintf_s(dtstr, 16, L"%lf", dt);
                         swprintf_s(Tstr, 32, L"%lf", T);
                         swprintf_s(Vstr, 32, L"%lf", V);
                         swprintf_s(Test, 128, L"Check timestep: %lf\nCheck temperature: %lf\nCheck volume: %lf\n",dt, T,V);
                         MessageBox(hDlg,
                             Test,
                             L"I hope I got it right:",
                             MB_OK);
                     }
                     pItem->Release();
                 }
                 
             }
             pFileOpen->Release();
             if (dt != 0 && T != 0 && V != 0 && JJ != NULL)
                 swprintf_s(TCoV, 64, L"%.10f", trapJJ(datatype, dt, T, V, s, p));
             EndDialog(hDlg, TRUE);
             break;

         case IDCANCEL:
             EndDialog(hDlg, TRUE);
             return TRUE;
         }
         return 0;
}
return FALSE;

UNREFERENCED_PARAMETER(lParam);
}
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite")

#include <fstream>
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
#include <cmath>

float average();

// Global variables
HWND hwndStatic[2];
HRESULT hr;
IFileOpenDialog* pFileOpen;
IFileSaveDialog* pFileSave;
IShellItem* pItem;
TCHAR AttenJJText[] = _T("Choose JJ correlation file");
TCHAR AttenSSText[] = _T("Choose SS correlation file");
TCHAR AttenParamText[] = _T("Open log file to calculate averages");
TCHAR PathText[] = _T("You've chosen:");
PWSTR pszFilePath, CSVfile, JJfile, SSfile, ParamFile;
FILE* fpjj,*fpss,*fp;
FILE* fr;
FILE* ETZ;
ID2D1Factory* pFactory;
IDWriteFactory* pDWriteFactory;
IDWriteTextFormat* pTextFormat, *pTextSmallFormat;
ID2D1HwndRenderTarget* pRenderTarget;
ID2D1SolidColorBrush* pBrush, *gBrush, *pBlueBrush, *pRedBrush, *pbrickBrush;
D2D1_POINT_2F zero, xEnd, yEnd;

static const WCHAR msc_fontName[] = L"Verdana";
static const FLOAT msc_fontSize = 36;
static int actualPage = 0;

float* JJ,*SS, minJJ = 0, minSS = 0, *TCValArray, *VisValArray;
int N,s,pjj,pss,lines, axis=4, NAvejj = 0, NAvess = 0;
UINT delim;
BOOL datatype = FALSE, dataline = TRUE, valLine = FALSE, method = TRUE;
double dt=0, T, V;
double steps[2000],pressure[2000],temperature[2000], volume[2000], dencity[2000];
double* tabs[5] = { steps,pressure,temperature,volume,dencity };
TCHAR dtstr[16],Tstr[32], Vstr[32], TCoV[64], TC[64],Viscos[64], AveT[20], AvePr[20], AvePrbar[20], AveVol[20], AveDens[20],delimchar,del[4];
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
double  expext (float x);

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

   /* HWND hwndTab = CreateWindow(WC_TABCONTROL, L"",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        0, 0, 550, 700,
        hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);*/

    ////Add tabs
    //TCITEM tie;
    //TCHAR achTemp[256] = L"Main";

    //tie.mask = TCIF_TEXT | TCIF_IMAGE;
    //tie.iImage = -1;
    //tie.pszText = achTemp;

    //swprintf_s(achTemp, 256, L"Main");
    //TabCtrl_InsertItem(hwndTab, 0, &tie);

    //swprintf_s(achTemp, 256, L"Correlation");
    //TabCtrl_InsertItem(hwndTab, 1, &tie);

    

    //hwndStatic[0] = CreateWindow(WC_STATIC, L"",
    //    WS_CHILD | WS_VISIBLE | WS_BORDER,
    //    0, 30, 300, 400,
    //    hwndTab, (HMENU)10, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
    //    NULL);

    //hwndStatic[1] = CreateWindow(WC_STATIC, L"",
    //    WS_CHILD | WS_VISIBLE | WS_BORDER,
    //    100, 100, 100, 100,
    //    hwndTab, (HMENU)11, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
    //    NULL);

    //Create OPEN JJ button
    HWND hwndButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"OPEN JJ correlation data",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10,         // x position 
        10,         // y position 
        170,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        (HMENU) 1,       //
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);      // Pointer not needed.

        //Create OPEN SS button
    HWND hwndButtonSS = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"OPEN SS correlation data",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10,         // x position 
        37,         // y position 
        170,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        (HMENU)11,       //
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);      // Pointer not needed.
    //Create group open/save CSV file
    HWND hwndCSVGroup = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Open or Create CSV file",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX | WS_GROUP,  // Styles 
        195,         // x position 
        3,         // y position 
        180,        // Button width
        60,        // Button height
        hWnd,     // Parent window
        (HMENU)4,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    //Create CREATE Excel file button
    HWND hwndCrButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Create",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        200,         // x position 
        22,         // y position 
        170,        // Button width
        18,        // Button height
        hWnd,     // Parent window
        (HMENU)2,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);      // Pointer not needed.

    //Create OPEN Excel file button
    HWND hwndOpButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Open",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        200,         // x position 
        42,         // y position 
        170,        // Button width
        18,        // Button height
        hWnd,     // Parent window
        (HMENU)12,       // No menu.
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
        25,        // Button height
        hWnd,     // Parent window
        (HMENU)3,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);      // Pointer not needed.

    HWND hwndsaveButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Save values",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        390,         // x position 
        37,         // y position 
        170,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        (HMENU)9,       // No menu.
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
         890, 70,
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

     HWND hAveControl = CreateWindowEx(WS_EX_LEFT | WS_EX_CLIENTEDGE | WS_EX_CONTEXTHELP,    //Extended window styles.
         WC_EDIT,
         NULL,
         WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER    // Window styles.
         | ES_NUMBER | ES_LEFT,                     // Edit control styles.
         890, 93,
         70, 20,
         hWnd,
         (HMENU)45,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     Edit_SetReadOnly(hAveControl, TRUE);
     Edit_LimitText(hAveControl, 6);
     hAveControl = CreateWindowEx(WS_EX_LEFT | WS_EX_LTRREADING,
         UPDOWN_CLASS,
         NULL,
         WS_CHILDWINDOW | WS_VISIBLE
         | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK | UDS_NOTHOUSANDS,
         0, 0,
         0, 0,         // Set to zero to automatically size to fit the buddy window.
         hWnd,
         (HMENU)46,
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     SendMessage(hAveControl, UDM_SETRANGE, 1, MAKELPARAM(1, 1));
     SetDlgItemInt(hWnd, 45, 1, FALSE);

     HWND hwndButtonRad21 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"xx",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
         700,         // x position
         13,         // y position 
         117,        // Button width
         30,        // Button height
         hWnd,     // Parent window
         (HMENU)21,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

     HWND hwndButtonRad22 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"yy",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
         700,         // x position
         38,         // y position 
         117,        // Button width
         20,        // Button height
         hWnd,     // Parent window
         (HMENU)22,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

     HWND hwndButtonRad23 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"zz",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
         700,         // x position
         58,         // y position 
         117,        // Button width
         20,        // Button height
         hWnd,     // Parent window
         (HMENU)23,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

     HWND hwndButtonRad24 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Ave xyz",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
         700,         // x position
         78,         // y position 
         117,        // Button width
         20,        // Button height
         hWnd,     // Parent window
         (HMENU)24,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     HWND hwndButtonGroup2 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Tensor:",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX | WS_GROUP,  // Styles 
         700,         // x position 
         3,         // y position 
         120,        // Button width
         95,        // Button height
         hWnd,     // Parent window
         (HMENU)20,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     CheckRadioButton(hWnd, 21, 24, 24);

     HWND hwndButtonCheck31 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Correlation",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,  // Styles 
         578,         // x position
         75,         // y position 
         117,        // Button width
         30,        // Button height
         hWnd,     // Parent window
         (HMENU)31,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     HWND hwndButtonCheck32 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"TC/Viscosity",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,  // Styles 
         578,         // x position
         100,         // y position 
         117,        // Button width
         20,        // Button height
         hWnd,     // Parent window
         (HMENU)32,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

     HWND hwndButtonGroup3 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Lines:",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX | WS_GROUP,  // Styles 
         575,         // x position 
         65,         // y position 
         120,        // Button width
         58,       // Button height
         hWnd,     // Parent window
         (HMENU)30,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     CheckDlgButton(hWnd, 31, BST_CHECKED);

     HWND hwndButtonRad41 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Cut",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
         820,         // x position
         15,         // y position 
         127,        // Button width
         30,        // Button height
         hWnd,     // Parent window
         (HMENU)41,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);

     HWND hwndButtonRad42 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Average",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,  // Styles 
         820,         // x position
         40,         // y position 
         127,        // Button width
         20,        // Button height
         hWnd,     // Parent window
         (HMENU)42,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     HWND hwndButtonGroup4 = CreateWindow(
         L"BUTTON",  // Predefined class; Unicode assumed 
         L"Method:",      // Button text 
         WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX | WS_GROUP,  // Styles 
         820,         // x position 
         3,         // y position 
         130,        // Button width
         60,        // Button height
         hWnd,     // Parent window
         (HMENU)40,       // No menu.
         (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
         NULL);
     CheckRadioButton(hWnd, 41, 42, 41);

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);

    //ShowWindow(hwndStatic[0],
    //    TRUE);
    //UpdateWindow(hwndStatic[0]);
    //ShowWindow(hwndStatic[1],
    //    FALSE);
    
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

        if (datatype != (IsDlgButtonChecked(hWnd, 5) == BST_CHECKED)) {
            datatype = (IsDlgButtonChecked(hWnd, 5) == BST_CHECKED);
            CalculateLayout();
            if (datatype) {
                SetDlgItemInt(hWnd, 7, pjj, FALSE);
                SetDlgItemInt(hWnd, 45, NAvejj, FALSE);
                if (dt != 0 && T != 0 && V != 0 && s != 0) {
                    
                    if (method)
                        swprintf_s(TC, 64, L"%.10f", TCValArray[pjj - 1]);
                    else
                        swprintf_s(TC, 64, L"%.10f", average());
                }
            }
            else {
                SetDlgItemInt(hWnd, 7, pss, FALSE);
                SetDlgItemInt(hWnd, 45, NAvess, FALSE);
                if (dt != 0 && T != 0 && V != 0 && s != 0 && SS!=NULL)
                    if(method)
                        swprintf_s(Viscos, 64, L"%.10f", VisValArray[pss-1]);
                    else
                        swprintf_s(Viscos, 64, L"%.10f", average());

            }
        }
        if (datatype)
        {
            pjj = GetDlgItemInt(hWnd, 7, NULL, FALSE);
            NAvejj = GetDlgItemInt(hWnd, 45, NULL, FALSE);
            if (pjj > N)
            {
                pjj = N;
                SetDlgItemInt(hWnd, 7, pjj, FALSE);
            }
            else if (pjj < 0)
            {
                pjj = 0;
                SetDlgItemInt(hWnd, 7, pjj, FALSE);
            }
            if (NAvejj > pjj) 
            {
                NAvejj = pjj;
                SetDlgItemInt(hWnd, 45, NAvejj, FALSE);
            }
            else if  (NAvejj < 1)
            {
                NAvejj = 1;
                SetDlgItemInt(hWnd, 45, NAvejj, FALSE);
            }
        }
        else
        {
            pss = GetDlgItemInt(hWnd, 7, NULL, FALSE);
            NAvess = GetDlgItemInt(hWnd, 45, NULL, FALSE);
            if (pss > N)
            {
                pss = N;
                SetDlgItemInt(hWnd, 7, pss, FALSE);
            }
            else if (pss < 0)
            {
                pss = 0;
                SetDlgItemInt(hWnd, 7, pss, FALSE);
            }
            if (NAvess > pss)
            {
                NAvess = pss;
                SetDlgItemInt(hWnd, 45, NAvess, FALSE);
            }
            else if (NAvess < 1)
            {
                NAvess = 1;
                SetDlgItemInt(hWnd, 45, NAvess, FALSE);
            }
        }

        hdc = BeginPaint(hWnd, &ps);
        
        OnPaint(hWnd);
        
        TextOut(hdc,
            5, 65,
            greeting, _tcslen(greeting));
        TextOut(hdc,
            860, 72,
            L"p = ", _tcslen(L"p = "));
        TextOut(hdc,
            825, 95,
            L"N_ave =", _tcslen(L"N_ave ="));

        if(datatype) {
            if (TC[0] == NULL)
            {
                TextOut(hdc,
                        5, 125,
                        L"Enter all required data to calculate Therm.Cond.", _tcslen(L"Enter all required data to calculate Therm.Cond."));
            }
            else{
                if (method)
                    swprintf_s(TC, 64, L"%.10f", TCValArray[pjj-1]);
                else
                    swprintf_s(TC, 64, L"%.10f", average());
                TextOut(hdc,
                    5, 125,
                    L"Thermal Conductivity: ", _tcslen(L"Thermal Conductivity: "));

                TextOut(hdc,
                    150, 125,
                    TC, _tcslen(TC));
            }
        }
        else {
            if (Viscos[0] == NULL) 
                TextOut(hdc,
                    5, 125,
                    L"Enter all required data to calculate Viscosity", _tcslen(L"Enter all required data to calculate Viscosity"));
            else {
                if (method)
                    swprintf_s(Viscos, 64, L"%.10f", VisValArray[pss-1]);
                else
                    swprintf_s(Viscos, 64, L"%.10f", average());
                TextOut(hdc,
                    5, 125,
                    L"Viscosity: ", _tcslen(L"Viscosity: "));
                TextOut(hdc,
                    80, 125,
                    Viscos, _tcslen(Viscos));
            }
            
        }
        if(dt == 0.0)
            TextOut(hdc,
                5, 80,
                L"Timestep: --", _tcslen(L"Timestep: --"));
        else {
            TextOut(hdc,
                5, 80,
                L"Timestep: ", _tcslen(L"Timestep: "));
            TextOut(hdc,
                95, 80,
                dtstr, _tcslen(dtstr));
        }
        if (T == 0.0)
            TextOut(hdc,
                5, 95,
                L"Temperature: --", _tcslen(L"Temperature: --"));
        else {
            TextOut(hdc,
                5, 95,
                L"Temperature: ", _tcslen(L"Temperature: "));
            TextOut(hdc,
                95, 95,
                Tstr, _tcslen(Tstr));
        }
        if (V == 0.0)
            TextOut(hdc,
                5, 110,
                L"Volume: --", _tcslen(L"Volume: --"));
        else {
            TextOut(hdc,
                5, 110,
                L"Volume: ", _tcslen(L"Volume: "));
            TextOut(hdc,
                95, 110,
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

                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &JJfile);

                            // Display the file name to the user.
                            if (SUCCEEDED(hr))
                            {
                                /*MessageBoxW(NULL, JJfile, L"File Path", MB_OK);*/
                                _wfopen_s(&fpjj, JJfile, L"r");
                                JJ = edit(fpjj, axis);
                                N = JJlength();
                                TCValArray = new float[N];
                                pjj = N;
                                minJJ = 0;
                                for (int i = 0; i < N; i++)
                                    minJJ = (minJJ > JJ[i]) ? JJ[i] : minJJ;
                                minJJ = minJJ / JJ[0];
                                CalculateLayout();
                                SetDlgItemInt(hWnd, 7, pjj, FALSE);
                                SendDlgItemMessageA(hWnd, 8, UDM_SETRANGE, 0, MAKELPARAM(20000, 0));
                                SendDlgItemMessageA(hWnd, 7, EM_SETREADONLY, FALSE, NULL);

                                SetDlgItemInt(hWnd, 45, NAvejj, FALSE);
                                SendDlgItemMessageA(hWnd, 46, UDM_SETRANGE, 0, MAKELPARAM(20000, 1));
                                SendDlgItemMessageA(hWnd, 45, EM_SETREADONLY, FALSE, NULL);
                                s = SampInt();
                                /*if (JJ!=NULL)
                                    MessageBoxW(NULL, _T("Data has been processed successfully"), L"You're cool!", MB_OK);*/

                                if (dt != 0 && T != 0 && V != 0) {
                                    TCValArray = trapJJ(JJ, datatype, dt, T, V, s, N);
                                    if (method)
                                        swprintf_s(TC, 64, L"%.10f", TCValArray[pjj-1]);
                                    else
                                        swprintf_s(TC, 64, L"%.10f", average());
                                }
                            }
                            pItem->Release();
                        }
                        
                    }
                    SafeRelease(&pFileOpen);
                }
                break;

            case 11:
            {
                // Create the FileOpenDialog object.
                hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                    IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

                hr = pFileOpen->Show(NULL);
                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {

                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {

                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &SSfile);

                        // Display the file name to the user.
                        if (SUCCEEDED(hr))
                        {
                            /*MessageBoxW(NULL, SSfile, L"File Path", MB_OK);*/
                            _wfopen_s(&fpss, SSfile, L"r");
                            SS = edit(fpss, axis);
                            N = JJlength();
                            VisValArray = new float[N];
                            pss = N;
                            minSS = 0;
                            for (int i = 0; i < N; i++)
                                minSS = (minJJ > SS[i]) ? SS[i] : minSS;
                            minSS = minSS / SS[0];
                            CalculateLayout();
                            SetDlgItemInt(hWnd, 7, pss, FALSE);
                            SendDlgItemMessageA(hWnd, 8, UDM_SETRANGE, 0, MAKELPARAM(20000, 0));
                            SendDlgItemMessageA(hWnd, 7, EM_SETREADONLY, FALSE, NULL);
                            s = SampInt();
                            /*if (SS != NULL)
                                MessageBoxW(NULL, _T("Data has been processed successfully"), L"You're cool!", MB_OK);*/

                            if (dt != 0 && T != 0 && V != 0) {
                                VisValArray = trapJJ(SS, datatype, dt, T, V, s, N);
                                if (method)
                                    swprintf_s(Viscos, 64, L"%.10f", VisValArray[pss - 1]);
                                else
                                    swprintf_s(Viscos, 64, L"%.10f", average());
                            }
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
                    COMDLG_FILTERSPEC fileFilter[2];
                    fileFilter[0].pszName = L"CSV";
                    fileFilter[0].pszSpec = L"*.csv";
                    fileFilter[1].pszName = L"Text";
                    fileFilter[1].pszSpec = L"*.txt";

                    pFileSave->SetFileTypes(2, fileFilter);
                    pFileSave->SetFileTypeIndex(1);
                    pFileSave->SetDefaultExtension(L"csv");
                    // Show the Open dialog box.
                    hr = pFileSave->Show(NULL);
                    // Get the file name from the dialog box.
                    if (SUCCEEDED(hr))
                    {
                        hr = pFileSave->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {
                            hr = pFileSave->GetFileTypeIndex(&delim);
                            if (SUCCEEDED(hr))
                            {
                                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &CSVfile);

                                // Display the file name to the user.
                                if (SUCCEEDED(hr))
                                {
                                    /*MessageBoxW(NULL, CSVfile, L"File Path", MB_OK);*/
                                    /*_wfopen_s(&fr, pszFilePath, L"w");*/
                                    /*save(fr,dt);*/
                                    _wfopen_s(&ETZ, CSVfile, L"w");
                                    if (delim == 1) {
                                        delimchar = L';';
                                        fwprintf(ETZ, L"sep=;\nTemperature K;Pressure bar;Pressure atm;Volume cm^3/mole;Density grams/cm^3;Thermal conductivity W/mK;Viscosity Pa*s\n");
                                    }
                                    
                                    else {
                                        delimchar = L'\t';
                                        fwprintf(ETZ, L"Temperature K\tPressure bar\tPressure atm\tVolume cm^3/mole\tDensity grams/cm^3\tThermal conductivity W/mK\tViscosity Pa*s\n");
                                    }
                                    fclose(ETZ);
                                }
                                pItem->Release();
                            }
                        }
                    }
                    SafeRelease(&pFileSave);
                }
                break;
            case 12:
            {
                // Create the FileOpenDialog object.
                hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                    IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
                // Show the Open dialog box.
                COMDLG_FILTERSPEC fileFilter[2];
                fileFilter[0].pszName = L"CSV";
                fileFilter[0].pszSpec = L"*.csv";
                fileFilter[1].pszName = L"Text";
                fileFilter[1].pszSpec = L"*.txt";

                pFileOpen->SetFileTypes(2, fileFilter);
                pFileOpen->SetFileTypeIndex(1);
                pFileOpen->SetDefaultExtension(L"csv");
                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);
                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    hr = pFileOpen->GetFileTypeIndex(&delim);
                    if (SUCCEEDED(hr))
                    {
                        hr = pFileOpen->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {

                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &CSVfile);

                            // Display the file name to the user.
                            if (SUCCEEDED(hr))
                            {
                                /*MessageBoxW(NULL, CSVfile, L"File Path", MB_OK);*/
                                _wfopen_s(&ETZ, CSVfile, L"a+");
                                if (delim == 1) {
                                    
                                    TCHAR sep[10];
                                    const WCHAR sepcmp[] = L"sep=";
                                    rewind(ETZ);
                                    fgetws(sep, 10, ETZ);
                                    rewind(ETZ);
                                    if (wcsncmp(sep, sepcmp, 4) == 0) {
                                        delimchar = sep[4];
                                    }
                                    else {
                                        GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT,
                                            LOCALE_SLIST,
                                            (LPWSTR)&delimchar,
                                            4);
                                    }
                                    /*fwprintf(ETZ, L"Eto polnaya hiynya");*/
                                    fwprintf(ETZ,
                                        L"\nTemperature K%cPressure bar%cPressure atm%cVolume cm^3/mole%cDensity grams/cm^3%cThermal conductivity W/mK%cViscosity Pa*s\n",
                                        delimchar,
                                        delimchar,
                                        delimchar,
                                        delimchar,
                                        delimchar,
                                        delimchar);
                                }

                                else {
                                    delimchar = L'\t';
                                    fwprintf(ETZ, L"Temperature K\tPressure bar\tPressure atm\tVolume cm^3/mole\tDensity grams/cm^3\tThermal conductivity W/mK\tViscosity Pa*s\n");
                                }
                                fclose(ETZ);

                            }
                            pItem->Release();
                        }
                    }

                }
                SafeRelease(&pFileOpen);
            }
            break;
            case 3:
                DialogBoxA(hInst,                   // application instance
                    MAKEINTRESOURCEA(IDD_SETDT), // dialog box resource
                    hWnd,                          // owner window
                    SetTDProc);   
                break;
            case 9:
                if (ETZ == NULL)
                    MessageBoxW(NULL, _T("Please, create file first"), L"Excel will be pleased", MB_OK);
                else
                {
                    _wfopen_s(&ETZ, CSVfile, L"a");
                    fwprintf(ETZ, AveT);
                    fwprintf(ETZ, &delimchar);
                    fwprintf(ETZ, AvePrbar);
                    fwprintf(ETZ, &delimchar);
                    fwprintf(ETZ, AvePr);
                    fwprintf(ETZ, &delimchar);
                    fwprintf(ETZ, AveVol);
                    fwprintf(ETZ, &delimchar);
                    fwprintf(ETZ, AveDens);
                    fwprintf(ETZ, &delimchar);
                    if (TC[0]==NULL)
                        fwprintf(ETZ, L"NoData");
                    else
                        fwprintf(ETZ, TC);
                    fwprintf(ETZ, &delimchar);
                    if (Viscos[0] == NULL)
                        fwprintf(ETZ, L"NoData");
                    else
                        fwprintf(ETZ, Viscos);
                    fwprintf(ETZ, L"\n");
                    fclose(ETZ);
                }
                break;
            case 21:
                axis = 1;
                break;
            case 22:
                axis = 2;
                break;
            case 23:
                axis = 3;
                break;
            case 24:
                axis = 4;
                break;
            case 31:
                dataline = IsDlgButtonChecked(hWnd, 31);
                break;
            case 32:
                valLine = IsDlgButtonChecked(hWnd, 32);
                break;
            case 41:
                method = TRUE;
                break;
            case 42:
                method = FALSE;
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

    //case WM_NOTIFY:
    //    switch (((LPNMHDR)lParam)->code)
    //    {
    //    case TCN_SELCHANGING:
    //    {
    //        // Return FALSE to allow the selection to change.
    //        return FALSE;
    //    }

    //    case TCN_SELCHANGE:
    //    {
    //        int iPage = TabCtrl_GetCurSel(GetParent(hwndStatic[0]));
    //        ShowWindow(hwndStatic[actualPage], SW_HIDE);
    //        ShowWindow(hwndStatic[iPage], SW_SHOW);
    //        actualPage = iPage;
    //        break;
    //    }
    //    }
    //    return TRUE;


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
        if (datatype) {
            zero = D2D1::Point2F(30.0, y - 30 - (1 / (minJJ - 1) + 1) * (y - 190));
            xEnd = D2D1::Point2F(x - 30.0, y - 30 - (1 / (minJJ - 1) + 1) * (y - 140 - 30));
        }
        else {
            zero = D2D1::Point2F(30.0, y - 30 - (1 / (minSS - 1) + 1) * (y - 190));
            xEnd = D2D1::Point2F(x - 30.0, y - 30 - (1 / (minSS - 1) + 1) * (y - 140 - 30));
        }
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
            const D2D1_COLOR_F colorblue = D2D1::ColorF(D2D1::ColorF::Blue);
            const D2D1_COLOR_F colorred = D2D1::ColorF(D2D1::ColorF::Red);
            const D2D1_COLOR_F colorbrick = D2D1::ColorF(D2D1::ColorF::Firebrick);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
            hr = pRenderTarget->CreateSolidColorBrush(colorblue, &pBlueBrush);
            hr = pRenderTarget->CreateSolidColorBrush(colorred, &pRedBrush);
            hr = pRenderTarget->CreateSolidColorBrush(colorbrick, &pbrickBrush);
            if (SUCCEEDED(hr))
            {
                const D2D1_COLOR_F color = D2D1::ColorF(0.3f, 0.9f, 0.3f);
                hr = pRenderTarget->CreateSolidColorBrush(color, &gBrush);
                if (SUCCEEDED(hr))
                {
                    CalculateLayout();
                }
            }
        }
    }
    return hr;
}

void DiscardGraphicsResources() {
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
    SafeRelease(&gBrush);
}

void OnPaint(HWND m_hwnd) {
    HRESULT hr = CreateGraphicsResources(m_hwnd);
    if (SUCCEEDED(hr))
    {
        WCHAR sc_helloWorld[] = L"Choose correlation data to plot graph";
        WCHAR sc_PoshelNAXUY[] = L"Correlation\nTher.C/Vis";
        WCHAR index[32];
        float xm;
        float ym;
        D2D1_SIZE_F size = pRenderTarget->GetSize();
        const int x = size.width;
        const int y = size.height;
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(1.f, 1.f, 1.f));
        pRenderTarget->DrawLine(zero, xEnd, pBrush, 2.0f);
        pRenderTarget->DrawLine(D2D1::Point2F(30, y - 20), yEnd, pBrush, 2.0f);
        pRenderTarget->DrawLine(D2D1::Point2F(x - 125, 168), D2D1::Point2F(x - 90, 168), pBlueBrush, 2.0f);
        pRenderTarget->DrawLine(D2D1::Point2F(x - 125, 185), D2D1::Point2F(x - 90, 185), pRedBrush, 2.0f);
        pRenderTarget->DrawText(
            sc_PoshelNAXUY,
            22,
            pTextSmallFormat,
            D2D1::RectF(x - 200, 150, x - 130, 200),
            pBrush);
        if (datatype) {
            /*if (dt != NULL && pjj != NULL)*/
            if (pjj != NULL) {
                for (int i = 1; i <= 10; i++)
                {
                    /*swprintf_s(index, 32, L"%lf", dt * pjj * s * i / 10.0);*/
                    swprintf_s(index, 32, L"%lf", pjj * i / 10.0);
                    xm = 30 + (x - 60.0) * i / 10;
                    pRenderTarget->DrawText(
                        index,
                        7,
                        pTextSmallFormat,
                        D2D1::RectF(xm - 27, y, xm + 27, y - 30),
                        pBrush);
                }
            }
        }
        else {
            /*if (dt != NULL && pss != NULL)*/
            if (pss != NULL) {
                for (int i = 1; i <= 10; i++)
                {
                    /*swprintf_s(index, 32, L"%lf", dt * pss * s * i / 10.0);*/
                    swprintf_s(index, 32, L"%lf", pss * i / 10.0);
                    xm = 30 + (x - 60.0) * i / 10;
                    pRenderTarget->DrawText(
                        index,
                        7,
                        pTextSmallFormat,
                        D2D1::RectF(xm - 27, y, xm + 27, y - 30),
                        pBrush);
                }
            }
        }
        if (datatype) {
            for (int i = 1; i <= 10; i++)
            {
                swprintf_s(index, 14, L"%lf", (1 - (i - 1) / 10.0));
                xm = 30 + (x - 60.0) * i / 10;
                ym = 160 - (y - 190.0) / (minJJ - 1) * (i - 1) / 10;
                pRenderTarget->DrawLine(D2D1::Point2F(xm, y - 35), D2D1::Point2F(xm, y - 25), pBrush, 1.0f);
                pRenderTarget->DrawLine(D2D1::Point2F(25, ym), D2D1::Point2F(35, ym), pBrush, 1.0f);
                pRenderTarget->DrawLine(D2D1::Point2F(xm, y - 35), D2D1::Point2F(xm, 140), pBrush, 0.5f);
                pRenderTarget->DrawLine(D2D1::Point2F(x - 30, ym), D2D1::Point2F(35, ym), pBrush, 0.5f);
                pRenderTarget->DrawText(
                    index,
                    3,
                    pTextSmallFormat,
                    D2D1::RectF(0, ym - 8, 30, ym + 6),
                    pBrush);
            }

            swprintf_s(index, 14, L"%lf", 0.0);
            ym = 160 - (y - 190.0) / (minJJ - 1);
            pRenderTarget->DrawLine(D2D1::Point2F(25, ym), D2D1::Point2F(35, ym), pBrush, 1.0f);
            pRenderTarget->DrawText(
                index,
                3,
                pTextSmallFormat,
                D2D1::RectF(0, ym - 8, 30, ym + 6),
                pBrush);
            for (float i = -0.1; i >= minJJ; i -= 0.1)
            {
                swprintf_s(index, 14, L"%lf", i);
                ym = 160 - (y - 190.0) / (minJJ - 1) * (1 - i);
                pRenderTarget->DrawLine(D2D1::Point2F(28, ym), D2D1::Point2F(35, ym), pBrush, 1.0f);
                pRenderTarget->DrawLine(D2D1::Point2F(x - 30, ym), D2D1::Point2F(35, ym), pBrush, 0.5f);
                pRenderTarget->DrawText(
                    index,
                    4,
                    pTextSmallFormat,
                    D2D1::RectF(0, ym - 8, 30, ym + 6),
                    pBrush);
            }
            if (pjj == NULL)
                pRenderTarget->DrawText(
                    sc_helloWorld,
                    ARRAYSIZE(sc_helloWorld) - 1,
                    pTextFormat,
                    D2D1::RectF(x / 2 - 200, 200, x / 2 + 200, 220),
                    pBrush
                );
            else {
                WCHAR eq[100];
                swprintf_s(eq, 100, L"p = %i, s = %i     ", pjj, s);
                pRenderTarget->DrawText(
                    eq,
                    20,
                    pTextFormat,
                    D2D1::RectF(x / 2 - 200, 160, x / 2 + 200, 200),
                    pBrush
                );
            }
            if (JJ != NULL) {
                D2D1_POINT_2F start, end;
                float    step = (x - 60.0) / (pjj); 
                end = D2D1::Point2F(30.f, 160.f);
                if (dataline)
                for (int i = 1; i < pjj; i++) {
                    start = end;
                    end = D2D1::Point2F(30.f + step * i, (y - 30 - (1 / (minJJ - 1) + 1) * (y - 190) + (y-190)/(minJJ-1) * (JJ[i] / JJ[0])));
                    pRenderTarget->DrawLine(start, end, pBlueBrush, 1.5f);
                }
                if (TCValArray != NULL and valLine) {
                    end = D2D1::Point2F(30.f, y - 30 - (1 / (minJJ - 1) + 1) * (y - 190));
                    float maxval = MaxTC();
                    if (method)
                    for (int i = 1; i < pjj; i++) {
                        start = end;
                        end = D2D1::Point2F(30.f + step * i, y - 30 - (1 / (minJJ - 1) + 1) * (y - 190) + (y - 240) / (minJJ - 1) * (TCValArray[i] / maxval));
                        pRenderTarget->DrawLine(start, end, pRedBrush, 1.5f);
                    }
                    else {
                        for (int i = 1; i < pjj-NAvejj; i++) {
                            start = end;
                            end = D2D1::Point2F(30.f + step * i, y - 30 - (1 / (minJJ - 1) + 1) * (y - 190) + (y - 240) / (minJJ - 1) * (TCValArray[i] / maxval));
                            pRenderTarget->DrawLine(start, end, pRedBrush, 1.5f);
                        }
                        for (int i = pjj - NAvejj; i < pjj; i++) {
                            start = end;
                            end = D2D1::Point2F(30.f + step * i, y - 30 - (1 / (minJJ - 1) + 1) * (y - 190) + (y - 240) / (minJJ - 1) * (TCValArray[i] / maxval));
                            pRenderTarget->DrawLine(start, end, pbrickBrush, 3.f);
                        }
                    }
                }
            }
            
        }
        else
        {
            for (int i = 1; i <= 10; i++)
        {
            swprintf_s(index, 14, L"%lf", (1 - (i - 1) / 10.0));
            xm = 30 + (x - 60.0) * i / 10;
            ym = 160 - (y - 190.0)/(minSS-1) * (i - 1) / 10;
            pRenderTarget->DrawLine(D2D1::Point2F(xm, y - 35), D2D1::Point2F(xm, y - 25), pBrush, 1.0f);
            pRenderTarget->DrawLine(D2D1::Point2F(25, ym), D2D1::Point2F(35, ym), pBrush, 1.0f);
            pRenderTarget->DrawLine(D2D1::Point2F(xm, y - 35), D2D1::Point2F(xm, 140), pBrush, 0.5f);
            pRenderTarget->DrawLine(D2D1::Point2F(x - 30, ym), D2D1::Point2F(35, ym), pBrush, 0.5f);
            pRenderTarget->DrawText(
                index,
                3,
                pTextSmallFormat,
                D2D1::RectF(0, ym - 8, 30, ym + 6),
                pBrush);
        }

        swprintf_s(index, 14, L"%lf", 0.0);
        ym = 160 - (y - 190.0) / (minSS - 1);
        pRenderTarget->DrawLine(D2D1::Point2F(25, ym), D2D1::Point2F(35, ym), pBrush, 1.0f);
        pRenderTarget->DrawText(
            index,
            3,
            pTextSmallFormat,
            D2D1::RectF(0, ym - 8, 30, ym + 6),
            pBrush);
            for (float i = -0.1; i >= minSS; i -= 0.1)
            {
                swprintf_s(index, 14, L"%lf", i);
                ym = 160 - (y - 190.0) / (minSS - 1) * (1 - i);
                pRenderTarget->DrawLine(D2D1::Point2F(28, ym), D2D1::Point2F(35, ym), pBrush, 1.0f);
                pRenderTarget->DrawLine(D2D1::Point2F(x - 30, ym), D2D1::Point2F(35, ym), pBrush, 0.5f);
                pRenderTarget->DrawText(
                    index,
                    4,
                    pTextSmallFormat,
                    D2D1::RectF(0, ym - 8, 30, ym + 6),
                    pBrush);
            }
            if (pss == NULL)
                pRenderTarget->DrawText(
                    sc_helloWorld,
                    ARRAYSIZE(sc_helloWorld) - 1,
                    pTextFormat,
                    D2D1::RectF(x / 2 - 200, 200, x / 2 + 200, 220),
                    pBrush
                );
            else {
                WCHAR eq[100];
                swprintf_s(eq, 100, L"p = %i, s = %i     ", pss, s);
                pRenderTarget->DrawText(
                    eq,
                    20,
                    pTextFormat,
                    D2D1::RectF(x / 2 - 200, 160, x / 2 + 200, 200),
                    pBrush
                );
            }
            if (SS != NULL) {
                D2D1_POINT_2F start, end;
                float step = (x - 60.0) / (pss-1);
                end = D2D1::Point2F(30.f, 160.f);
                for (int i = 1; i < pss; i++) {
                    start = end;
                    end = D2D1::Point2F(30.f + step * i, (y - 30 - (1 / (minSS - 1) + 1) * (y - 190) + (y - 190) / (minSS - 1) * (SS[i] / SS[0])));
                    pRenderTarget->DrawLine(start, end, pBlueBrush, 1.5f);
                }
                if (VisValArray != NULL and valLine) {
                    end = D2D1::Point2F(30.f, y - 30 - (1 / (minSS - 1) + 1) * (y - 190));
                    float maxval = MaxVis();
                    if (method)
                        for (int i = 1; i < pss; i++) {
                            start = end;
                            end = D2D1::Point2F(30.f + step * i, y - 30 - (1 / (minSS - 1) + 1) * (y - 190) + (y - 240) / (minSS - 1) * (VisValArray[i] / maxval));
                            pRenderTarget->DrawLine(start, end, pRedBrush, 1.5f);
                        }
                    else {
                        for (int i = 1; i < pss - NAvess; i++) {
                            start = end;
                            end = D2D1::Point2F(30.f + step * i, y - 30 - (1 / (minSS - 1) + 1) * (y - 190) + (y - 240) / (minSS - 1) * (VisValArray[i] / maxval));
                            pRenderTarget->DrawLine(start, end, pRedBrush, 1.5f);
                        }
                        for (int i = pss - NAvess; i < pss; i++) {
                            start = end;
                            end = D2D1::Point2F(30.f + step * i, y - 30 - (1 / (minSS - 1) + 1) * (y - 190) + (y - 240) / (minSS - 1) * (VisValArray[i] / maxval));
                            pRenderTarget->DrawLine(start, end, pbrickBrush, 3.f);
                        }
                    }
                }
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
             if (dt != 0 && T != 0 && V != 0 && s != 0) {
                 if (datatype) {
                     TCValArray = trapJJ(JJ, datatype, dt, T, V, s, N);
                     if (method)
                        swprintf_s(TC, 64, L"%.10f", TCValArray[pjj - 1]);
                     else
                        swprintf_s(TC, 64, L"%.10f", average());
                 }
                 else {
                     VisValArray = trapJJ(SS, datatype, dt, T, V, s, N);
                     if(method)
                        swprintf_s(Viscos, 64, L"%.10f", VisValArray[pjj - 1]);
                     else
                         swprintf_s(Viscos, 64, L"%.10f", average());
                 }
             }

             EndDialog(hDlg, TRUE);
             return TRUE;
         case IDC_BUTTON1:
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

                     hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &ParamFile);

                     // Display the file name to the user.
                     if (SUCCEEDED(hr))
                     {
                         /*MessageBoxW(NULL, ParamFile, L"File Path", MB_OK);*/
                         _wfopen_s(&fp, ParamFile, L"r");
                         scan(fp, &dt,&T,&V, &lines,tabs);
                         swprintf_s(dtstr, 16, L"%lf", dt);
                         swprintf_s(Tstr, 32, L"%lf", T);
                         swprintf_s(Vstr, 32, L"%lf", V);
                         /*swprintf_s(Test, 128, L"Check timestep: %lf\nCheck temperature: %lf\nCheck volume: %lf\n",dt, T,V);
                         MessageBox(hDlg,
                             Test,
                             L"I hope I got it right:",
                             MB_OK);*/
                         double ave=0.0;
                         for (int j = 90; j <= 100; j++)
                              ave += pressure[j];
                         ave /= 11;
                         swprintf_s(AvePr, 20, L"%lf", ave);
                         swprintf_s(AvePrbar, 20, L"%lf", ave*1.01325);
                         ave = 0.0;
                         for (int j = 90; j <= 100; j++)
                             ave += temperature[j];
                         ave /= 11;
                         swprintf_s(AveT, 20, L"%lf", ave);
                         ave = 0.0;
                         for (int j = 90; j <= 100; j++)
                             ave += volume[j];
                         ave /= 11;
                         swprintf_s(AveVol, 20, L"%lf", ave);
                         ave = 0.0;
                         for (int j = 90; j <= 100; j++)
                             ave += dencity[j];
                         ave /= 11;
                         swprintf_s(AveDens, 20, L"%lf", ave);
                         ave = 0.0;
                     }
                     pItem->Release();
                 }
                 
             }
             pFileOpen->Release();
             if (dt != 0 && T != 0 && V != 0) {
                 if (JJ != NULL) {
                     TCValArray = trapJJ(JJ, datatype, dt, T, V, s, N);
                     if(method)
                        swprintf_s(TC, 64, L"%.10f", TCValArray[pjj - 1]);
                     else
                         swprintf_s(TC, 64, L"%.10f", average());
                 }
                 if (SS != NULL) {
                     VisValArray = trapJJ(SS, datatype, dt, T, V, s, N);
                     if (method)
                        swprintf_s(Viscos, 64, L"%.10f", VisValArray[pjj - 1]);
                     else
                         swprintf_s(Viscos, 64, L"%.10f", average());
                 }
                 }   
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

float average() 
{
    float ave = 0.f;
    if (datatype)
    {
        for (int i = 0; i < NAvejj; i++)
            ave += TCValArray[pjj - 1 - i]/NAvejj;
    }
    else
    {
        for (int i = 0; i < NAvess; i++)
            ave += VisValArray[pss - 1 - i]/NAvess;
    }
    return ave;
}

double expext(float x) {
    double a = -0.737908880590363e-2;
    double b = -0.398172655521746e-2;
    return (.63632746076124 * exp(a * x) + .357366674082023 * exp(b * x * x));
}
// kuka's simple crosshair overlay  

#include <windows.h>  
#include <shellapi.h>  
#include "resource.h"  

#define WM_TRAYICON (WM_USER + 1)  

// Ustawienia celownika  
int dotSize = 2;   // 2 = 2x2, 1 = 1x1
int offsetY = 0;
COLORREF colors[] = {
    RGB(255, 255, 255), // bialy  
    RGB(255, 0, 255),   // magenta  
    RGB(0, 255, 0),     // zielony  
    RGB(0, 0, 255),     // niebieski  
    RGB(255, 0, 0)      // czerwony  
};
int currentColorIndex = 0;
NOTIFYICONDATA nid = {};
HANDLE hMutex; // Uchwyt dla Mutexa  

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        int centerX = (rect.right - rect.left) / 2;
        int centerY = ((rect.bottom - rect.top) / 2) + offsetY;

        if (dotSize == 1) {
            SetPixel(hdc, centerX, centerY, colors[currentColorIndex]);
        }
        else {
            HBRUSH brush = CreateSolidBrush(colors[currentColorIndex]);
            RECT dotRect = { centerX - 1, centerY - 1, centerX + 1, centerY + 1 };
            FillRect(hdc, &dotRect, brush);
            DeleteObject(brush);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 2)
        {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_HOTKEY:
    {
        static bool visible = true;
        switch (wParam) {
        case 1: // CTRL + F1: Show/Hide  
            visible = !visible;
            ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
            break;
        case 2: // CTRL + F4: Exit  
            DestroyWindow(hwnd);
            break;
        case 3: // CTRL + F3: Change color  
            currentColorIndex = (currentColorIndex + 1) % 5;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case 4: // CTRL + PAGE UP: GORA  
            offsetY -= 1;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case 5: // CTRL + PAGE DOWN: DOL  
            offsetY += 1;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case 6: // CTRL + HOME: RESET  
            offsetY = 0;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case 7: // CTRL + F2: Toggle Size
            dotSize = (dotSize == 2) ? 1 : 2;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        return 0;
    }

    case WM_TRAYICON:
    {
        if (lParam == WM_RBUTTONUP)
        {
            POINT curPoint;
            GetCursorPos(&curPoint);
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 2, "Exit");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, curPoint.x, curPoint.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONDBLCLK)
        {
            static bool visible = true;
            visible = !visible;
            ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
        }
        return 0;
    }

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        for (int i = 1; i <= 7; i++) UnregisterHotKey(hwnd, i);
        if (hMutex) {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    hMutex = CreateMutex(NULL, TRUE, "Global\\KukaCrosshairMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    const char CLASS_NAME[] = "OverlayWindow";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);

    int screenX = GetSystemMetrics(SM_CXSCREEN);
    int screenY = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        CLASS_NAME, "Overlay", WS_POPUP,
        0, 0, screenX, screenY,
        NULL, NULL, hInstance, NULL
    );

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    lstrcpy(nid.szTip, "KUKA REMINDER: CTRL+F1 (Toggle), CTRL+F2 (Size), CTRL+F3 (Color), CTRL+F4 (Exit), CTRL+PGUP/PGDN (Adjust), CTRL+HOME (Reset)");
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    RegisterHotKey(hwnd, 1, MOD_CONTROL, VK_F1);    // Toggle Show/Hide
    RegisterHotKey(hwnd, 7, MOD_CONTROL, VK_F2);    // Toggle Size
    RegisterHotKey(hwnd, 3, MOD_CONTROL, VK_F3);    // Change Color
    RegisterHotKey(hwnd, 2, MOD_CONTROL, VK_F4);    // Exit
    RegisterHotKey(hwnd, 4, MOD_CONTROL, VK_PRIOR); // Page Up Adjust
    RegisterHotKey(hwnd, 5, MOD_CONTROL, VK_NEXT);  // Page Down Adjust 
    RegisterHotKey(hwnd, 6, MOD_CONTROL, VK_HOME);  // Reset

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
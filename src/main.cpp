#ifndef UNICODE
#define UNICODE
#endif 

#include "MainWindow.h"

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int nCmdShow)
{
    MainWindow win;

    if (!win.Create(L"LibreTerm", WS_OVERLAPPEDWINDOW))
    {
        return 0;
    }

    win.Show(nCmdShow);

    HACCEL hAccel = LoadAccelerators(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ACCELERATOR));

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
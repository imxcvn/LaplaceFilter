#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <commctrl.h>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <codecvt>
#include <fstream>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <Commctrl.h>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Comctl32.lib")

// Typ funkcji przetwarzaj¹cej fragment obrazu
typedef void (*ProcessFunction)(unsigned char*, unsigned char*, int, int);

struct ImageFragment {
    unsigned char* pixels;  // WskaŸnik na fragment tablicy pikseli bitmapy
    unsigned char* outputPixels;  // WskaŸnik na fragment tablicy wyjœciowej
    int width;              // Szerokoœæ fragmentu
    int height;             // Wysokoœæ fragmentu
    ProcessFunction processFunc;  // Wybrana funkcja
};

extern "C" void ThreadStartProc(unsigned char* pixel, unsigned char* outputPixels, int width, int height);
extern "C" __declspec(dllexport) void ProcessImageFragment(unsigned char* pixel, unsigned char* outputPixels, int width, int height);

void ThreadFunction(ImageFragment* fragment)
{
    // Wywolanie funkcji przypisanej do danego fragmentu
    fragment->processFunc(fragment->pixels, fragment->outputPixels, fragment->width, fragment->height);
}

// Funkcja pobieraj¹ca œcie¿kê do pulpitu
std::wstring GetDesktopFolder() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L""; // W razie b³êdu zwraca pusty string
}

// Funkcja ³aduj¹ca bitmapê z pliku
HBITMAP LoadBitmapFromFile(LPCWSTR filename)
{
    return (HBITMAP)LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}

// Funkcja do zapisu bitmapy do pliku
bool SaveBitmapToFile(const unsigned char* pixels, int width, int height, const char* filename)
{
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = {};

    // Ustawienia nag³ówka pliku BMP
    fileHeader.bfType = 0x4D42;  // 'BM'
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + width * height * 3;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;

    // Ustawienia nag³ówka informacji BMP
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = -height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    FILE* file = fopen(filename, "wb");
    if (!file) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return false;
    }

    // Zapis nag³ówków i pikseli
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);
    fwrite(pixels, 3, width * height, file);

    fclose(file);
    return true;
}

HWND g_hWnd = nullptr;  // Uchwyt do okna
HWND hEdit1, hEdit2, hEdit4;  // Uchwyty do pól edycyjnych
HWND hLabel1, hLabel2, hLabel3, hLabel4;  // Uchwyty do napisów nad polami edycyjnymi
HWND hCheckBoxASM, hCheckBoxCPP;  // Uchwyty do check boxow 

// Prototyp funkcji okna
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Funkcja tworz¹ca okno
bool CreateAppWindow(HINSTANCE hInstance, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"ImageProcessingApp";
    HWND hButtonStart;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(255, 184, 255));
    wc.hIcon = (HICON)LoadImage(nullptr, L"cat-_1_.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);

    if (!wc.hIcon) {
        MessageBox(nullptr, L"Nie uda³o siê za³adowaæ ikony!", L"B³¹d", MB_ICONERROR);
    }

    // Rejestracja klasy okna
    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Tworzenie okna
    g_hWnd = CreateWindowEx(0, CLASS_NAME, L"Image Processing App", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd) {
        MessageBox(nullptr, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    hButtonStart = CreateWindowEx(0, L"BUTTON", L"Start Processing", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 325, 450, 150, 30, g_hWnd, (HMENU)1, hInstance, nullptr);

    // Tworzenie pól edycyjnych
    hEdit1 = CreateWindowEx(0x00000200L, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 225, 250, 350, 25, g_hWnd, nullptr, hInstance, nullptr);
    hEdit2 = CreateWindowEx(0x00000200L, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 225, 300, 350, 25, g_hWnd, nullptr, hInstance, nullptr);
    hEdit4 = CreateWindowEx(0x00000200L, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 225, 350, 350, 25, g_hWnd, nullptr, hInstance, nullptr);
    hCheckBoxASM = CreateWindowEx(0, L"BUTTON", L"ASM", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 225, 405, 150, 20, g_hWnd, nullptr, hInstance, nullptr);
    hCheckBoxCPP = CreateWindowEx(0, L"BUTTON", L"C++", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 400, 405, 150, 20, g_hWnd, nullptr, hInstance, nullptr);

    // Tworzenie napisów nad polami edycyjnymi
    hLabel1 = CreateWindowEx(0, L"STATIC", L"Choose a file to process (only .bmp files are allowed):", WS_CHILD | WS_VISIBLE, 225, 230, 350, 20, g_hWnd, nullptr, hInstance, nullptr);
    hLabel2 = CreateWindowEx(0, L"STATIC", L"Enter the number of threads (max 64):", WS_CHILD | WS_VISIBLE, 225, 280, 350, 20, g_hWnd, nullptr, hInstance, nullptr);
    hLabel4 = CreateWindowEx(0, L"STATIC", L"File name to save the processed image (with .bmp):", WS_CHILD | WS_VISIBLE, 225, 330, 350, 20, g_hWnd, nullptr, hInstance, nullptr);
    hLabel3 = CreateWindowEx(0, L"STATIC", L"Choose processing method:", WS_CHILD | WS_VISIBLE, 225, 382, 350, 20, g_hWnd, nullptr, hInstance, nullptr);

    ShowWindow(g_hWnd, nCmdShow);
    return true;
}

// Funkcja obs³ugi zdarzeñ okna
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {  // ID przycisku Start
            wchar_t buffer[1024];
            int processingMethod;
            int numThreads;
            std::wstring desktopFolder;
            std::wstring fullPath;
            std::string output;
            LPCWSTR fn;

            // Pobieranie danych z pól edycyjnych
            GetWindowText(hEdit1, buffer, 1024);
            try {
                std::string fileName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(buffer);
                if (fileName.empty()) {
                    MessageBox(hwnd, L"File name field cannot be empty.", L"Invalid Input", MB_OK | MB_ICONERROR);
                    return 0;
                }
                desktopFolder = GetDesktopFolder();
                fullPath = desktopFolder + L"\\" + std::wstring(fileName.begin(), fileName.end());
                fn = fullPath.c_str();
                if (!PathFileExists(fn)) {
                    MessageBox(g_hWnd, L"The specified file does not exist.", L"File Error", MB_OK | MB_ICONERROR);
                    return 0;
                }
                if (fileName.length() < 4 || fileName.compare(fileName.length() - 4, 4, ".bmp") != 0) {
                    MessageBox(hwnd, L"Chosen file is not a valid .bmp file.", L"Invalid Input", MB_OK | MB_ICONERROR);
                    return 0;
                }
            }
            catch (...) {
                MessageBox(hwnd, L"Invalid input for the name of the file to be processed.", L"Error", MB_OK | MB_ICONERROR);
                return 0;
            }

            GetWindowText(hEdit2, buffer, 1024);
            try {
                numThreads = std::stoi(buffer);
                if (numThreads < 1 || numThreads > 64) {
                    MessageBox(hwnd, L"Number of threads must be between 1 and 64.", L"Invalid Input", MB_OK | MB_ICONERROR);
                    return 0;
                }
            }
            catch (...) {
                MessageBox(hwnd, L"Invalid input for the number of threads.", L"Error", MB_OK | MB_ICONERROR);
                return 0;
            }

            GetWindowText(hEdit4, buffer, 1024);
            try {
                output = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(buffer);
                if (output.empty()) {
                    MessageBox(hwnd, L"Output file name field cannot be empty.", L"Invalid Input", MB_OK | MB_ICONERROR);
                    return 0;
                }
                if (output.length() < 4 || output.compare(output.length() - 4, 4, ".bmp") != 0) {
                    MessageBox(hwnd, L"Output file name must end with '.bmp'.", L"Invalid Input", MB_OK | MB_ICONERROR);
                    return 0;
                }
            }
            catch (...) {
                MessageBox(hwnd, L"Invalid input for the output file name.", L"Error", MB_OK | MB_ICONERROR);
                return 0;
            }

            try {
                if (SendMessage(hCheckBoxASM, BM_GETCHECK, 0, 0) == BST_CHECKED && SendMessage(hCheckBoxCPP, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    MessageBox(hwnd, L"Please select only one processing method.", L"Error", MB_OK | MB_ICONERROR);
                    return 0;
                }

                if (SendMessage(hCheckBoxASM, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    processingMethod = 1;
                }
                else if (SendMessage(hCheckBoxCPP, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    processingMethod = 2;
                }
                else {
                    MessageBox(hwnd, L"Please select a processing method.", L"Error", MB_OK | MB_ICONERROR);
                    return 0;
                }
            }
            catch (...) {
                MessageBox(hwnd, L"Invalid input for processing method.", L"Error", MB_OK | MB_ICONERROR);
                return 0;
            }

            // £adowanie obrazu
            HBITMAP hBitmap = LoadBitmapFromFile(fn);
            if (!hBitmap) {
                MessageBox(g_hWnd, L"The specified file does not exist or is not a valid .bmp file.", L"File Error", MB_OK | MB_ICONERROR);
                return -1;
            }

            BITMAP bitmap;
            GetObject(hBitmap, sizeof(BITMAP), &bitmap);

            int width = bitmap.bmWidth;
            int height = bitmap.bmHeight;

            BITMAPINFO bmpInfo = {};
            bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmpInfo.bmiHeader.biWidth = width;
            bmpInfo.bmiHeader.biHeight = -height;
            bmpInfo.bmiHeader.biPlanes = 1;
            bmpInfo.bmiHeader.biBitCount = 24;
            bmpInfo.bmiHeader.biCompression = BI_RGB;
            bmpInfo.bmiHeader.biSizeImage = 0;
            bmpInfo.bmiHeader.biXPelsPerMeter = 0;
            bmpInfo.bmiHeader.biYPelsPerMeter = 0;
            bmpInfo.bmiHeader.biClrUsed = 0;
            bmpInfo.bmiHeader.biClrImportant = 0;

            // Konwersja bitmapy na tablicê pikseli
            unsigned char* pixels = new unsigned char[width * height * 3];
            unsigned char* outputPixels = new unsigned char[width * height * 3];

            HDC hdc = GetDC(NULL);

            if (!GetDIBits(hdc, hBitmap, 0, height, pixels, &bmpInfo, DIB_RGB_COLORS)) {
                std::cerr << "Failed to retrieve bitmap bits using GetDIBits." << std::endl;
                delete[] pixels;
                delete[] outputPixels;
                DeleteObject(hBitmap);
                ReleaseDC(NULL, hdc);
                return -1;
            }

            if (!GetDIBits(hdc, hBitmap, 0, height, outputPixels, &bmpInfo, DIB_RGB_COLORS)) {
                std::cerr << "Failed to retrieve bitmap bits using GetDIBits." << std::endl;
                delete[] pixels;
                delete[] outputPixels;
                DeleteObject(hBitmap);
                ReleaseDC(NULL, hdc);
                return -1;
            }

            ReleaseDC(NULL, hdc);

            // Pobieranie danych z GUI
            ProcessFunction processFunction = nullptr;

            if (processingMethod == 1) {
                processFunction = ThreadStartProc;
            }
            else if (processingMethod == 2) {
                processFunction = ProcessImageFragment;
            }
            else {
                std::cerr << "Invalid choice." << std::endl;
                return -1;
            }

            // Podzia³ obrazu na fragmenty
            int fragmentHeight = height / numThreads;
            int remainingHeight = height % numThreads;
            std::vector<std::thread> threads;
            std::vector<ImageFragment> fragments(numThreads);

            auto startTime = std::chrono::high_resolution_clock::now();

            // Tworzenie watkow i przetwarzanie fragmentow
            for (int i = 0; i < numThreads; i++) {

                int startY = i * fragmentHeight + min(i, remainingHeight);  // Ustalenie poczateku fragmentu z uwzglednieniem dodatkowych pikseli
                int endY = startY + fragmentHeight + (i < remainingHeight ? 1 : 0);  // Dodanie dodatkowych pikseli do fragmentu, jesli i < remainingHeight

                int fragmentSize = endY - startY;  // Rozmiar fragmentu (bez paddingu)

                // Obliczenie wysokosci fragmentu z paddingiem
                int paddedHeight = fragmentSize + (i > 0 ? 1 : 0) + (i < numThreads - 1 ? 1 : 0);  // Dodatkowy wiersz na gorze i na dole, pod warunkiem

                // Ustalenie poczateku fragmentu z paddingiem
                fragments[i].pixels = &pixels[(startY - (i > 0 ? 1 : 0)) * width * 3];
                fragments[i].outputPixels = &outputPixels[(startY - (i > 0 ? 1 : 0)) * width * 3];

                fragments[i].width = width;
                fragments[i].height = paddedHeight;


                fragments[i].processFunc = processFunction;

                threads.emplace_back(ThreadFunction, &fragments[i]);

            }


            // Czekanie na zakoñczenie przetwarzania
            for (auto& t : threads) {
                t.join();
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            // Zapis przetworzonego obrazu
            std::string outputFileName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(desktopFolder + L"\\" + std::wstring(output.begin(), output.end()));
            SaveBitmapToFile(outputPixels, width, height, outputFileName.c_str());

            std::wstring resultMessage = L"Processing completed in " + std::to_wstring(duration.count()) + L" milliseconds. Your changed image has been saved to the desktop.";
            MessageBox(NULL, resultMessage.c_str(), L"Processing Time", MB_OK | MB_ICONINFORMATION);

            // Czyszczenie pamiêci
            delete[] pixels;
            delete[] outputPixels;
            DeleteObject(hBitmap);
            pixels = nullptr;
            outputPixels = nullptr;

            return 0;
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Tworzenie czcionki
        HFONT hFont = CreateFont(
            70, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Elephant"
        );
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(79, 3, 60));

        // Rysowanie tekstu 
        RECT rect;
        GetClientRect(hwnd, &rect);
        rect.top += 100;

        DrawText(hdc, L"Laplace filter", -1, &rect, DT_SINGLELINE | DT_CENTER);

        SelectObject(hdc, oldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetTextColor(hdcStatic, RGB(59, 2, 45));
        static HBRUSH hBrush = CreateSolidBrush(RGB(255, 184, 255));
        return (LRESULT)hBrush;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    // Tworzenie okna aplikacji
    if (!CreateAppWindow(hInstance, nCmdShow)) {
        return -1;
    }

    // Pêtla wiadomoœci
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

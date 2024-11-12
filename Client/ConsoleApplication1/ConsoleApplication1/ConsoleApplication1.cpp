#include <iostream> // Подключение библиотеки для ввода-вывода
#include <winsock2.h> // Подключение библиотеки для работы с сокетами Windows
#include <windows.h> // Подключение библиотеки Windows API
#include <string> // Подключение библиотеки для работы со строками
#include <thread> // Подключение библиотеки для работы с потоками
#include <chrono> // Подключение библиотеки для работы со временем
#include <ws2tcpip.h> // Подключение библиотеки для расширенных функций сокетов
#include <fstream> // Подключение библиотеки для работы с файлами
#include <ostream> // Подключение библиотеки для работы с потоками вывода

#pragma comment(lib, "ws2_32.lib") // Указание компилятору подключить библиотеку ws2_32.lib

using namespace std; // Использование стандартного пространства имен

// Функция для отправки активности пользователя на сервер
void sendActivity(const string& serverIp, int port) {
    WSADATA wsaData; // Структура для хранения информации о версии Winsock
    SOCKET sock; // Дескриптор сокета
    struct sockaddr_in server; // Структура для хранения адреса сервера
    WORD wVersionRequested = MAKEWORD(2, 2); // Запрос версии Winsock 2.2
    WSAStartup(wVersionRequested, &wsaData); // Инициализация Winsock
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Создание UDP сокета
    server.sin_family = AF_INET; // Установка семейства адресов (IPv4)
    server.sin_port = htons(port); // Установка порта сервера (преобразование в сетевой порядок байтов)
    inet_pton(AF_INET, serverIp.c_str(), &server.sin_addr); // Преобразование IP-адреса из строки в бинарный формат

    while (true) { // Бесконечный цикл
        char username[256]; // Массив для хранения имени пользователя
        DWORD username_len = sizeof(username); // Длина массива имени пользователя
        GetUserNameA(username, &username_len); // Получение имени текущего пользователя
        string message = string(username) + " is active"; // Формирование сообщения о активности пользователя
        sendto(sock, message.c_str(), sizeof(message), 0, (struct sockaddr*)&server, sizeof(server)); // Отправка сообщения на сервер
        this_thread::sleep_for(chrono::seconds(5)); // Пауза на 5 секунд
    }

    closesocket(sock);
    WSACleanup();
}

// Функция для создания скриншота рабочего стола
void takeScreenshot() {
    HWND hwndDesktop = GetDesktopWindow(); // Получение дескриптора рабочего стола
    HDC hdcDesktop = GetDC(hwndDesktop); // Получение контекста устройства рабочего стола
    HDC hdcMem = CreateCompatibleDC(hdcDesktop); // Создание совместимого контекста устройства
    RECT desktopRect; // Структура для хранения размеров рабочего стола
    GetWindowRect(hwndDesktop, &desktopRect); // Получение размеров рабочего стола
    int width = desktopRect.right - desktopRect.left; // Ширина рабочего стола
    int height = desktopRect.bottom - desktopRect.top; // Высота рабочего стола
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcDesktop, width, height); // Создание битмапа совместимого размера
    SelectObject(hdcMem, hBitmap); // Выбор битмапа в контекст устройства памяти
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY); // Копирование изображения рабочего стола в битмап

    BITMAP bmp; // Структура для хранения информации о битмапе
    GetObject(hBitmap, sizeof(BITMAP), &bmp); // Получение информации о созданном битмапе

    BITMAPFILEHEADER bmfHeader; // Заголовок файла битмапа
    bmfHeader.bfType = 0x4D42; // Установка типа файла (BM)
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmp.bmWidthBytes * bmp.bmHeight; // Общий размер файла
    bmfHeader.bfReserved1 = 0; // Резервное поле
    bmfHeader.bfReserved2 = 0; // Резервное поле
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Смещение до данных изображения

    BITMAPINFOHEADER bi; // Заголовок информации о битмапе
    bi.biSize = sizeof(BITMAPINFOHEADER); // Размер заголовка информации о битмапе
    bi.biWidth = bmp.bmWidth; // Ширина изображения
    bi.biHeight = bmp.bmHeight; // Высота изображения
    bi.biPlanes = 1; // Количество цветовых плоскостей
    bi.biBitCount = 32; // Количество бит на пиксель (32 бита)
    bi.biCompression = BI_RGB; // Тип сжатия (без сжатия)
    bi.biSizeImage = bmp.bmWidth * bmp.bmHeight; // Размер изображения в байтах
    bi.biXPelsPerMeter = 0; // Горизонтальное разрешение (не используется)
    bi.biYPelsPerMeter = 0; // Вертикальное разрешение (не используется)
    bi.biClrUsed = 0; // Количество используемых цветов (не используется)
    bi.biClrImportant = 0; // Количество важных цветов (не используется)

    errno_t err;
    FILE* file;
    err = fopen_s(&file, "screenshot.bmp", "wb"); // Открытие файла для записи в бинарном режиме

    fwrite(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, file); // Запись заголовка файла битмапа в файл
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, file); // Запись заголовка информации о битмапе в файл

    BYTE* pixels = new BYTE[bmp.bmWidthBytes * bmp.bmHeight]; // Выделение памяти для пикселей изображения
    GetDIBits(hdcMem, hBitmap, 0, bmp.bmHeight, pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS); // Получение данных пикселей из битмапа

    fwrite(pixels, bmp.bmWidthBytes * bmp.bmHeight, 1, file); // Запись данных пикселей в файл

    delete[] pixels; // Освобождение памяти, выделенной под пиксели
    fclose(file); // Закрытие файла

    DeleteObject(hBitmap); // Удаление объекта битмапа из памяти
    DeleteDC(hdcMem); // Удаление контекста устройства памяти
    ReleaseDC(hwndDesktop, hdcDesktop); // Освобождение контекста устройства рабочего стола
}

// Главная функция программы
int main()
{
    string serverIp = "127.0.0.1"; // IP-адрес сервера (локальный адрес)
    int port = 8888; // Порт сервера
    thread activityThread(sendActivity, serverIp, port); // Создание потока для отправки активности пользователя на сервер
    activityThread.detach(); // Отделение потока (он будет работать независимо)

    while (true) { // Бесконечный цикл
        takeScreenshot(); // Вызов функции для создания скриншота
        this_thread::sleep_for(chrono::seconds(30)); // Пауза на 30 секунд перед следующим скриншотом
    }

    return 0; // Возврат из главной функции (не будет достигнуто из-за бесконечного цикла)
}

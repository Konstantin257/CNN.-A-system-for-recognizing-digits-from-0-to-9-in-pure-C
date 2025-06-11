//1. Матрица весов и вектор коэффициентов смещения.bin
//1. Тестовая выборка.bin
//1. Фильтр Собеля тест.bin
//1. Отпуллингованная тестовая выборка.bin




#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <math.h>
#include <time.h>
#include <tchar.h>



// Глобальные переменные
double W[196][10], b[10], x[196];
int current_label=0; // Текущая метка (цифра) обрабатываемого изображения
int label_original, height_original=28, width_original=28; //  Высота и ширина оригинального изображения (28x28 пикселей)
double pixels_original[784]; // Массив пикселей оригинального изображения (28x28=784)
int current_prediction=0; //  Результат распознавания текущего изображения (цифра от 0 до 9).
int sum[10]={0};
int total_images=0, total_images_2=0; // Общее количество изображений в тестовой выборке для разных режимов 
// (например, total_images для "3. Сфотографированные", total_images_2 для "2. Paint").
// 

int current_offset=0, current_offset_1=0, current_offset_2=0; // Текущее смещение в файле для чтения изображений 
// (используется для перемотки файла при достижении конца)
FILE* pooling_f_wb=NULL;
FILE* original_f=NULL;
// Глобальная переменная, которая отвечает за то, чтобы отрисованное слева изображение оставалось на экране 
// и не пропадало до нажатия следующей кнопки
int currentMode=0;

// Переменные для рисования
int drawingAreaX=450; 
int drawingAreaY=300; // Координаты области рисования на экране.
int drawingCellSize=10; // Размер одной ячейки (пикселя) в области рисования (по умолчанию 10) - от неё зависит размер области рисования (черное окно)
double drawingPixels[784]={0}; // Массив пикселей для хранения нарисованного изображения (28x28=784)
int isDrawing=0; // Флаг, указывающий, происходит ли в данный момент рисование (1 — да, 0 — нет)



// 4. Своё окно
//
// Функция отрисовки цифры в пиксельном виде в левом окне
void DrawDigit_4(HDC hdc, int x_pos, int y_pos, int cellSize) {
    HBRUSH hBrush=CreateSolidBrush(RGB(0, 255, 0));
    HPEN hPen=CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

    HBRUSH hOldBrush=(HBRUSH) SelectObject(hdc, hBrush);
    HPEN hOldPen=(HPEN) SelectObject(hdc, hPen);

    for (int row=0; row < height_original; row++){
        for (int col=0; col < width_original; col++){
            if (pixels_original[row*width_original + col] > 0.5){
                int left=x_pos + col*cellSize;
                int top=y_pos + row*cellSize;
                Rectangle(hdc, left, top, left + cellSize, top + cellSize);
            }
        }
    }

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

// Функция, которая рисует белые пиксели на черном окне в том месте, где было проведено курсором с зажатой ЛКМ
void DrawDrawingArea_4(HDC hdc) {
    // структура RECT — прямоугольник, который задаёт область рисования.
    // - Координаты:
    // -Верхний левый угол:(drawingAreaX, drawingAreaY)
    // - Нижний правый угол:(drawingAreaX + width_original*drawingCellSize, drawingAreaY + height_original*drawingCellSize)
    // - width_original и height_original — размеры области в "ячейках".
    // - drawingCellSize — размер одной ячейки(пикселя) в пикселях экрана.
    RECT drawingRect={
        drawingAreaX,
        drawingAreaY,
        drawingAreaX + width_original*drawingCellSize,
        drawingAreaY + height_original*drawingCellSize
    };
    FillRect(hdc, &drawingRect, (HBRUSH) GetStockObject(BLACK_BRUSH));

    // -Двойной цикл по строкам и столбцам области.
    // - Для каждой ячейки проверяется значение в массиве drawingPixels(массив типа double).
    // - Если значение больше 0.5 (то есть "пиксель активен"), рисуется белый прямоугольник размером drawingCellSize на drawingCellSize в соответствующей позиции.
    // - Координаты прямоугольника рассчитываются с учётом смещения drawingAreaX, drawingAreaY и размера ячейки.
    HBRUSH hWhiteBrush=CreateSolidBrush(RGB(255, 255, 255));
    for (int row=0; row < height_original; row++){
        for (int col=0; col < width_original; col++){
            if (drawingPixels[row*width_original + col] == 0.5){// == 1.0
                RECT pixelRect={
                    drawingAreaX + col*drawingCellSize,
                    drawingAreaY + row*drawingCellSize,
                    drawingAreaX + (col+1)*drawingCellSize,
                    drawingAreaY + (row+1)*drawingCellSize
                };
                FillRect(hdc, &pixelRect, hWhiteBrush);
            }
        }
    }
    DeleteObject(hWhiteBrush);
}

// Функция, которая отвечает за рисование белой кисточкой по черному окну
void UpdateDrawing_4(int x, int y) {
    int col=(x - drawingAreaX) / drawingCellSize;
    int row=(y - drawingAreaY) / drawingCellSize;

    if (col >= 0 && col < width_original && row >= 0 && row < height_original){
        drawingPixels[row*width_original + col]=1.0;

        // Также устанавливаем соседние пиксели для более плавного рисования
        // if (col > 0) drawingPixels[row * width_original + col - 1]=1.0;
        // if (col < width_original - 1) drawingPixels[row * width_original + col + 1]=1.0;
        // if (row > 0) drawingPixels[(row - 1) * width_original + col]=1.0;
        // if (row < height_original - 1) drawingPixels[(row + 1) * width_original + col]=1.0;
    }
}

// Функция очистки черного окна рисования
void ClearDrawingArea_4() {
    for (int i=0; i < 784; i++){
        drawingPixels[i]=0.0;
    }
}

// Функция мгновенного сохранения нарисованного изображения в бинарном файле без сохранения в PNG
void SaveDrawingToBinary_4() {
    FILE* fp=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Тестовая выборка.bin"), _T("wb"));
    if (!fp){
        MessageBox(NULL, _T("Ошибка открытия файла для записи"), _T("Ошибка"), MB_ICONERROR);
        return;
    }
    int label=0; // Метка для пользовательского рисунка
    int height=28;
    int width=28;

    fwrite(&label, sizeof(int), 1, fp);
    fwrite(&height, sizeof(int), 1, fp);
    fwrite(&width, sizeof(int), 1, fp);
    fwrite(drawingPixels, sizeof(double), 784, fp);
    fclose(fp);

    fp=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Тестовая выборка.bin"), _T("rb"));
    if (!fp){
        MessageBox(NULL, _T("Ошибка открытия файла для чтения"), _T("Ошибка"), MB_ICONERROR);
        return;
    }

    double loadedPixels[784];
    if (fread(&label, sizeof(int), 1, fp)!=1){ MessageBox(NULL, _T("Ошибка чтения метки"), _T("Ошибка"), MB_ICONERROR); }
    if (fread(&height, sizeof(int), 1, fp)!=1){ MessageBox(NULL, _T("Ошибка чтения высоты"), _T("Ошибка"), MB_ICONERROR); }
    if (fread(&width, sizeof(int), 1, fp)!=1){ MessageBox(NULL, _T("Ошибка чтения ширины"), _T("Ошибка"), MB_ICONERROR); }
    if (fread(pixels_original, sizeof(double), 784, fp)!=784){ MessageBox(NULL, _T("Ошибка чтения пикселей"), _T("Ошибка"), MB_ICONERROR); }
    fclose(fp);

    // Обновляем окно для отрисовки
    HWND hwnd=FindWindow(_T("CNN Viewer"), NULL);
    if (hwnd){
        // Получаем контекст устройства
        HDC hdc=GetDC(hwnd);

        // Отрисовываем изображение в левом окне (координаты 50, 200, размер ячейки 15)
        DrawDigit_4(hdc, 50, 200, 15);

        // Освобождаем контекст устройства
        ReleaseDC(hwnd, hdc);

        //// Принудительно обновляем окно
        InvalidateRect(hwnd, NULL, TRUE);
    }


    label_original=-1; // Указываем, что метки нет (работаем без метки, просто рисуем и распознаём)

    //// После сохранения обновите данные для распознавания
    current_offset=0; // Сбросьте offset
    total_images=1;   // Теперь в файле только одно изображение
}

// Функция фильтрации изображения
void apply_filter_to_all_test_4() {
    FILE* fp_wb=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Тестовая выборка.bin"), _T("rb"));
    FILE* filterf=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Фильтр Собеля тест.bin"), _T("wb"));

    if (!fp_wb || !filterf){
        MessageBox(NULL, _T("Ошибка открытия файлов для фильтрации"), _T("Ошибка"), MB_ICONERROR);
        return;
    }

    int label, height, width;
    if (fread(&label, sizeof(int), 1, fp_wb) != 1) MessageBox(NULL, _T("Ошибка открытия файлов для фильтрации"), _T("Ошибка"), MB_ICONERROR);;
    if (fread(&height, sizeof(int), 1, fp_wb) != 1) MessageBox(NULL, _T("Ошибка открытия файлов для фильтрации"), _T("Ошибка"), MB_ICONERROR);;
    if (fread(&width, sizeof(int), 1, fp_wb) != 1) MessageBox(NULL, _T("Ошибка открытия файлов для фильтрации"), _T("Ошибка"), MB_ICONERROR);;

    double image_data[784];
    if (fread(image_data, sizeof(double), 784, fp_wb) != 784) MessageBox(NULL, _T("Ошибка открытия файлов для фильтрации"), _T("Ошибка"), MB_ICONERROR);;

    // Фильтр 3x3
    int filter1[9]={-1, -1, -1,
        0, 0, 0,
        1, 1, 1};
    int filter2[9]={-1, 0, 1,
        -1, 0, 1,
        -1, 0, 1};

    double padded[30*30]={1.0};
    for (int y=0; y < 28; y++){
        for (int x=0; x < 28; x++){
            padded[(y+1) * 30 + (x+1)]=image_data[y * 28 + x];
        }
    }

    double filtered1[28 * 28]={0.0};
    double filtered2[28 * 28]={0.0};
    double combined[28 * 28]={0.0};

    // Применение свёртки
    for (int y=0; y < 28; y++){
        for (int x=0; x < 28; x++){
            double sum1=0.0;
            double sum2=0.0;
            // Проход по фильтру 3x3
            for (int ky=0; ky < 3; ky++){
                for (int kx=0; kx < 3; kx++){
                    sum1+=padded[(y+ky) * 30 + (x+kx)] * filter1[ky * 3 + kx];
                    sum2+=padded[(y+ky) * 30 + (x+kx)] * filter2[ky * 3 + kx];
                }
            }
            filtered1[y * 28 + x]=sum1;
            filtered2[y * 28 + x]=sum2;
        }
    }
    for (int i=0; i < 28*28; i++){
        if (i<28 || i>755 || i%28==0 || i%28==27){ combined[i]=0.0; }
        else{ combined[i]=sqrt(filtered1[i] * filtered1[i] + filtered2[i] * filtered2[i]); }
    }

    double max_val=0.0;
    for (int i=0; i < 28*28; i++){
        combined[i]=1.0 - combined[i];  // Инверсия: границы станут белыми
        if (combined[i] > max_val) max_val=combined[i];
    }

    // Нормализация
    if (max_val > 0){
        for (int i=0; i < 28*28; i++){
            combined[i]/=max_val;
        }
    }
    fwrite(&label, sizeof(int), 1, filterf);
    int hw=28;
    fwrite(&hw, sizeof(int), 1, filterf);
    fwrite(&hw, sizeof(int), 1, filterf);
    fwrite(combined, sizeof(double), 784, filterf);


    fclose(fp_wb);
    fclose(filterf);
}

// Функция пуллинга (уменьшения изображения до размера 14*14)
void pooling_test_4() {
    FILE* pooling_f_rb=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Фильтр Собеля тест.bin"), _T("rb"));
    FILE* pooling_f_wb=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Отпуллингованная тестовая выборка.bin"), _T("wb"));

    if (!pooling_f_rb || !pooling_f_wb){
        MessageBox(NULL, _T("Ошибка открытия файлов для пуллинга"), _T("Ошибка"), MB_ICONERROR);
        return;
    }

    int label, height, width;
    double image[784];

    if (fread(&label, sizeof(int), 1, pooling_f_rb) != 1 ||
        fread(&height, sizeof(int), 1, pooling_f_rb) != 1 ||
        fread(&width, sizeof(int), 1, pooling_f_rb) != 1 ||
        fread(image, sizeof(double), 784, pooling_f_rb) != 784){
        MessageBox(NULL, _T("Ошибка чтения изображения"), _T("Ошибка"), MB_ICONERROR);
        fclose(pooling_f_rb);
        fclose(pooling_f_wb);
        return;
    }

    // Применяем пуллинг (max pooling 2x2)
    int s=0;
    double max[14*14];

    for (int y=0; y<28; y+=2){
        for (int x=0; x<28; x+=2){
            max[s]=2.0;
            for (int ky=0; ky<2; ky++){
                for (int kx=0; kx<2; kx++){
                    if (image[(y+ky) * 28 + (x+kx)]<max[s]){ max[s]=image[(y+ky) * 28 + (x+kx)]; }
                }
            }
            s++;
        }
    }

    int hw=14;
    fwrite(&label, sizeof(int), 1, pooling_f_wb);
    fwrite(&hw, sizeof(int), 1, pooling_f_wb);
    fwrite(&hw, sizeof(int), 1, pooling_f_wb);
    fwrite(max, sizeof(double), 196, pooling_f_wb);

    fclose(pooling_f_rb);
    fclose(pooling_f_wb);
}

// Функция приведения изображения к виду, готовому к распознаванию
void testing_4(int offset) {

    FILE* W_b=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Матрица весов и вектор коэффициентов смещения.bin"), _T("rb"));
    if (W_b){
        fread(W, sizeof(double), 196 * 10, W_b);
        fread(b, sizeof(double), 10, W_b);
        fclose(W_b);
    }

    FILE* test_file=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Тестовая выборка.bin"), _T("rb"));
    FILE* pooling_f_wb_4=_tfopen(_T("Обученные файлы\\4. Своё окно\\1. Отпуллингованная тестовая выборка.bin"), _T("rb"));

    //MessageBox(NULL, _T(/*"%d", current_prediction*/"аааааааа"), _T("Успех"), MB_OK);


    // Читаем текущее изображение из пуллингованного файла
    fread(&current_label, sizeof(int), 1, pooling_f_wb_4);
    fread(&height_original, sizeof(int), 1, pooling_f_wb_4);
    fread(&width_original, sizeof(int), 1, pooling_f_wb_4);
    fread(x, sizeof(double), 196, pooling_f_wb_4);

    // Читаем соответствующее оригинальное изображение
    fread(&label_original, sizeof(int), 1, test_file);
    fread(&height_original, sizeof(int), 1, test_file);
    fread(&width_original, sizeof(int), 1, test_file);
    fread(pixels_original, sizeof(double), 784, test_file);

    // Обновляем окно для отрисовки
    HWND hwnd=FindWindow(_T("CNN Viewer"), NULL);
    if (hwnd){
        // Получаем контекст устройства
        HDC hdc=GetDC(hwnd);

        // Отрисовываем изображение в левом окне (координаты 50, 200, размер ячейки 15)
        DrawDigit_4(hdc, 50, 200, 15);

        // Освобождаем контекст устройства
        ReleaseDC(hwnd, hdc);

        //// Принудительно обновляем окно
        InvalidateRect(hwnd, NULL, TRUE);
    }

    // Распознаем изображение
    double z[10]={0};
    for (int m=0; m < 10; m++){
        for (int n=0; n < 196; n++){
            z[m]+=x[n] * W[n][m];
        }
        z[m]+=b[m];
    }

    double max=z[0];
    for (int m=1; m < 10; m++) if (z[m] > max) max=z[m];

    double yy[10]={0};
    double sum_exp=0.0;
    for (int m=0; m < 10; m++){
        yy[m]=exp(z[m] - max);
        sum_exp+=yy[m];
    }

    for (int m=0; m < 10; m++) yy[m]/=sum_exp;

    double max_prob=0.0;
    current_prediction=0;
    for (int k=0; k < 10; k++){
        if (yy[k] > max_prob){
            max_prob=yy[k];
            current_prediction=k;
        }
    }
    //TCHAR buffer[100];
    //// Преобразуем число в строку
    //_stprintf(buffer, _T("%d"), current_prediction);
    //// Выводим MessageBox с числом
    //MessageBox(NULL, buffer, _T("Успех"), MB_OK);
}








// 3. Сфотографированные
//
// Функция для полной очистки папки, в которую сохраняем обработанные изображения
void clear_directory_3(const TCHAR* path) {
    TCHAR search_path[MAX_PATH];
    _stprintf_s(search_path, MAX_PATH, _T("%s\\*"), path);

    WIN32_FIND_DATA find_data;
    HANDLE hFind=FindFirstFile(search_path, &find_data);

    if (hFind == INVALID_HANDLE_VALUE){
        _tprintf(_T("Папка %s не существует или пуста\n"), path);
        return;
    }

    do{
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
            TCHAR file_path[MAX_PATH];
            _stprintf_s(file_path, MAX_PATH, _T("%s\\%s"), path, find_data.cFileName);

            if (!DeleteFile(file_path)){
                _tprintf(_T("Не удалось удалить файл: %s\n"), file_path);
            }
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
    _tprintf(_T("Папка %s очищена\n"), path);
}

// Функция копирования изображений
void copy_file_3(const TCHAR* src, const TCHAR* dest) {
    TCHAR cmd[1024];
    _stprintf_s(cmd, sizeof(cmd)/sizeof(TCHAR), _T("copy \"%s\" \"%s\""), src, dest);
    _tsystem(cmd);
}

// 1. Стадия обработки изображений
void process_images_3() {
    const TCHAR* src_dir=_T("Обученные файлы\\3. Сфотографированные\\1. Не обработанные");
    const TCHAR* processed_dir=_T("Обученные файлы\\3. Сфотографированные\\2. Обработанные");

    // clear_directory_3(processed_dir); // В данном случае ничего не очищаем, так как датасет уже загружен
    // CreateDirectory(processed_dir, NULL);

    TCHAR search_path[MAX_PATH];
    _stprintf_s(search_path, MAX_PATH, _T("%s\\*.png"), src_dir);

    WIN32_FIND_DATA find_data;
    HANDLE hFind=FindFirstFile(search_path, &find_data);

    if (hFind == INVALID_HANDLE_VALUE){
        _tprintf(_T("Не найдено PNG-файлов в папке 'не обработанные'\n"));
        return;
    }

    do{
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
            TCHAR filename[MAX_PATH];
            _tcscpy_s(filename, MAX_PATH, find_data.cFileName);

            // Копируем оригинал в "обработанные"
            TCHAR src_file[MAX_PATH];
            _stprintf_s(src_file, MAX_PATH, _T("%s\\%s"), src_dir, filename);
            TCHAR dest_file[MAX_PATH];
            _stprintf_s(dest_file, MAX_PATH, _T("%s\\%s"), processed_dir, filename);
            copy_file_3(src_file, dest_file);

            // Извлекаем цифры из имени файла
            int digit, number;
            if (_stscanf_s(filename, _T("%d_%d.png"), &digit, &number) != 2){
                _tprintf(_T("Некорректное имя файла: %s\n"), filename);
                continue;
            }

            if (digit == 9){
                // 1. Серая версия (добавляем 1 к номеру)
                TCHAR gray_filename[MAX_PATH];
                _stprintf_s(gray_filename, MAX_PATH, _T("%d_%06d.png"), digit, number + 1);

                TCHAR cmd[1024];
                _stprintf_s(cmd, 1024,
                    _T("magick convert \"%s\\%s\" -colorspace gray \"%s\\%s\""),
                    processed_dir, filename, processed_dir, gray_filename);
                _tsystem(cmd);

                // 2. Яркая версия (добавляем 2 к номеру)
                TCHAR bright_filename[MAX_PATH];
                _stprintf_s(bright_filename, MAX_PATH, _T("%d_%06d.png"), digit, number + 2);

                _stprintf_s(cmd, 1024,
                    _T("magick convert \"%s\\%s\" -colorspace gray -brightness-contrast 60x0 \"%s\\%s\""),
                    processed_dir, filename, processed_dir, bright_filename);
                _tsystem(cmd);

                // 3. Повышение резкости (5 итераций)
                for (int i=3; i <= 12; i++){
                    TCHAR prev_filename[MAX_PATH];
                    _stprintf_s(prev_filename, MAX_PATH, _T("%d_%06d.png"), digit, number + i - 1);

                    TCHAR new_filename[MAX_PATH];
                    _stprintf_s(new_filename, MAX_PATH, _T("%d_%06d.png"), digit, number + i);

                    _stprintf_s(cmd, 1024,
                        _T("magick convert \"%s\\%s\" -sharpen 0x10 \"%s\\%s\""),
                        processed_dir, prev_filename, processed_dir, new_filename);
                    _tsystem(cmd);
                }
            }
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
    MessageBox(NULL, _T("Обработка изображений завершена! Результаты в папке '2. Обработанные"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
}

// Приведение изображения к размеру 28*28 и выравнивание по центру
void scale_images_3() {
    const TCHAR* processed_dir=_T("Обученные файлы\\3. Сфотографированные\\2. Обработанные");
    const TCHAR* scaled_dir=_T("Обученные файлы\\3. Сфотографированные\\3. Масштабированные");

    // clear_directory_3(scaled_dir); // В данном случае ничего не очищаем, так как всё уже загружено
    // CreateDirectory(scaled_dir, NULL);

    TCHAR search_path[MAX_PATH];
    _stprintf_s(search_path, MAX_PATH, _T("%s\\*.png"), processed_dir);

    WIN32_FIND_DATA find_data;
    HANDLE hFind=FindFirstFile(search_path, &find_data);

    if (hFind == INVALID_HANDLE_VALUE){
        _tprintf(_T("Не найдено PNG-файлов в папке '2. Обработанные'\n"));
        return;
    }

    do{
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
            TCHAR filename[MAX_PATH];
            _tcscpy_s(filename, MAX_PATH, find_data.cFileName);

            int digit, number;
            if (_stscanf_s(filename, _T("%d_%d.png"), &digit, &number) != 2){
                _tprintf(_T("Некорректное имя файла: %s\n"), filename);
                continue;
            }

            if (digit == 9){
                TCHAR base_name[MAX_PATH];
                _tcscpy_s(base_name, MAX_PATH, filename);
                TCHAR* dot=_tcsrchr(base_name, _T('.'));
                if (dot) *dot=_T('\0');

                // Уменшение изображений и сохранение их в папку "3. Масштабированные"
                TCHAR cmd[1024];
                _stprintf_s(cmd, 1024,
                    _T("magick convert \"%s\\%s\" ")
                    _T("-threshold 50%% ")
                    _T("-trim +repage ")
                    _T("-resize 20x20 ")
                    _T("-background white ")
                    _T("-gravity center ")
                    _T("-extent 28x28 ")
                    _T("\"%s\\%s\""),
                    processed_dir, filename, scaled_dir, filename);
                _tsystem(cmd);
            }
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
    MessageBox(NULL, _T("Масштабирование завершено! Результаты в папке '3. Масштабированные'"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
}

// 2. Стадия обработки изображений
void process_final_sharpening_3() {
    const TCHAR* scaled_dir=_T("Обученные файлы\\3. Сфотографированные\\3. Масштабированные");
    const TCHAR* ready_dir=_T("Обученные файлы\\3. Сфотографированные\\4. Готовые");

    /* clear_directory(ready_dir); */

    // Для каждой цифры от 0 до 9
    for (int i=0; i < 10; i++){
        // Обрабатываем 8 изображений (0-7)
        for (int j=0; j <= 12; j++){
            TCHAR src_filename[MAX_PATH];
            _stprintf_s(src_filename, MAX_PATH, _T("%s\\%d_%06d.png"), scaled_dir, i, j);

            TCHAR base_name[MAX_PATH];
            _stprintf_s(base_name, MAX_PATH, _T("%d_%06d"), i, j);

            // Проверяем существует ли исходный файл
            WIN32_FIND_DATA find_data;
            HANDLE hFind=FindFirstFile(src_filename, &find_data);

            if (hFind == INVALID_HANDLE_VALUE){
                _tprintf(_T("Файл не найден: %s\n"), src_filename);
                continue;
            }
            FindClose(hFind);

            TCHAR final_file[MAX_PATH];
            _stprintf_s(final_file, MAX_PATH, _T("%s\\%s.png"), ready_dir, base_name);

            // 2 первых изображения просто копируем
            if (j <= 1){
                TCHAR cmd[1024];
                _stprintf_s(cmd, 1024, _T("magick convert \"%s\" \"%s\""), src_filename, final_file);
                _tsystem(cmd);
                continue;
            }

            // Для изображений 2-7 делаем повышение резкости
            TCHAR temp_file[MAX_PATH];
            _stprintf_s(temp_file, MAX_PATH, _T("%s\\temp_%s.png"), ready_dir, base_name);

            TCHAR cmd[1024];
            // Создаем временную копию в той же папке
            _stprintf_s(cmd, 1024, _T("magick convert \"%s\" \"%s\""), src_filename, temp_file);
            _tsystem(cmd);

            // 5 циклов повышения резкости (работаем с одним временным файлом)
            for (int sharp_iter=1; sharp_iter <= 5; sharp_iter++){
                TCHAR new_temp[MAX_PATH];
                _stprintf_s(new_temp, MAX_PATH, _T("%s\\temp%d_%s.png"), ready_dir, sharp_iter, base_name);

                _stprintf_s(cmd, 1024,
                    _T("magick convert \"%s\" -sharpen 0x10 \"%s\""),
                    temp_file, new_temp);
                _tsystem(cmd);

                // Удаляем предыдущий временный файл
                DeleteFile(temp_file);

                // Обновляем имя временного файла для следующей итерации
                _tcscpy_s(temp_file, MAX_PATH, new_temp);
            }

            // Перемещаем финальный результат (переименовываем)
            _stprintf_s(cmd, 1024, _T("move /Y \"%s\" \"%s\""), temp_file, final_file);
            _tsystem(cmd);
        }
    }
    MessageBox(NULL, _T("Финальная обработка завершена! Результаты в папке '4. Готовые'"), _T("Успех"), MB_OK | MB_ICONINFORMATION);

    // Дополнительная очистка (на случай если что-то осталось)
    WIN32_FIND_DATA find_temp;
    TCHAR temp_search[MAX_PATH];
    _stprintf_s(temp_search, MAX_PATH, _T("%s\\temp*.png"), ready_dir);

    HANDLE hTempFind=FindFirstFile(temp_search, &find_temp);
    if (hTempFind != INVALID_HANDLE_VALUE){
        do{
            TCHAR temp_file_path[MAX_PATH];
            _stprintf_s(temp_file_path, MAX_PATH, _T("%s\\%s"), ready_dir, find_temp.cFileName);
            DeleteFile(temp_file_path);
        } while (FindNextFile(hTempFind, &find_temp) != 0);
        FindClose(hTempFind);
    }
}

// Функция конвертации PNG в PGM через командную строку при помощи программы ImageMagic 
void konvertation_3() {
    const TCHAR* png_dir=_T("Обученные файлы\\3. Сфотографированные\\4. Готовые");
    const TCHAR* pgm_dir=_T("Обученные файлы\\3. Сфотографированные\\5. Тест автоматом");

    // 1. Очистка папки (удаляем все .pgm файлы из папки "5. Тест автоматом")
    TCHAR del_cmd[512];
    _stprintf_s(del_cmd, _T("del /Q \"%s\\*.pgm\""), pgm_dir);
    _tsystem(del_cmd);

    // 2. Конвертация PNG в PGM
    TCHAR convert_cmd[512];
    _stprintf_s(convert_cmd, _T("magick mogrify -path \"%s\" -format pgm \"%s\\*.png\""), pgm_dir, png_dir);

    int result=_tsystem(convert_cmd);

    if (result == 0){
        MessageBox(NULL, _T("Конвертация успешно завершена!"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
    }
    else{
        TCHAR error_msg[256];
        _stprintf_s(error_msg, _T("Ошибка конвертации (код: %d)"), result);
        MessageBox(NULL, error_msg, _T("Ошибка"), MB_OK | MB_ICONERROR);
    }
}






// Функция преобразования пикселей изображения в формат [0.0; 1.0]
int bin_data_lf_3() {
    TCHAR filename[MAX_PATH];
    FILE* fp_wb, * file;

    // Открываем бинарный файл для записи
    fp_wb=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Тестовая выборка.bin"), _T("wb"));
    if (!fp_wb){
        MessageBox(NULL, _T("Не удалось открыть файл для записи!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Формируем имя файла и проверяем, существует ли он
    for (int i=0; i < 10; i++){
        for (int j=12; j < 20; j++){ // Берём только изображение под номером 12, так как оно самое яркое (экономим время)
            _stprintf_s(filename, _T("Обученные файлы\\3. Сфотографированные\\5. Тест автоматом\\%d_00%04d.pgm"), i, j);

            file=_tfopen(filename, _T("rb"));
            if (!file){
                //// Формируем сообщение об ошибке с конкретным путем файла
                //TCHAR errorMessage[256];
                //_stprintf_s(errorMessage, 256, _T("Файл не найден: %s"), filename);

                //MessageBox(NULL, errorMessage, _T("Ошибка"), MB_OK | MB_ICONERROR);
                break;
            }

            int width, height, maxval;
            unsigned char* image_data;
            size_t image_size;

            // Считываем заголовок PGM
            if (fscanf_s(file, "P5\n%d %d\n%d\n", &width, &height, &maxval) != 3){
                MessageBox(NULL, _T("Ошибка чтения заголовка PGM"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                fclose(file);
                fclose(fp_wb);
                return 2;
            }

            // Выделяем память под изображение
            image_size=(size_t) width * (size_t) height;
            image_data=(unsigned char*) malloc(image_size * sizeof(unsigned char));
            if (!image_data){
                MessageBox(NULL, _T("Ошибка выделения памяти"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                fclose(file);
                fclose(fp_wb);
                return 2;
            }

            // Чтение данных изображения
            if (fread(image_data, sizeof(unsigned char), image_size, file) != image_size){
                TCHAR errorMsg[512];
                _stprintf_s(errorMsg, _T("Ошибка чтения данных изображения %s, метка %d"), filename, i);
                MessageBox(NULL, errorMsg, _T("Ошибка"), MB_OK | MB_ICONWARNING);
                free(image_data);
                continue;
            }

            // Запись метки и размеров в бинарный файл
            fwrite(&i, sizeof(int), 1, fp_wb);
            fwrite(&height, sizeof(int), 1, fp_wb);
            fwrite(&width, sizeof(int), 1, fp_wb);

            // Преобразование в черно-белое
            double* binary_image=(double*) malloc(image_size * sizeof(double));
            if (!binary_image){
                MessageBox(NULL, _T("Ошибка выделения памяти для черно-белого изображения"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                free(image_data);
                fclose(file);
                fclose(fp_wb);
                return 1;
            }

            for (int k=0; k < image_size; k++){
                binary_image[k]=((double) image_data[k]) / 255.0;
            }

            // Запись бинарных данных
            fwrite(binary_image, sizeof(double), image_size, fp_wb);
            fclose(file);
            fflush(fp_wb);

            // Вывод информации о прогрессе
            TCHAR progressMsg[512];
            _stprintf_s(progressMsg, _T("Загружено изображение %s, метка %d"), filename, i);
            OutputDebugString(progressMsg);
            // MessageBox(NULL, progressMsg, _T("Информация"), MB_OK | MB_ICONINFORMATION); // Раскомментировать для показа каждого шага

            free(image_data);
            free(binary_image);
        }
    }

    fclose(fp_wb);
    MessageBox(NULL, _T("Загрузка в бинарный файл завершена успешно!"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
    return 0;
}

// Подсчёт сумм изображений для каждой метки
int sum_images_test_3(int sum[10]) {
    FILE* fp_wb;
    int label, height, width, new_label=0, out=0, elements_read;
    double* image_data=(double*) malloc(28 * 28 * sizeof(double));

    fp_wb=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Тестовая выборка.bin"), _T("rb"));
    if (!fp_wb){
        MessageBox(NULL, _T("Не удалось открыть файл тестовой выборки!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        free(image_data);
        return 0;
    }

    for (int i=0; i < 10; i++){
        for (int j=0; j < 7000; j++){
            if (fread(&label, sizeof(int), 1, fp_wb) != 1 ||
                fread(&height, sizeof(int), 1, fp_wb) != 1 ||
                fread(&width, sizeof(int), 1, fp_wb) != 1){
                out=1;
                break;
            }

            elements_read=fread(image_data, sizeof(double), 28 * 28, fp_wb);
            if (elements_read != 28 * 28){
                out=1;
                break;
            }

            if (new_label == label){
                sum[i]++;
            }
            else{
                new_label=label;
                sum[i + 1]++;
                break;
            }
        }
        if (out == 1){ break; }
    }

    free(image_data);
    fclose(fp_wb);
    return *sum;
}

// Функция фильтрации
void apply_filter_to_all_test_3() {
    int sum_image[10]={0};
    int label, height, width, image_size;

    sum_images_test_3(sum_image);

    // Вывод информации о количестве изображений
    TCHAR sum_msg[256];
    for (int i=0; i < 10; i++){
        _stprintf_s(sum_msg, _T("%d. sum = %d\n"), i, sum_image[i]);
        OutputDebugString(sum_msg);
    }

    FILE* fp_wb, * filterf;
    fp_wb=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Тестовая выборка.bin"), _T("rb"));
    filterf=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Фильтр Собеля тест.bin"), _T("wb"));

    if (!fp_wb || !filterf){
        MessageBox(NULL, _T("Не удалось открыть файлы для чтения/записи!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        if (fp_wb) fclose(fp_wb);
        if (filterf) fclose(filterf);
        return;
    }

    for (int i=0; i < 10; i++){
        for (int j=0; j < sum_image[i]; j++){
            if (fread(&label, sizeof(int), 1, fp_wb) != 1 ||
                fread(&height, sizeof(int), 1, fp_wb) != 1 ||
                fread(&width, sizeof(int), 1, fp_wb) != 1){
                MessageBox(NULL, _T("Ошибка чтения данных изображения"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                break;
            }

            image_size=width * height;
            double* image_data=(double*) malloc(image_size * sizeof(double));
            if (!image_data){
                MessageBox(NULL, _T("Ошибка выделения памяти"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                break;
            }

            if (fread(image_data, sizeof(double), image_size, fp_wb) != image_size){
                MessageBox(NULL, _T("Ошибка чтения данных изображения"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                free(image_data);
                continue;
            }

            // Фильтр Собеля
            int filter1[9]={-1, -1, -1, 0, 0, 0, 1, 1, 1};
            int filter2[9]={-1, 0, 1, -1, 0, 1, -1, 0, 1};

            double padded[30 * 30]={0.0};
            for (int y=0; y < 28; y++){
                for (int x=0; x < 28; x++){
                    padded[(y + 1) * 30 + (x + 1)]=image_data[y * 28 + x];
                }
            }

            double filtered1[28 * 28]={0.0};
            double filtered2[28 * 28]={0.0};
            double combined[28 * 28]={0.0};

            // Применение фильтра
            for (int y=0; y < 28; y++){
                for (int x=0; x < 28; x++){
                    double sum1=0.0, sum2=0.0;
                    for (int ky=0; ky < 3; ky++){
                        for (int kx=0; kx < 3; kx++){
                            sum1+=padded[(y + ky) * 30 + (x + kx)] * filter1[ky * 3 + kx];
                            sum2+=padded[(y + ky) * 30 + (x + kx)] * filter2[ky * 3 + kx];
                        }
                    }
                    filtered1[y * 28 + x]=sum1;
                    filtered2[y * 28 + x]=sum2;
                    combined[y * 28 + x]=sqrt(sum1 * sum1 + sum2 * sum2);
                }
            }

            // Обработка границ
            for (int k=0; k < 28 * 28; k++){
                if (k < 28 || k > 755 || k % 28 == 0 || k % 28 == 27){
                    combined[k]=0.0;
                }
            }

            // Нормализация
            double max_val=0.0;
            for (int k=0; k < 28 * 28; k++){
                combined[k]=1.0 - combined[k];
                if (combined[k] > max_val) max_val=combined[k];
            }

            if (max_val > 0){
                for (int k=0; k < 28 * 28; k++){
                    combined[k]/=max_val;
                }
            }

            // Запись результата
            fwrite(&label, sizeof(int), 1, filterf);
            int hw=28;
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(combined, sizeof(double), 28 * 28, filterf);

            free(image_data);

            if (j % 2000 == 0){
                TCHAR progress_msg[256];
                _stprintf_s(progress_msg, _T("Загружено фильтрованное изображение: i= %d, j= %d"), i, j);
                OutputDebugString(progress_msg);
            }
        }
    }

    //// Визуализация результатов
    //double* image_data_s=(double*) malloc(28 * 28 * sizeof(double));
    //if (image_data_s){
    //    _tfreopen(_T("C:\\Users\\Пользователь\\Desktop\\Цифры\\Обученные\\5. Распознавание Paint\\1. Фильтр Собеля тест.bin"), _T("rb"), filterf);
    //    for (int i=0; i < 10; i++){
    //        for (int j=0; j < sum_image[i]; j++){
    //            if (fread(&label, sizeof(int), 1, filterf) != 1 ||
    //                fread(&height, sizeof(int), 1, filterf) != 1 ||
    //                fread(&width, sizeof(int), 1, filterf) != 1 ||
    //                fread(image_data_s, sizeof(double), 28 * 28, filterf) != 28 * 28){
    //                break;
    //            }
    //            if (j % 700 == 0 && j != 0){
    //                printer(label, 28, 28, image_data_s);
    //            }
    //        }
    //    }
    //    free(image_data_s);
    //}

    fclose(fp_wb);
    fclose(filterf);
    MessageBox(NULL, _T("Фильтрация изображений завершена успешно!"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
}

// Функция пуллинга
void pooling_test_3() {
    int sum[10]={0};
    sum_images_test_3(sum);

    FILE* pooling_f_rb, * pooling_f_wb;
    pooling_f_rb=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Фильтр Собеля тест.bin"), _T("rb"));
    pooling_f_wb=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Отпуллингованная тестовая выборка.bin"), _T("wb"));

    if (!pooling_f_rb || !pooling_f_wb){
        MessageBox(NULL, _T("Не удалось открыть файлы для операции pooling!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        if (pooling_f_rb) fclose(pooling_f_rb);
        if (pooling_f_wb) fclose(pooling_f_wb);
        return;
    }

    int label, height, weight, s=0, hw;
    double* image=(double*) malloc(28 * 28 * sizeof(double));
    double max[14 * 14]={0.0};

    if (!image){
        MessageBox(NULL, _T("Ошибка выделения памяти для изображения!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        fclose(pooling_f_rb);
        fclose(pooling_f_wb);
        return;
    }

    // Записываем информацию о количестве изображений
    fwrite(sum, sizeof(int), 10, pooling_f_wb);

    // Выводим информацию о количестве изображений
    for (int i=0; i < 10; i++){
        TCHAR debugMsg[256];
        _stprintf_s(debugMsg, _T("%d. Количество изображений: %d\n"), i, sum[i]);
        OutputDebugString(debugMsg);
    }

    // Обрабатываем каждое изображение
    for (int i=0; i < 10; i++){
        for (int j=0; j < sum[i]; j++){
            s=0;

            // Читаем данные изображения
            if (fread(&label, sizeof(int), 1, pooling_f_rb) != 1 ||
                fread(&height, sizeof(int), 1, pooling_f_rb) != 1 ||
                fread(&weight, sizeof(int), 1, pooling_f_rb) != 1 ||
                fread(image, sizeof(double), 28 * 28, pooling_f_rb) != 28 * 28){
                MessageBox(NULL, _T("Ошибка чтения данных изображения!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                break;
            }

            // Применяем pooling 2x2 - пропускаем по пикселям изображения матрицу размером 2*2 с шагом == 2 и сохраняем самый яркий пиксель.
            // Пересечений между шагами нет.
            for (int y=0; y<28; y+=2){
                for (int x=0; x<28; x+=2){
                    max[s]=2.0;
                    for (int ky=0; ky<2; ky++){
                        for (int kx=0; kx<2; kx++){
                            if (image[(y+ky) * 28 + (x+kx)]<max[s]){ max[s]=image[(y+ky) * 28 + (x+kx)]; }
                        }
                    }
                    s++;
                }
            }

            // Записываем результат
            hw=14;
            fwrite(&label, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(max, sizeof(double), s, pooling_f_wb);

            // Вывод прогресса (каждые 100 изображений)
            if (j % 100 == 0){
                TCHAR progressMsg[256];
                _stprintf_s(progressMsg, _T("Обработано изображений: цифра %d - %d/%d"), i, j, sum[i]);
                OutputDebugString(progressMsg);
            }
        }
    }

    // Освобождаем ресурсы
    free(image);
    fclose(pooling_f_rb);
    fclose(pooling_f_wb);

    MessageBox(NULL, _T("Операция pooling успешно завершена!"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
}




// Распознавание и отрисовка
// Открываем файлы
void LoadCNNData_3() {
    // Загрузка весов и смещений
    FILE* W_b=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Матрица весов и вектор коэффициентов смещения.bin"), _T("rb"));
    if (W_b){
        fread(W, sizeof(double), 196 * 10, W_b);
        fread(b, sizeof(double), 10, W_b);
        fclose(W_b);
    }

    // Открываем файлы с изображениями
    original_f=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Тестовая выборка.bin"), _T("rb"));
    pooling_f_wb=_tfopen(_T("Обученные файлы\\3. Сфотографированные\\1. Отпуллингованная тестовая выборка.bin"), _T("rb"));

    if (pooling_f_wb){
        fread(sum, sizeof(int), 10, pooling_f_wb);
        total_images=0;
        for (int i=0; i < 10; i++){
            total_images+=sum[i];
        }
    }
}

// Функция приведения изображения к виду, готовому к распознаванию
void testing_3(int offset) {
    if (!pooling_f_wb || !original_f) return;

    // Перемещаемся в начало файлов
    fseek(pooling_f_wb, sizeof(int) * 10, SEEK_SET);
    fseek(original_f, 0, SEEK_SET);

    // Пропускаем указанное количество изображений
    for (int i=0; i < offset; i++){
        // Пропускаем в оригинальном файле
        fseek(original_f, sizeof(int) + sizeof(int) + sizeof(int) + 784 * sizeof(double), SEEK_CUR);

        // Пропускаем в пуллингованном файле
        fseek(pooling_f_wb, sizeof(int) + sizeof(int) + sizeof(int) + 196 * sizeof(double), SEEK_CUR);
    }

    // Читаем текущее изображение из пуллингованного файла
    if (fread(&current_label, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(&height_original, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(&width_original, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(x, sizeof(double), 196, pooling_f_wb) == 196){

        // Читаем соответствующее оригинальное изображение
        fread(&label_original, sizeof(int), 1, original_f);
        fread(&height_original, sizeof(int), 1, original_f);
        fread(&width_original, sizeof(int), 1, original_f);
        fread(pixels_original, sizeof(double), 784, original_f);

        // Распознаем изображение
        double z[10]={0};
        for (int m=0; m < 10; m++){
            for (int n=0; n < 196; n++){
                z[m]+=x[n] * W[n][m];
            }
            z[m]+=b[m];
        }

        double max=z[0];
        for (int m=1; m < 10; m++) if (z[m] > max) max=z[m];

        double yy[10]={0};
        double sum_exp=0.0;
        for (int m=0; m < 10; m++){
            yy[m]=exp(z[m] - max);
            sum_exp+=yy[m];
        }

        for (int m=0; m < 10; m++) yy[m]/=sum_exp;

        double max_prob=0.0;
        current_prediction=0;
        for (int k=0; k < 10; k++){
            if (yy[k] > max_prob){
                max_prob=yy[k];
                current_prediction=k;
            }
        }
    }
    else{
        current_label=-1;
        current_prediction=-1;
    }
}

// Функция отрисовки изображения в виде пикселей в левом окне
void DrawDigit_3(HDC hdc, int x_pos, int y_pos, int cellSize) {
    HBRUSH hBrush=CreateSolidBrush(RGB(0, 255, 0)); // Зеленый цвет
    HPEN hPen=CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // Красный цвет обводки

    HBRUSH hOldBrush=(HBRUSH) SelectObject(hdc, hBrush);
    HPEN hOldPen=(HPEN) SelectObject(hdc, hPen);

    for (int row=0; row < height_original; row++){
        for (int col=0; col < width_original; col++){
            // Почему здесь цифры высвечиваются только при == 0.0 ?
            // Потому что при обработке изображение приведено в формат строгой бинаризации
            // 0.0 - пиксель цифры, 1.0 - фон
            // По идее, так делать нельзя, так как наша модель обучена на изображениях, пиксели которых варьируются в диапазоне [0.0; 1.0],
            // но другого способа подать на вход сфотографированное изображение я не нашёл
            // По идее, пиксели жолжны быть в диапазоне [0.0; 1.0]
            if (pixels_original[row * width_original + col] == 0.0){
                int left=x_pos + col * cellSize;
                int top=y_pos + row * cellSize;
                Rectangle(hdc, left, top, left + cellSize, top + cellSize);
            }
        }
    }

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}









// 1. Распознаём mnist 
//
// Эти изображения уже отфильтрованы и отпуллингованы, поэтому функции фильтрации и пуллинга отсутствуют
// Функция открытия файлов
void LoadCNNData_1() {
    // Загрузка весов и смещений
    FILE* W_b=_tfopen(_T("Обученные файлы\\1. Распознаём mnist\\1. Матрица весов и вектор коэффициентов смещения.bin"), _T("rb"));
    if (W_b){
        fread(W, sizeof(double), 196 * 10, W_b);
        fread(b, sizeof(double), 10, W_b);
        fclose(W_b);
    }

    // Открываем файлы с изображениями
    original_f=_tfopen(_T("Обученные файлы\\1. Распознаём mnist\\1. Тестовая выборка.bin"), _T("rb"));
    pooling_f_wb=_tfopen(_T("Обученные файлы\\1. Распознаём mnist\\1. Отпуллингованная тестовая выборка.bin"), _T("rb"));

    if (pooling_f_wb){
        fread(sum, sizeof(int), 10, pooling_f_wb);
        total_images=0;
        for (int i=0; i < 10; i++){
            total_images+=sum[i];
        }
    }
}

// Функция приведения изображения к виду, готовому к распознаванию
void testing_1(int offset) {
    if (!pooling_f_wb || !original_f) return;

    // Перемещаемся в начало файлов
    fseek(pooling_f_wb, sizeof(int) * 10, SEEK_SET);
    fseek(original_f, 0, SEEK_SET);

    // Пропускаем указанное количество изображений
    for (int i=0; i < offset; i++){
        // Пропускаем в оригинальном файле
        fseek(original_f, sizeof(int) + sizeof(int) + sizeof(int) + 784 * sizeof(double), SEEK_CUR);

        // Пропускаем в пуллингованном файле
        fseek(pooling_f_wb, sizeof(int) + sizeof(int) + sizeof(int) + 196 * sizeof(double), SEEK_CUR);
    }

    // Читаем текущее изображение из пуллингованного файла
    if (fread(&current_label, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(&height_original, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(&width_original, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(x, sizeof(double), 196, pooling_f_wb) == 196){

        // Читаем соответствующее оригинальное изображение
        fread(&label_original, sizeof(int), 1, original_f);
        fread(&height_original, sizeof(int), 1, original_f);
        fread(&width_original, sizeof(int), 1, original_f);
        fread(pixels_original, sizeof(double), 784, original_f);

        // Распознаем изображение
        double z[10]={0};
        for (int m=0; m < 10; m++){
            for (int n=0; n < 196; n++){
                z[m]+=x[n] * W[n][m];
            }
            z[m]+=b[m];
        }

        double max=z[0];
        for (int m=1; m < 10; m++) if (z[m] > max) max=z[m];

        double yy[10]={0};
        double sum_exp=0.0;
        for (int m=0; m < 10; m++){
            yy[m]=exp(z[m] - max);
            sum_exp+=yy[m];
        }

        for (int m=0; m < 10; m++) yy[m]/=sum_exp;

        double max_prob=0.0;
        current_prediction=0;
        for (int k=0; k < 10; k++){
            if (yy[k] > max_prob){
                max_prob=yy[k];
                current_prediction=k;
            }
        }
    }
    else{
        current_label=-1;
        current_prediction=-1;
    }
}

// Функция отрисовки изображения в виде пикселей в левом окне
void DrawDigit_1(HDC hdc, int x_pos, int y_pos, int cellSize) {
    HBRUSH hBrush=CreateSolidBrush(RGB(0, 255, 0)); // Зеленый цвет
    HPEN hPen=CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // Красный цвет обводки

    HBRUSH hOldBrush=(HBRUSH) SelectObject(hdc, hBrush);
    HPEN hOldPen=(HPEN) SelectObject(hdc, hPen);

    for (int row=0; row < height_original; row++){
        for (int col=0; col < width_original; col++){
            // Почемму здесь высвечивается 1 только при < 0.001 ?
            if (pixels_original[row * width_original + col] < 0.5){
                int left=x_pos + col * cellSize;
                int top=y_pos + row * cellSize;
                Rectangle(hdc, left, top, left + cellSize, top + cellSize);
            }
        }
    }

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}






// 2. Paint
//
// Функция конвертации PNG в PGM через командную строку при помощи программы ImageMagic 
void konvertation_2() {
    const TCHAR* png_dir=_T("Обученные файлы\\2. Paint\\Paint");
    const TCHAR* pgm_dir=_T("Обученные файлы\\2. Paint\\Paint Конвертированные");

    // 1. Очистка папки (удаляем все .pgm файлы), чтобы создать новые
    TCHAR del_cmd[512];
    _stprintf_s(del_cmd, _T("del /Q \"%s\\*.pgm\""), pgm_dir);
    _tsystem(del_cmd);

    // 2. Конвертация PNG в PGM (системный вызов при помощи командной строки)
    TCHAR convert_cmd[512];
    _stprintf_s(convert_cmd, _T("magick mogrify -path \"%s\" -format pgm \"%s\\*.png\""), pgm_dir, png_dir);

    int result=_tsystem(convert_cmd);

    if (result == 0){ MessageBox(NULL, _T("Конвертация успешно завершена!"), _T("Успех"), MB_OK | MB_ICONINFORMATION); }
    else{
        TCHAR error_msg[256];
        _stprintf_s(error_msg, _T("Ошибка конвертации (код: %d)"), result);
        MessageBox(NULL, error_msg, _T("Ошибка"), MB_OK | MB_ICONERROR);
    }
}

// Функция преобразования пикселей изображения в формат [0.0; 1.0], но получается всё равно только 0.0 и 1.0.
// Таким изображение вышло после обработки - строго черно-белым.
int bin_data_lf_2() {
    TCHAR filename[MAX_PATH];
    FILE* fp_wb, * file;

    // Открываем бинарный файл для записи
    fp_wb=_tfopen(_T("Обученные файлы\\2. Paint\\1. Тестовая выборка.bin"), _T("wb"));
    if (!fp_wb){
        MessageBox(NULL, _T("Не удалось открыть файл для записи!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        return 1;
    }

    // Формируем имя файла и проверяем, существует ли он
    for (int i=0; i < 10; i++){
        for (int j=0; j < 20; j++){
            _stprintf_s(filename, _T("Обученные файлы\\2. Paint\\Paint Конвертированные\\%d_00%04d.pgm"), i, j);

            file=_tfopen(filename, _T("rb"));
            if (!file){ break; }

            int width, height, maxval;
            unsigned char* image_data;
            size_t image_size;

            // Считываем заголовок PGM
            if (fscanf_s(file, "P5\n%d %d\n%d\n", &width, &height, &maxval) != 3){
                MessageBox(NULL, _T("Ошибка чтения заголовка PGM"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                fclose(file);
                fclose(fp_wb);
                return 2;
            }

            // Выделяем память под изображение
            image_size=(size_t) width * (size_t) height;
            image_data=(unsigned char*) malloc(image_size * sizeof(unsigned char));
            if (!image_data){
                MessageBox(NULL, _T("Ошибка выделения памяти"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                fclose(file);
                fclose(fp_wb);
                return 2;
            }

            // Чтение данных изображения
            if (fread(image_data, sizeof(unsigned char), image_size, file) != image_size){
                TCHAR errorMsg[512];
                _stprintf_s(errorMsg, _T("Ошибка чтения данных изображения %s, метка %d"), filename, i);
                MessageBox(NULL, errorMsg, _T("Ошибка"), MB_OK | MB_ICONWARNING);
                free(image_data);
                continue;
            }

            // Запись метки и размеров в бинарный файл
            fwrite(&i, sizeof(int), 1, fp_wb);
            fwrite(&height, sizeof(int), 1, fp_wb);
            fwrite(&width, sizeof(int), 1, fp_wb);

            // Преобразование в черно-белое
            double* binary_image=(double*) malloc(image_size * sizeof(double));
            if (!binary_image){
                MessageBox(NULL, _T("Ошибка выделения памяти для черно-белого изображения"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                free(image_data);
                fclose(file);
                fclose(fp_wb);
                return 1;
            }

            // Реверсивное преобразование в диапазон [0.0; 1.0] (именно так была обучена наша модель, это моя логическая неточность при обучении).
            // Таким образом мы в цифровом формате перевели цвета из "белым по черному" в "черным по белому"
            // Таким же образом были преобразованы в цифровой формат обучающие изображения
            // Можно было сделать просто binary_image[k]=((double) image_data[k]) / 255.0;
            for (int k=0; k < image_size; k++){
                binary_image[k]=(255 - (double) image_data[k]) / 255.0;
            }

            // Запись бинарных данных
            fwrite(binary_image, sizeof(double), image_size, fp_wb);
            fclose(file);
            fflush(fp_wb);

            // Вывод информации о прогрессе
            TCHAR progressMsg[512];
            _stprintf_s(progressMsg, _T("Загружено изображение %s, метка %d"), filename, i);
            OutputDebugString(progressMsg);
            // MessageBox(NULL, progressMsg, _T("Информация"), MB_OK | MB_ICONINFORMATION); // Раскомментировать для показа каждого шага

            free(image_data);
            free(binary_image);
        }
    }

    fclose(fp_wb);
    MessageBox(NULL, _T("Загрузка в бинарный файл завершена успешно!"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
    return 0;
}

// Подсчёт сумм изображений для каждой метки
int sum_images_test_2(int sum[10]) {
    FILE* fp_wb;
    int label, height, width, new_label=0, out=0, elements_read;
    double* image_data=(double*) malloc(28 * 28 * sizeof(double));

    fp_wb=_tfopen(_T("Обученные файлы\\2. Paint\\1. Тестовая выборка.bin"), _T("rb"));
    if (!fp_wb){
        MessageBox(NULL, _T("Не удалось открыть файл тестовой выборки!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        free(image_data);
        return 0;
    }

    for (int i=0; i < 10; i++){
        for (int j=0; j < 7000; j++){
            if (fread(&label, sizeof(int), 1, fp_wb) != 1 ||
                fread(&height, sizeof(int), 1, fp_wb) != 1 ||
                fread(&width, sizeof(int), 1, fp_wb) != 1){
                out=1;
                break;
            }

            elements_read=fread(image_data, sizeof(double), 28 * 28, fp_wb);
            if (elements_read != 28 * 28){
                out=1;
                break;
            }

            if (new_label == label){
                sum[i]++;
            }
            else{
                new_label=label;
                sum[i + 1]++;
                break;
            }
        }
        if (out == 1){ break; }
    }

    free(image_data);
    fclose(fp_wb);
    return *sum;
}

// Функция фильтрации
void apply_filter_to_all_test_2() {
    int sum_image[10]={0};
    int label, height, width, image_size;

    sum_images_test_2(sum_image);

    // Вывод информации о количестве изображений
    TCHAR sum_msg[256];
    for (int i=0; i < 10; i++){
        _stprintf_s(sum_msg, _T("%d. sum = %d\n"), i, sum_image[i]);
        OutputDebugString(sum_msg);
    }

    FILE* fp_wb, * filterf;
    fp_wb=_tfopen(_T("Обученные файлы\\2. Paint\\1. Тестовая выборка.bin"), _T("rb"));
    filterf=_tfopen(_T("Обученные файлы\\2. Paint\\1. Фильтр Собеля тест.bin"), _T("wb"));

    if (!fp_wb || !filterf){
        MessageBox(NULL, _T("Не удалось открыть файлы для чтения/записи!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        if (fp_wb) fclose(fp_wb);
        if (filterf) fclose(filterf);
        return;
    }

    for (int i=0; i < 10; i++){
        for (int j=0; j < sum_image[i]; j++){
            if (fread(&label, sizeof(int), 1, fp_wb) != 1 ||
                fread(&height, sizeof(int), 1, fp_wb) != 1 ||
                fread(&width, sizeof(int), 1, fp_wb) != 1){
                MessageBox(NULL, _T("Ошибка чтения данных изображения"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                break;
            }

            image_size=width * height;
            double* image_data=(double*) malloc(image_size * sizeof(double));
            if (!image_data){
                MessageBox(NULL, _T("Ошибка выделения памяти"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                break;
            }

            if (fread(image_data, sizeof(double), image_size, fp_wb) != image_size){
                MessageBox(NULL, _T("Ошибка чтения данных изображения"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                free(image_data);
                continue;
            }

            // Фильтр Собеля
            int filter1[9]={-1, -1, -1, 0, 0, 0, 1, 1, 1};
            int filter2[9]={-1, 0, 1, -1, 0, 1, -1, 0, 1};

            double padded[30 * 30]={0.0};
            for (int y=0; y < 28; y++){
                for (int x=0; x < 28; x++){
                    padded[(y + 1) * 30 + (x + 1)]=image_data[y * 28 + x];
                }
            }

            double filtered1[28 * 28]={0.0};
            double filtered2[28 * 28]={0.0};
            double combined[28 * 28]={0.0};

            // Применение фильтра
            for (int y=0; y < 28; y++){
                for (int x=0; x < 28; x++){
                    double sum1=0.0, sum2=0.0;
                    for (int ky=0; ky < 3; ky++){
                        for (int kx=0; kx < 3; kx++){
                            sum1+=padded[(y + ky) * 30 + (x + kx)] * filter1[ky * 3 + kx];
                            sum2+=padded[(y + ky) * 30 + (x + kx)] * filter2[ky * 3 + kx];
                        }
                    }
                    filtered1[y * 28 + x]=sum1;
                    filtered2[y * 28 + x]=sum2;
                    combined[y * 28 + x]=sqrt(sum1 * sum1 + sum2 * sum2);
                }
            }

            // Обработка границ
            for (int k=0; k < 28 * 28; k++){
                if (k < 28 || k > 755 || k % 28 == 0 || k % 28 == 27){
                    combined[k]=0.0;
                }
            }

            // Нормализация
            double max_val=0.0;
            for (int k=0; k < 28 * 28; k++){
                combined[k]=1.0 - combined[k];
                if (combined[k] > max_val) max_val=combined[k];
            }

            if (max_val > 0){
                for (int k=0; k < 28 * 28; k++){
                    combined[k]/=max_val;
                }
            }

            // Запись результата
            fwrite(&label, sizeof(int), 1, filterf);
            int hw=28;
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(combined, sizeof(double), 28 * 28, filterf);

            free(image_data);

            if (j % 2000 == 0){
                TCHAR progress_msg[256];
                _stprintf_s(progress_msg, _T("Загружено фильтрованное изображение: i= %d, j= %d"), i, j);
                OutputDebugString(progress_msg);
            }
        }
    }

    //// Визуализация результатов
    //double* image_data_s=(double*) malloc(28 * 28 * sizeof(double));
    //if (image_data_s){
    //    _tfreopen(_T("C:\\Users\\Пользователь\\Desktop\\Цифры\\Обученные\\5. Распознавание Paint\\1. Фильтр Собеля тест.bin"), _T("rb"), filterf);
    //    for (int i=0; i < 10; i++){
    //        for (int j=0; j < sum_image[i]; j++){
    //            if (fread(&label, sizeof(int), 1, filterf) != 1 ||
    //                fread(&height, sizeof(int), 1, filterf) != 1 ||
    //                fread(&width, sizeof(int), 1, filterf) != 1 ||
    //                fread(image_data_s, sizeof(double), 28 * 28, filterf) != 28 * 28){
    //                break;
    //            }
    //            if (j % 700 == 0 && j != 0){
    //                printer(label, 28, 28, image_data_s);
    //            }
    //        }
    //    }
    //    free(image_data_s);
    //}

    fclose(fp_wb);
    fclose(filterf);
    MessageBox(NULL, _T("Фильтрация изображений завершена успешно!"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
}

// Функция пуллинга
void pooling_test_2() {
    int sum[10]={0};
    sum_images_test_2(sum);

    FILE* pooling_f_rb, * pooling_f_wb;
    pooling_f_rb=_tfopen(_T("Обученные файлы\\2. Paint\\1. Фильтр Собеля тест.bin"), _T("rb"));
    pooling_f_wb=_tfopen(_T("Обученные файлы\\2. Paint\\1. Отпуллингованная тестовая выборка.bin"), _T("wb"));

    if (!pooling_f_rb || !pooling_f_wb){
        MessageBox(NULL, _T("Не удалось открыть файлы для операции pooling!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        if (pooling_f_rb) fclose(pooling_f_rb);
        if (pooling_f_wb) fclose(pooling_f_wb);
        return;
    }

    int label, height, weight, s=0, hw;
    double* image=(double*) malloc(28 * 28 * sizeof(double));
    double max[14 * 14]={0.0};

    if (!image){
        MessageBox(NULL, _T("Ошибка выделения памяти для изображения!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
        fclose(pooling_f_rb);
        fclose(pooling_f_wb);
        return;
    }

    // Записываем информацию о количестве изображений
    fwrite(sum, sizeof(int), 10, pooling_f_wb);

    // Выводим информацию о количестве изображений
    for (int i=0; i < 10; i++){
        TCHAR debugMsg[256];
        _stprintf_s(debugMsg, _T("%d. Количество изображений: %d\n"), i, sum[i]);
        OutputDebugString(debugMsg);
    }

    // Обрабатываем каждое изображение
    for (int i=0; i < 10; i++){
        for (int j=0; j < sum[i]; j++){
            s=0;

            // Читаем данные изображения
            if (fread(&label, sizeof(int), 1, pooling_f_rb) != 1 ||
                fread(&height, sizeof(int), 1, pooling_f_rb) != 1 ||
                fread(&weight, sizeof(int), 1, pooling_f_rb) != 1 ||
                fread(image, sizeof(double), 28 * 28, pooling_f_rb) != 28 * 28){
                MessageBox(NULL, _T("Ошибка чтения данных изображения!"), _T("Ошибка"), MB_OK | MB_ICONERROR);
                break;
            }

            // Применяем max-pooling 2x2
            for (int y=0; y<28; y+=2){
                for (int x=0; x<28; x+=2){
                    max[s]=2.0;
                    for (int ky=0; ky<2; ky++){
                        for (int kx=0; kx<2; kx++){
                            if (image[(y+ky) * 28 + (x+kx)]<max[s]){ max[s]=image[(y+ky) * 28 + (x+kx)]; }
                        }
                    }
                    s++;
                }
            }

            // Записываем результат
            hw=14;
            fwrite(&label, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(max, sizeof(double), s, pooling_f_wb);

            // Вывод прогресса (каждые 100 изображений)
            if (j % 100 == 0){
                TCHAR progressMsg[256];
                _stprintf_s(progressMsg, _T("Обработано изображений: цифра %d - %d/%d"), i, j, sum[i]);
                OutputDebugString(progressMsg);
            }
        }
    }

    // Освобождаем ресурсы
    free(image);
    fclose(pooling_f_rb);
    fclose(pooling_f_wb);

    MessageBox(NULL, _T("Операция pooling успешно завершена!"), _T("Успех"), MB_OK | MB_ICONINFORMATION);
}

// Функция открытия файлов
void LoadCNNData_2() {
    // Загрузка весов и смещений
    FILE* W_b=_tfopen(_T("Обученные файлы\\2. Paint\\1. Матрица весов и вектор коэффициентов смещения.bin"), _T("rb"));
    if (W_b){
        fread(W, sizeof(double), 196 * 10, W_b);
        fread(b, sizeof(double), 10, W_b);
        fclose(W_b);
    }

    // Открываем файлы с изображениями
    original_f=_tfopen(_T("Обученные файлы\\2. Paint\\1. Тестовая выборка.bin"), _T("rb"));
    pooling_f_wb=_tfopen(_T("Обученные файлы\\2. Paint\\1. Отпуллингованная тестовая выборка.bin"), _T("rb"));

    // Берём суммы, так как бинарный файл с отпуллингованными изображениями в самом начале содержит суммы цыфр каждой метки
    if (pooling_f_wb){
        fread(sum, sizeof(int), 10, pooling_f_wb);
        total_images_2=0;
        for (int i=0; i < 10; i++){
            total_images_2+=sum[i];
        }
    }
}

// Функция приведения изображения к виду, готовому к распознаванию
void testing_2(int offset) {
    if (!pooling_f_wb || !original_f) return;

    // Перемещаемся в начало файлов
    fseek(pooling_f_wb, sizeof(int) * 10, SEEK_SET);
    fseek(original_f, 0, SEEK_SET);

    // Пропускаем указанное количество изображений
    for (int i=0; i < offset; i++){
        // Пропускаем в оригинальном файле
        fseek(original_f, sizeof(int) + sizeof(int) + sizeof(int) + 784 * sizeof(double), SEEK_CUR);

        // Пропускаем в пуллингованном файле
        fseek(pooling_f_wb, sizeof(int) + sizeof(int) + sizeof(int) + 196 * sizeof(double), SEEK_CUR);
    }

    // Читаем текущее изображение из пуллингованного файла
    if (fread(&current_label, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(&height_original, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(&width_original, sizeof(int), 1, pooling_f_wb) == 1 &&
        fread(x, sizeof(double), 196, pooling_f_wb) == 196){

        // Читаем соответствующее оригинальное изображение
        fread(&label_original, sizeof(int), 1, original_f);
        fread(&height_original, sizeof(int), 1, original_f);
        fread(&width_original, sizeof(int), 1, original_f);
        fread(pixels_original, sizeof(double), 784, original_f);

        // Распознаем изображение
        double z[10]={0};
        for (int m=0; m < 10; m++){
            for (int n=0; n < 196; n++){
                z[m]+=x[n] * W[n][m];
            }
            z[m]+=b[m];
        }

        double max=z[0];
        for (int m=1; m < 10; m++) if (z[m] > max) max=z[m];

        double yy[10]={0};
        double sum_exp=0.0;
        for (int m=0; m < 10; m++){
            yy[m]=exp(z[m] - max);
            sum_exp+=yy[m];
        }

        for (int m=0; m < 10; m++) yy[m]/=sum_exp;

        double max_prob=0.0;
        current_prediction=0;
        for (int k=0; k < 10; k++){
            if (yy[k] > max_prob){
                max_prob=yy[k];
                current_prediction=k;
            }
        }
    }
    else{
        current_label=-1;
        current_prediction=-1;
    }
}

// Функция отрисовки изображения в виде пикселей в левом окне
void DrawDigit_2(HDC hdc, int x_pos, int y_pos, int cellSize) {
    HBRUSH hBrush=CreateSolidBrush(RGB(0, 255, 0)); // Зеленый цвет
    HPEN hPen=CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // Красный цвет обводки

    HBRUSH hOldBrush=(HBRUSH) SelectObject(hdc, hBrush);
    HPEN hOldPen=(HPEN) SelectObject(hdc, hPen);

    for (int row=0; row < height_original; row++){
        for (int col=0; col < width_original; col++){
            // Почемму здесь высвечивается 1 только при < 0.001 ?
            if (pixels_original[row * width_original + col] == 0.0){
                int left=x_pos + col * cellSize;
                int top=y_pos + row * cellSize;
                Rectangle(hdc, left, top, left + cellSize, top + cellSize);
            }
        }
    }

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hLoadButton, hButton, hResultText, hStatusText, hClearButton, hBinDataButton, hSaveButton, h, h_2;
    static int cellSize=15; // Уменьшим размер клетки для лучшего отображения

    switch (uMsg){
        // Программируем кнопки
    case WM_CREATE: {
        {
            // Просто вывески
            hStatusText=CreateWindow(
                _T("STATIC"), _T("Текущее изображение: 0"),
                WS_VISIBLE | WS_CHILD,
                20, 20, 280, 20,
                hwnd, NULL, NULL, NULL);

            h_2=CreateWindow(
                _T("STATIC"), _T("Нумерация изображений:"),
                WS_VISIBLE | WS_CHILD,
                20, 40, 280, 50,
                hwnd, NULL, NULL, NULL);

            h=CreateWindow(
                _T("STATIC"), _T("Результаты распознавания:"),
                WS_VISIBLE | WS_CHILD,
                20, 80, 280, 50,
                hwnd, NULL, NULL, NULL);
        }

        // 3. Сфотографированные (кнопки)
        {
            hResultText=CreateWindow(
                _T("STATIC"), _T("3. Сфотографированные"),
                WS_VISIBLE | WS_CHILD,
                750, 20, 280, 20,
                hwnd, NULL, NULL, NULL);

            hButton=CreateWindow(
                _T("BUTTON"), _T("1. Обработать изображения"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                750, 50, 280, 30,
                hwnd, (HMENU) 1, NULL, NULL);

            hButton=CreateWindow(
                _T("BUTTON"), _T("2. Распознать изображения"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                750, 80, 280, 30,
                hwnd, (HMENU) 2, NULL, NULL);

        }


        // 4. Своё окно (кнопки)
        {
            hResultText=CreateWindow(
                _T("STATIC"), _T("4. Своё окно"),
                WS_VISIBLE | WS_CHILD,
                450, 20, 280, 20,
                hwnd, NULL, NULL, NULL);

            hClearButton=CreateWindow(_T("BUTTON"), _T("1. Очистить область рисования"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                450, 50, 280, 30, hwnd, (HMENU) 10, NULL, NULL);

            hLoadButton=CreateWindow(_T("BUTTON"), _T("2. Загрузить"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                450, 80, 280, 30, hwnd, (HMENU) 11, NULL, NULL);

            hButton=CreateWindow(_T("BUTTON"), _T("3. Распознать изображение"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                450, 110, 280, 30, hwnd, (HMENU) 13, NULL, NULL);
        }

        // 1. Распознаём mnist (кнопки)
        {
            hStatusText=CreateWindow(
                _T("STATIC"), _T("1. Распознаём mnist"),
                WS_VISIBLE | WS_CHILD,
                1050, 20, 200, 20,
                hwnd, NULL, NULL, NULL);

            hButton=CreateWindow(
                _T("BUTTON"), _T("1. Распознать mnist"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                1050, 50, 200, 30,
                hwnd, (HMENU) 25, NULL, NULL);
        }


        // 2. Paint (кнопки)
        {
            hStatusText=CreateWindow(
                _T("STATIC"), _T("2. Paint"),
                WS_VISIBLE | WS_CHILD,
                750, 370, 300, 20,
                hwnd, NULL, NULL, NULL);

            hButton=CreateWindow(
                _T("BUTTON"), _T("1. Обработать изображения Paint"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                750, 400, 300, 30,
                hwnd, (HMENU) 30, NULL, NULL);

            hButton=CreateWindow(
                _T("BUTTON"), _T("2. Распознать изображения Paint"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                750, 430, 300, 30,
                hwnd, (HMENU) 31, NULL, NULL);

        }

        return 0;
    }

    // Обработка кнопок
    case WM_COMMAND: {
        PAINTSTRUCT ps;
        HDC hdc=BeginPaint(hwnd, &ps);

        // 1. Создаем буфер в памяти для двойной буферизации
        HDC hdcMem=CreateCompatibleDC(hdc);
        HBITMAP hBitmap=CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
        HBITMAP hOldBitmap=(HBITMAP) SelectObject(hdcMem, hBitmap);

        // 3. Сфотографированные
        if (LOWORD(wParam) == 1){
            process_images_3();
            scale_images_3();
            process_final_sharpening_3();
            konvertation_3();
            bin_data_lf_3();
            apply_filter_to_all_test_3();
            pooling_test_3();
        }
        if (LOWORD(wParam) == 2){
            LoadCNNData_3();
            current_offset+=1;
            if (current_offset >= total_images) current_offset=0;

            testing_3(current_offset);

            TCHAR resultText[100], statusText[100];
            if (current_label != -1){
                _stprintf(resultText, _T("Оригинальная метка: %d\nРаспознано как: %d"), label_original, current_prediction);
                _stprintf(statusText, _T("Изображение %d/%d"), current_offset + 1, total_images);
            }
            else{
                _tcscpy(resultText, _T("Достигнут конец файла"));
                _tcscpy(statusText, _T("Ошибка"));
            }

            SetWindowText(h, resultText);
            SetWindowText(h_2, statusText);
            InvalidateRect(hwnd, NULL, TRUE);
        }

        // 4. Своё окно
        if (LOWORD(wParam) == 10){
            ClearDrawingArea_4();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        if (LOWORD(wParam) == 11){
            SaveDrawingToBinary_4();
            apply_filter_to_all_test_4();
            pooling_test_4();
            /*MessageBox(NULL, _T("Изображение обработано и готово к распознаванию!"), _T("Успех"), MB_OK);*/
        }
        if (LOWORD(wParam) == 13){
            current_offset+=0;
            if (current_offset >= total_images) current_offset=0;
            testing_4(current_offset);

            /*TCHAR resultText[100], statusText[100];
            _stprintf(resultText, _T("Оригинальная метка: %d\nРаспознано как: %d"), label_original, current_prediction);
            _stprintf(statusText, _T("Изображение %d/%d"), current_offset + 1, total_images);

            SetWindowText(hResultText, resultText);
            SetWindowText(hStatusText, statusText);*/

            TCHAR resultText[100], statusText[100];
            if (current_label != -1){
                _stprintf(resultText, _T("Оригинальная метка: %d\nРаспознано как: %d"), label_original, current_prediction);
                _stprintf(statusText, _T("Изображение %d/%d"), current_offset + 1, total_images);
            }
            else{
                _tcscpy(resultText, _T("Достигнут конец файла"));
                _tcscpy(statusText, _T("Ошибка"));
            }

            SetWindowText(h, resultText);
            SetWindowText(h_2, statusText);
            InvalidateRect(hwnd, NULL, TRUE);
        }

        // 1. Распознаём mnist
        if (LOWORD(wParam) == 25){

            //// 2. Заливаем фон окна
            //FillRect(hdcMem, &ps.rcPaint, (HBRUSH) (7));
            LoadCNNData_1();
            current_offset_1+=500;
            if (current_offset_1 >= total_images) current_offset_1=0;

            testing_1(current_offset_1);

            TCHAR resultText[100], statusText[100];
            if (current_label != -1){
                _stprintf(resultText, _T("Оригинальная метка: %d\nРаспознано как: %d"), label_original, current_prediction);
                _stprintf(statusText, _T("Изображение %d/%d"), current_offset_1 + 1, total_images);
            }
            else{
                _tcscpy(resultText, _T("Достигнут конец файла"));
                _tcscpy(statusText, _T("Ошибка"));
            }

            SetWindowText(h, resultText);
            SetWindowText(h_2, statusText);
            InvalidateRect(hwnd, NULL, TRUE);

            PAINTSTRUCT ps;
            HDC hdc=BeginPaint(hwnd, &ps);

            ////// 1. Создаем буфер в памяти для двойной буферизации
            ////HDC hdcMem=CreateCompatibleDC(hdc);
            ////HBITMAP hBitmap=CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
            ////HBITMAP hOldBitmap=(HBITMAP) SelectObject(hdcMem, hBitmap);
            /*FillRect(hdcMem, &ps.rcPaint, (HBRUSH) (7));
            Rectangle(hdcMem, 10, 200, 10 + width_original * cellSize, 200 + height_original * cellSize);*/
            DrawDigit_1(hdc, 10, 200, cellSize);
            InvalidateRect(hwnd, NULL, TRUE);
        }

        // 2. Paint
        if (LOWORD(wParam) == 30){
            konvertation_2();
            bin_data_lf_2();
            apply_filter_to_all_test_2();
            pooling_test_2();
        }
        if (LOWORD(wParam) == 31){

            // 2. Заливаем фон окна
            FillRect(hdcMem, &ps.rcPaint, (HBRUSH) (7));

            LoadCNNData_2();
            current_offset_2+=1;
            if (current_offset_2 >= total_images_2) current_offset_2=0;

            testing_2(current_offset_2);

            TCHAR resultText[100], statusText[100];
            if (current_label != -1){
                _stprintf(resultText, _T("Оригинальная метка: %d\nРаспознано как: %d"), label_original, current_prediction);
                _stprintf(statusText, _T("Изображение %d/%d"), current_offset_2 + 1, total_images_2);
            }
            else{
                _tcscpy(resultText, _T("Достигнут конец файла, перезапустите программу"));
                _tcscpy(statusText, _T("Ошибка"));
            }

            SetWindowText(h, resultText);
            SetWindowText(h_2, statusText);

            InvalidateRect(hwnd, NULL, TRUE);
        }
        currentMode=wParam;
        return 0;
    }

    // Обновляем окно программы после нажатия каждой кнопки
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc=BeginPaint(hwnd, &ps);

        // 1. Создаем буфер в памяти для двойной буферизации
        HDC hdcMem=CreateCompatibleDC(hdc);
        HBITMAP hBitmap=CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
        HBITMAP hOldBitmap=(HBITMAP) SelectObject(hdcMem, hBitmap);

        // 2. Заливаем фон окна
        FillRect(hdcMem, &ps.rcPaint, (HBRUSH) (7));

        // 3. Отрисовываем тестовое изображение (верхняя часть)
        if (current_label != -1){
            // Рамка вокруг изображения
            Rectangle(hdcMem, 10, 200, 10 + width_original * cellSize, 200 + height_original * cellSize);

            if (LOWORD(currentMode) == 11){ DrawDigit_4(hdcMem, 10, 200, cellSize); }
            if (LOWORD(currentMode) == 13){ DrawDigit_4(hdcMem, 10, 200, cellSize); }
            if (LOWORD(currentMode) == 2){ DrawDigit_3(hdcMem, 10, 200, cellSize); }
            if (LOWORD(currentMode) == 25){ DrawDigit_1(hdcMem, 10, 200, cellSize); }
            if (LOWORD(currentMode) == 31){ DrawDigit_2(hdcMem, 10, 200, cellSize); }

        }

        // 4. Отрисовываем область для рисования (средняя часть)
        {
            // Черный фон
            RECT drawArea={
                drawingAreaX,
                drawingAreaY,
                drawingAreaX + width_original * drawingCellSize,
                drawingAreaY + height_original * drawingCellSize
            };
            FillRect(hdcMem, &drawArea, (HBRUSH) GetStockObject(BLACK_BRUSH));

            // Белые пиксели
            HBRUSH hWhiteBrush=CreateSolidBrush(RGB(255, 255, 255));
            HPEN hNullPen=CreatePen(PS_SOLID, 0, RGB(255, 255, 255)); ////// Вот обводка пикселей окна для рисования!!!

            HBRUSH hOldBrush=(HBRUSH) SelectObject(hdcMem, hWhiteBrush);
            HPEN hOldPen=(HPEN) SelectObject(hdcMem, hNullPen);

            for (int row=0; row < height_original; row++){
                for (int col=0; col < width_original; col++){
                    if (drawingPixels[row * width_original + col] > 0.5){ // Черное окно для рисования
                        int left=drawingAreaX + col * drawingCellSize;
                        int top=drawingAreaY + row * drawingCellSize;
                        Rectangle(hdcMem, left, top, left + drawingCellSize, top + drawingCellSize);
                    }
                }
            }



            SelectObject(hdcMem, hOldBrush);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hWhiteBrush);
            DeleteObject(hNullPen);
        }

        // 6. Копируем буфер на экран
        BitBlt(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, hdcMem, 0, 0, SRCCOPY);

        // 7. Освобождаем ресурсы
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    // Сообщение, которое приходит, когда окно программы закрывается.
    case WM_DESTROY: {
        if (pooling_f_wb) fclose(pooling_f_wb);
        if (original_f) fclose(original_f);
        PostQuitMessage(0);
        return 0;
    }

    // Сообщение о нажатии левой кнопки мыши
    // Устанавливается флаг isDrawing=1, означающий, что началось рисование.
    case WM_LBUTTONDOWN: {
        isDrawing=1;
        UpdateDrawing_4(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    // Сообщение об отпускании левой кнопки мыши.
    // Сбрасывает флаг isDrawing=0 - рисование заканчивается.
    case WM_LBUTTONUP: {
        isDrawing=0;
        return 0;
    }
                     
    // Сообщение о движении мыши по черному окну
    case WM_MOUSEMOVE: {
        if (isDrawing==1){
            UpdateDrawing_4(LOWORD(lParam), HIWORD(lParam));
            RECT updateRect={
                drawingAreaX,
                drawingAreaY,
                drawingAreaX + width_original * drawingCellSize,
                drawingAreaY + height_original * drawingCellSize
            };
            InvalidateRect(hwnd, &updateRect, FALSE); // FALSE = не стирать фон
        }
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    _tsetlocale(LC_ALL, _T(""));

    ULONG_PTR gdiplusToken;

    const TCHAR CLASS_NAME[]=_T("CNN Viewer");

    WNDCLASS wc={0};
    wc.lpfnWndProc=WindowProc;
    wc.hInstance=hInstance;
    wc.lpszClassName=CLASS_NAME;
    wc.hbrBackground=(HBRUSH) (COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd=CreateWindow(
        CLASS_NAME, _T("Распознавание цифр CNN"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 1200,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg={0};
    while (GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

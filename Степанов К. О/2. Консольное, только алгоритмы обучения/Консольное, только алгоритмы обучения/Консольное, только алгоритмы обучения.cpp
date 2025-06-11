// Консольное, только алгоритмы обучения.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

// "1. Хранение обучающей выборки.bin"
// "1. Фильтр Собеля.bin"
// "1. Пулинг.bin"
// "1. Выходные вектора.bin"
// "1. Суммы изображений.bin"
// "1. Матрица весов и вектор коэффициентов смещения.bin"
// "1. Копия векторов.bin"
// "1. Тестовая выборка.bin"
// "1. Фильтр Собеля тест.bin"
// "1. Отпуллингованная тестовая выборка.bin"
// "1. Матрица весов и вектор коэффициентов смещения.bin"
// 

// Максимально обученная модель CNN. Точность 88.5%. Обучена за 3726 секунд. А до 76.8% обучилась за 73 секунды
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <Windows.h>
#include <math.h>
#include <time.h>


void printer(int label, int width, int height, double* pixel){
    printf("Метка: %d\n", label);
    printf("Ширина: %d, Высота: %d\n", width, height);
    printf("Пиксели:\n");
    for (int row=0; row < width; row++){
        for (int col=0; col < height; col++){
            /*printf("%.1f ", pixel[row * height + col]);*/
            if (pixel[row * width + col]<0.5){ printf("*"); }
            else{ printf(" "); }
        }
        printf("\n");
    }
}

// Функция формирования черно-белых обучащих изображений и сохранения его в бинарный файл
int bin_data(){
    char filename[256];

    FILE* fp_wb, * file;
    fp_wb=fopen("Обученные файлы\\1. Хранение обучающей выборки.bin", "wb");
    // Формируем имя файла и проверяем, существует ли он
    for (int i=0; i<10; i++){
        for (int j=0; j<7000; j++){
            sprintf_s(filename, sizeof(filename), "train\\60tis_image\\%d_00%04d.pgm", i, j);
            file=fopen(filename, "rb");
            if (!file){ printf("Файл %s не существует.\n", filename); break; } // Проверяем, существует ли файл

            // filename - это переменная, содержащая путь файла с изображениями формата PGM

            int width, height, maxval;
            unsigned char* image_data;
            size_t image_size;
            // Считываем ширину, высоту и размер изображения
            if (fscanf_s(file, "P5\n%d %d\n%d\n", &width, &height, &maxval) != 3){
                fprintf(stderr, "Ошибка чтения заголовка PGM\n");
                fclose(file);
                return 2;
            }
            /*printf("Width: %lld, Height: %lld, Maxval: %lld\n", width, height, maxval);*/
            // Заносим в бинарный файл метку, высоту и ширину
            fwrite(&i, sizeof(int), 1, fp_wb);
            fwrite(&height, sizeof(int), 1, fp_wb);
            fwrite(&width, sizeof(int), 1, fp_wb);


            // Вычисление размера изображения и выделение памяти
            image_size=(size_t) width * (size_t) height;
            image_data=(unsigned char*) malloc(image_size * sizeof(unsigned char));
            if (image_data == NULL){
                printf("Ошибка выделения памяти");
                fclose(file);
                return 2;
            }

            // Чтение ровно 28*28 (image_size) пикселей из переданного файла в массив image_data
            if (fread(image_data, sizeof(unsigned char), image_size, file) != image_size){
                fprintf(stderr, "Ошибка чтения данных изображения\n");
                free(image_data);
                fclose(file);
                return 2;
            }

            // Преобразование в черно-белое
            double* binary_image=(double*) malloc(image_size * sizeof(double));
            if (binary_image == NULL){
                printf("Ошибка выделения памяти для черно-белого изображения");
                free(image_data);
                return 1;
            }
            // Считываем каждый пиксель в массив пикселей binary_image из массива пикселей image_data 
            for (int i=0; i < image_size; i++){
                binary_image[i]=(255.0 - (double) image_data[i]) / 255.0;
            }
            /*printer(i, height, width, binary_image);*/
            // Записываем чёрно-белыt пикселей в виде 0 и 1 в глобально объявленный бинарный файл (fp_wb)
            fwrite(binary_image, sizeof(double), image_size, fp_wb);
            fclose(file);
            fflush(fp_wb);

            if (j%1000==0){ printf("Загружено изображение %s, метка %d\n", filename, i); }
            free(image_data);
            free(binary_image);
        }
    }

    fclose(fp_wb);
    return 5;
}

int sum_images(int sum[10]){
    FILE* fp_wb;
    int label, height, width, new_label=0, out=0, elements_read;
    double* image_data=(double*) malloc(28*28 * sizeof(double));
    fp_wb=fopen("Обученные файлы\\1. Хранение обучающей выборки.bin", "rb");
    for (int i=0; i<10; i++){
        for (int j=0; j<7000; j++){

            fread(&label, sizeof(int), 1, fp_wb);
            fread(&height, sizeof(int), 1, fp_wb);
            fread(&width, sizeof(int), 1, fp_wb);

            elements_read=fread(image_data, sizeof(double), 28 * 28, fp_wb);
            if (elements_read != 28 * 28){ out=1; break; }
            if (new_label==label){ sum[i]++; }
            else{ new_label=label; sum[i+1]++; break; }
        }
        if (out==1){ break; }
    }
    free(image_data);
    fclose(fp_wb);
    return *sum;
}


// Фильтрация обучающих изображений
void apply_filter_to_all(){
    int sum_image[10]={0};
    int label, height, width, image_size;
    sum_images(sum_image);
    for (int i=0; i<10; i++){
        printf("%d\n", sum_image[i]);
    }

    FILE* fp_wb, * filterf;
    fp_wb=fopen("Обученные файлы\\1. Хранение обучающей выборки.bin", "rb");
    filterf=fopen("Обученные файлы\\1. Фильтр Собеля.bin", "wb");

    for (int i=0; i<10; i++){
        for (int j=0; j<sum_image[i]; j++){
            fread(&label, sizeof(int), 1, fp_wb);
            fread(&height, sizeof(int), 1, fp_wb);
            fread(&width, sizeof(int), 1, fp_wb);

            image_size=width * height;
            double* image_data=(double*) malloc(image_size * sizeof(double));
            fread(image_data, sizeof(double), image_size, fp_wb);
            /*if (j==0){
                printf("Оригинал\n");
                printer(label, 28, 28, image_data);
            }*/


            // Фильтр 3x3
            int filter1[9]={-1, -1, -1,
                0, 0, 0,
                1, 1, 1};
            int filter2[9]={-1, 0, 1,
                -1, 0, 1,
                -1, 0, 1};

            double padded[30*30]={0.0};
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
                    // Проход фильтрами 3x3 по изображению
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

            // Вычисление градиента яркости(величины изменения) в каждой точке изображения.
            for (int i=0; i < 28*28; i++){
                if (i<28 || i>755 || i%28==0 || i%28==27){ combined[i]=0.0; }
                // Вычисляем длину вектора градиента в точке изображения
                else{ combined[i]=sqrt(filtered1[i] * filtered1[i] + filtered2[i] * filtered2[i]); }
            }

            for (int i=0; i < 28*28; i++){
                combined[i]=1.0 - combined[i];  // Инверсия: фон станет белым, а цифра снова черной
            }

            free(image_data);

            fwrite(&label, sizeof(int), 1, filterf);
            int hw=28;
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(combined, sizeof(double), 28*28, filterf);
            if (j%2000==0){ printf("Загружено фильтрованное изображение: i= %d, j= %d\n", i, j); }
        }
    }
    //// Если хотим вывести на экран отфильтрованное изображение
    fclose(filterf);
    filterf=fopen("Обученные файлы\\1. Фильтр Собеля.bin", "rb");
    double* image_data_s=(double*) malloc(28*28 * sizeof(double));
    for (int i=0; i<10; i++){
        for (int j=0; j<sum_image[i]; j++){
            fread(&label, sizeof(int), 1, filterf);
            fread(&height, sizeof(int), 1, filterf);
            fread(&width, sizeof(int), 1, filterf);
            fread(image_data_s, sizeof(double), 28*28, filterf);
            if (j%5000==0 && j!=0){ printer(label, 28, 28, image_data_s); }
        }
    }
    free(image_data_s);

    fclose(fp_wb);
    fclose(filterf);
    printf("Изображения загружены.\n");
}

// Сжатие обучающих изображений
void pooling(){
    FILE* pooling_f_rb, * pooling_f_wb;
    // pooling_f_rb=fopen("1. Хранение обучающей выборки.bin", "rb");
    pooling_f_rb=fopen("Обученные файлы\\1. Фильтр Собеля.bin", "rb");
    pooling_f_wb=fopen("Обученные файлы\\1. Пулинг.bin", "wb");

    int sum[10]={0}, label, height, weight, s=0, hw;
    double* image=(double*) malloc(28*28 * sizeof(double));
    double max[14*14];
    double pix[14*14];
    sum_images(sum);
    fwrite(sum, sizeof(int), 10, pooling_f_wb);
    for (int i=0; i<10; i++){
        printf("%d\n", sum[i]);
    }
    for (int i=0; i<10; i++){
        for (int j=0; j<sum[i]; j++){
            s=0;
            fread(&label, sizeof(int), 1, pooling_f_rb);
            fread(&height, sizeof(int), 1, pooling_f_rb);
            fread(&weight, sizeof(int), 1, pooling_f_rb);
            fread(image, sizeof(double), 28*28, pooling_f_rb);

            // Берём одно изображение и пропускаем по нему матрицу размером 2*2. Ищем самый черный (наименьший) пиксель по 
            // каждому участку размером 2*2 и сохраняем его в отдельный массив, который потом выносим в бинарный файл.
            // Делаем так с каждым
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
            hw=14;
            fwrite(&label, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(max, sizeof(double), s, pooling_f_wb);
        }
    }
    fclose(pooling_f_rb);
    fclose(pooling_f_wb);
    pooling_f_wb=fopen("Обученные файлы\\1. Пулинг.bin", "rb");
    fread(sum, sizeof(int), 10, pooling_f_wb);
    for (int i=0; i<10; i++){
        for (int j=0; j<sum[i]; j++){
            fread(&label, sizeof(int), 1, pooling_f_wb);
            fread(&height, sizeof(int), 1, pooling_f_wb);
            fread(&weight, sizeof(int), 1, pooling_f_wb);
            fread(max, sizeof(double), 14*14, pooling_f_wb);

            if (j%5000==0 && j!=0){ printer(label, height, weight, max); }
        }
    }
    fclose(pooling_f_wb);
}

// Внимание!!! Происходит инициализация компонентов, которые нужно обучать.
// z = sum(W*x) + b; - Сырые логиты;
// y = z[i]/sum(z[10]); - Логиты, переведённые в вероятностные коэффициенты;
// Обучаемые весовые коэффициенты - W;
// Статические коэффициенты - x - пиксели изображений;
// Обучаемый коэффициент сммещения - b;

// Внимание!!! Происходит перезапись сумм изображений в отдельный файл.
void weights(){
    FILE* pooling_f_rb, * output_vector, * sum_images, * W_b;

    pooling_f_rb=fopen("Обученные файлы\\1. Пулинг.bin", "rb");
    output_vector=fopen("Обученные файлы\\1. Выходные вектора.bin", "wb");
    sum_images=fopen("Обученные файлы\\1. Суммы изображений.bin", "wb");
    double W[14*14][10], x[14*14], y[10], b[10], z[10];
    int sum[10]={0}, label, height, weight;
    for (int n=0; n < 10; n++){ b[n]=0.01; }
    for (int m=0; m < 10; m++){
        for (int n=0; n < 196; n++){
            W[n][m]=((double) rand() / RAND_MAX) * 0.2 - 0.1;  // Случайные числа ? [-0.1, 0.1]
        }
    }
    W_b=fopen("Обученные файлы\\1. Матрица весов и вектор коэффициентов смещения.bin", "wb");
    fwrite(W, sizeof(double), 196*10, W_b);
    fwrite(b, sizeof(double), 10, W_b);
    fclose(W_b);

    fread(sum, sizeof(int), 10, pooling_f_rb);
    fwrite(sum, sizeof(int), 10, sum_images);
    fclose(sum_images);
    for (int n=0; n < 10; n++){ printf("%d\n", sum[n]); }

    for (int i=0; i<10; i++){
        for (int j=0; j<sum[i]; j++){
            for (int n=0; n < 10; n++){ z[n]=0; }
            fread(&label, sizeof(int), 1, pooling_f_rb);
            fread(&height, sizeof(int), 1, pooling_f_rb);
            fread(&weight, sizeof(int), 1, pooling_f_rb);
            fread(x, sizeof(double), 14*14, pooling_f_rb);

            for (int m=0; m<10; m++){
                for (int n=0; n<196; n++){
                    z[m]+=x[n] * W[n][m];  // Матричное умножение
                }
                z[m]+=b[m];  // Добавляем смещение
                if (i==0 && j==0){ printf("%f ", z[m]); }
            }

            // Используем softmax - преобразование вектора чисел (y) в вектор вероятностей
            double max=z[0];
            for (int m=1; m<10; m++){
                if (z[m] > max){ max=z[m]; }
            }
            double sum=0.0;
            for (int m=0; m<10; m++){
                z[m]=exp(z[m] - max);
                sum+=z[m];
            }

            // А это уже самый настоящий softmax (нормализация)
            for (int m=0; m<10; m++){ y[m]=z[m]/sum; }

            fwrite(&label, sizeof(int), 1, output_vector);
            fwrite(y, sizeof(double), 10, output_vector);
            fwrite(x, sizeof(double), 14*14, output_vector);
        }
    }
    fclose(pooling_f_rb);
    fclose(output_vector);
    output_vector=fopen("Обученные файлы\\1. Выходные вектора.bin", "rb");
    sum_images=fopen("Обученные файлы\\1. Суммы изображений.bin", "rb");
    fread(sum, sizeof(int), 10, sum_images);
    for (int m=0; m<10; m++){ printf("%d\n", sum[m]); }
    for (int i=0; i<10; i++){
        for (int j=0; j<sum[i]; j++){
            fread(&label, sizeof(int), 1, output_vector);
            fread(y, sizeof(double), 10, output_vector);
            fread(x, sizeof(double), 14*14, output_vector);
            if (j==0){
                double summ=0.0;
                printf("Метка: %d\n", label);
                for (int m=0; m<10; m++){ printf("%f ", y[m]); summ+=y[m]; }

                printf(" %f\n", summ);
            }
        }
    }

    /*for (int n=0; n < 196; n++){
        for (int m=0; m < 10; m++){
            printf("%f ", W[n][m]);
        }
        printf("\n");
    }*/
    fclose(output_vector);
    fclose(sum_images);
}

// Обучение инициализированных ранее компонентов (ядро программы)
void learning(){
    int label, sum[10]={0};
    double loss_print, y[10]={0}, yy[10]={0}, W[196][10], b[10], x[196];
    double loss=0.0;
    FILE* output_vector, * copy_vector, * sum_images, * W_b;
    sum_images=fopen("Обученные файлы\\1. Суммы изображений.bin", "rb");
    fread(sum, sizeof(int), 10, sum_images);
    for (int i=0; i<10; i++){ printf("%d\n", sum[i]); }
    fclose(sum_images);

    W_b=fopen("Обученные файлы\\1. Матрица весов и вектор коэффициентов смещения.bin", "rb");
    fread(W, sizeof(double), 196*10, W_b);
    fread(b, sizeof(double), 10, W_b);
    fclose(W_b);


    double lambda=0.001; // Сила регуляризации
    for (int epoch=0; epoch<500; epoch++){
        double yy[10]={0};
        printf("\nОбучающая итерация: %d\n", epoch);
        output_vector=fopen("1. Выходные вектора.bin", "rb");
        copy_vector=fopen("1. Копия векторов.bin", "wb");

        for (int i=0; i<10; i++){
            /*printf("Просмотрена метка: %d\n", i);*/
            for (int j=0; j<sum[i]; j++){

                fread(&label, sizeof(int), 1, output_vector);
                fread(y, sizeof(double), 10, output_vector);
                fread(x, sizeof(double), 14*14, output_vector);


                loss=-log(y[label] + 1e-10)/* + lambda * l2_penalty*/;
                if (i==0 && j==0){ printf("loss= %f\n", loss); }

                /*printf("grad ");*/
                double learning_rate=0.000001;
                /*if (epoch%10==0 && epoch!=0){ learning_rate=0.001; }*/

                for (int m=0; m < 10; m++){
                    double grad=y[m] - (m == label?1.0:0.0);
                    /*printf("%f ", grad);*/
                    for (int n=0; n < 196; n++){
                        W[n][m]-=learning_rate * (grad * x[n]);
                    }
                    b[m]-=learning_rate * grad;
                }/*printf("\n");*/

                // Вычисление сырых логитов
                double z[10]={0};
                for (int m=0; m<10; m++){
                    for (int n=0; n<196; n++){
                        z[m]+=x[n] * W[n][m];  // Матричное умножение
                    }
                    z[m]+=b[m];  // Добавляем смещение
                }

                // Нахождение максимального значения 
                // для числовой стабильности softmax
                double max=z[0];
                for (int m=1; m<10; m++){
                    if (z[m] > max){ max=z[m]; }
                }

                // Экспоненцирование разности сырых логитов 
                // и максимального значения - 
                // СТАБИЛИЗАЦИЯ и подготовка к softmax
                double sum=0.0;
                for (int m=0; m<10; m++){
                    yy[m]=exp(z[m] - max);
                    sum+=yy[m];
                }

                // А это уже самый настоящий softmax (нормализация)
                for (int m=0; m<10; m++){ yy[m]/=sum; }

                fwrite(&label, sizeof(int), 1, copy_vector);
                fwrite(yy, sizeof(double), 10, copy_vector);
                fwrite(x, sizeof(double), 14*14, copy_vector);
            }
        }
        fclose(copy_vector);
        fclose(output_vector);

        output_vector=fopen("1. Выходные вектора.bin", "wb");
        copy_vector=fopen("1. Копия векторов.bin", "rb");
        for (int i=0; i<10; i++){
            for (int j=0; j<sum[i]; j++){
                fread(&label, sizeof(int), 1, copy_vector);
                fread(yy, sizeof(double), 10, copy_vector);
                fread(x, sizeof(double), 14*14, copy_vector);
                double ssum=0;
                if (i==0 && j==0){
                    for (int m=0; m<10; m++){ printf("%f ", yy[m]); ssum+=yy[m]; }
                    printf("%f\n", ssum);
                    /*for (int m=0; m<14*14; m++){
                        if (m!=0 && m%14==0){ printf("\n"); }
                        if (x[m]<1){ printf("#"); }
                        else{ printf("*"); }
                    }*/
                }
                fwrite(&label, sizeof(int), 1, output_vector);
                fwrite(yy, sizeof(double), 10, output_vector);
                fwrite(x, sizeof(double), 14*14, output_vector);
            }
        }
        fclose(copy_vector);
        fclose(output_vector);
    }
    W_b=fopen("1. Матрица весов и вектор коэффициентов смещения.bin", "wb");
    fwrite(W, sizeof(double), 196*10, W_b);
    fwrite(b, sizeof(double), 10, W_b);
    fclose(W_b);
}



// Загрузка в бинарный файл тестовых изображений
int bin_data_test(){
    char filename[256];

    FILE* fp_wb, * file;
    fp_wb=fopen("Обученные файлы\\1. Тестовая выборка.bin", "wb");
    // Формируем имя файла и проверяем, существует ли он
    for (int i=0; i<10; i++){
        for (int j=0; j<7000; j++){
            sprintf_s(filename, sizeof(filename), "test\\converted\\%d_00%04d.pgm", i, j);
            file=fopen(filename, "rb");
            if (!file){ printf("Файл %s не существует.\n", filename); break; } // Проверяем, существует ли файл

            // filename - это переменная, содержащая путь файла с изображениями формата PGM

            int width, height, maxval;
            unsigned char* image_data;
            size_t image_size;
            // Считываем ширину, высоту и размер изображения
            if (fscanf_s(file, "P5\n%d %d\n%d\n", &width, &height, &maxval) != 3){
                fprintf(stderr, "Ошибка чтения заголовка PGM\n");
                fclose(file);
                return 2;
            }
            /*printf("Width: %lld, Height: %lld, Maxval: %lld\n", width, height, maxval);*/
            // Заносим в бинарный файл метку, высоту и ширину
            fwrite(&i, sizeof(int), 1, fp_wb);
            fwrite(&height, sizeof(int), 1, fp_wb);
            fwrite(&width, sizeof(int), 1, fp_wb);


            // Вычисление размера изображения и выделение памяти
            image_size=(size_t) width * (size_t) height;
            image_data=(unsigned char*) malloc(image_size * sizeof(unsigned char));
            if (image_data == NULL){
                printf("Ошибка выделения памяти");
                fclose(file);
                return 2;
            }

            // Чтение ровно 28*28 (image_size) пикселей из переданного файла в массив image_data
            if (fread(image_data, sizeof(unsigned char), image_size, file) != image_size){
                fprintf(stderr, "Ошибка чтения данных изображения\n");
                free(image_data);
                fclose(file);
                return 2;
            }

            // Преобразование в черно-белое
            double* binary_image=(double*) malloc(image_size * sizeof(double));
            if (binary_image == NULL){
                printf("Ошибка выделения памяти для черно-белого изображения");
                free(image_data);
                return 1;
            }
            // Считываем каждый пиксель в массив пикселей binary_image из массива пикселей image_data 
            for (int i=0; i < image_size; i++){
                binary_image[i]=(255.0 - (double) image_data[i]) / 255.0;
            }
            /*printer(i, height, width, binary_image);*/
            // Записываем чёрно-белыt пикселей в виде 0 и 1 в глобально объявленный бинарный файл (fp_wb)
            fwrite(binary_image, sizeof(double), image_size, fp_wb);
            fclose(file);
            fflush(fp_wb);

            if (j%1000==0){ printf("Загружено изображение %s, метка %d\n", filename, i); }
            free(image_data);
            free(binary_image);
        }
    }

    fclose(fp_wb);
    return 5;
}

int sum_images_test(int sum[10]){
    FILE* fp_wb;
    int label, height, width, new_label=0, out=0, elements_read;
    double* image_data=(double*) malloc(28*28 * sizeof(double));
    fp_wb=fopen("Обученные файлы\\1. Тестовая выборка.bin", "rb");
    for (int i=0; i<10; i++){
        for (int j=0; j<7000; j++){

            fread(&label, sizeof(int), 1, fp_wb);
            fread(&height, sizeof(int), 1, fp_wb);
            fread(&width, sizeof(int), 1, fp_wb);

            elements_read=fread(image_data, sizeof(double), 28 * 28, fp_wb);
            if (elements_read != 28 * 28){ out=1; break; }
            if (new_label==label){ sum[i]++; }
            else{ new_label=label; sum[i+1]++; break; }
        }
        if (out==1){ break; }
    }
    free(image_data);
    fclose(fp_wb);
    return *sum;
}

// Фильтрация тестовых изображений
void apply_filter_to_all_test(){
    int sum_image[10]={0};
    int label, height, width, image_size;
    sum_images_test(sum_image);
    for (int i=0; i<10; i++){
        printf("%d\n", sum_image[i]);
    }

    FILE* fp_wb, * filterf;
    fp_wb=fopen("Обученные файлы\\1. Тестовая выборка.bin", "rb");
    filterf=fopen("Обученные файлы\\1. Фильтр Собеля тест.bin", "wb");

    for (int i=0; i<10; i++){
        for (int j=0; j<sum_image[i]; j++){
            fread(&label, sizeof(int), 1, fp_wb);
            fread(&height, sizeof(int), 1, fp_wb);
            fread(&width, sizeof(int), 1, fp_wb);

            image_size=width * height;
            double* image_data=(double*) malloc(image_size * sizeof(double));
            fread(image_data, sizeof(double), image_size, fp_wb);
            /*if (j==0){
                printf("Оригинал\n");
                printer(label, 28, 28, image_data);
            }*/


            // Фильтр 3x3
            int filter1[9]={-1, -1, -1,
                0, 0, 0,
                1, 1, 1};
            int filter2[9]={-1, 0, 1,
                -1, 0, 1,
                -1, 0, 1};

            double padded[30*30]={0.0};
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

            for (int i=0; i < 28*28; i++){
                combined[i]=1.0 - combined[i];  // Инверсия: фон станет белым, а цифра снова черной
            }

            free(image_data);

            fwrite(&label, sizeof(int), 1, filterf);
            int hw=28;
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(&hw, sizeof(int), 1, filterf);
            fwrite(combined, sizeof(double), 28*28, filterf);
            if (j%2000==0){ printf("Загружено фильтрованное изображение: i= %d, j= %d\n", i, j); }
        }
    }
    //// Если хотим вывести на экран отфильтрованное изображение
    fclose(filterf);
    filterf=fopen("Обученные файлы\\1. Фильтр Собеля тест.bin", "rb");
    double* image_data_s=(double*) malloc(28*28 * sizeof(double));
    for (int i=0; i<10; i++){
        for (int j=0; j<sum_image[i]; j++){
            fread(&label, sizeof(int), 1, filterf);
            fread(&height, sizeof(int), 1, filterf);
            fread(&width, sizeof(int), 1, filterf);
            fread(image_data_s, sizeof(double), 28*28, filterf);
            if (j%700==0 && j!=0){ printer(label, 28, 28, image_data_s); }
        }
    }
    free(image_data_s);

    fclose(fp_wb);
    fclose(filterf);
    printf("Изображения загружены.\n");
}

// Сжатие тестовых изображений
void pooling_test(){
    int sum[10]={0};
    sum_images_test(sum);
    FILE* pooling_f_rb, * pooling_f_wb;
    pooling_f_rb=fopen("Обученные файлы\\1. Фильтр Собеля тест.bin", "rb");
    pooling_f_wb=fopen("Обученные файлы\\1. Отпуллингованная тестовая выборка.bin", "wb");

    int label, height, weight, s=0, hw;
    double* image=(double*) malloc(28*28 * sizeof(double));
    double max[14*14];
    double pix[14*14];

    fwrite(sum, sizeof(int), 10, pooling_f_wb);
    for (int i=0; i<10; i++){
        printf("%d\n", sum[i]);
    }
    for (int i=0; i<10; i++){
        for (int j=0; j<sum[i]; j++){
            s=0;
            fread(&label, sizeof(int), 1, pooling_f_rb);
            fread(&height, sizeof(int), 1, pooling_f_rb);
            fread(&weight, sizeof(int), 1, pooling_f_rb);
            fread(image, sizeof(double), 28*28, pooling_f_rb);

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
            hw=14;
            fwrite(&label, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(&hw, sizeof(int), 1, pooling_f_wb);
            fwrite(max, sizeof(double), s, pooling_f_wb);
        }
    }
    fclose(pooling_f_rb);
    fclose(pooling_f_wb);
    pooling_f_wb=fopen("Обученные файлы\\1. Отпуллингованная тестовая выборка.bin", "rb");
    fread(sum, sizeof(int), 10, pooling_f_wb);
    for (int i=0; i<10; i++){
        for (int j=0; j<sum[i]; j++){
            fread(&label, sizeof(int), 1, pooling_f_wb);
            fread(&height, sizeof(int), 1, pooling_f_wb);
            fread(&weight, sizeof(int), 1, pooling_f_wb);
            fread(max, sizeof(double), 14*14, pooling_f_wb);

            if (j%700==0 && j!=0){ printer(label, height, weight, max); }
        }
    }
    fclose(pooling_f_wb);
}

// Распознавание
void testing(){
    FILE* W_b, * pooling_f_wb;
    pooling_f_wb=fopen("Обученные файлы\\1. Отпуллингованная тестовая выборка.bin", "rb");
    W_b=fopen("Обученные файлы\\1. Матрица весов и вектор коэффициентов смещения.bin", "rb");

    double loss_print, y[10]={0}, yy[10]={0}, W[196][10], b[10], x[196];
    int sum[10];

    double tochnost=0.0;
    int summa=0;

    fread(sum, sizeof(int), 10, pooling_f_wb);
    for (int i=0; i<10; i++){ printf("%d\n", sum[i]); }

    fread(W, sizeof(double), 196*10, W_b);
    fread(b, sizeof(double), 10, W_b);

    int label, height, weight;

    for (int i=0; i<10; i++){
        /*printf("Просмотрена метка: %d\n", i);*/
        for (int j=0; j<sum[i]; j++){

            fread(&label, sizeof(int), 1, pooling_f_wb);
            fread(&height, sizeof(int), 1, pooling_f_wb);
            fread(&weight, sizeof(int), 1, pooling_f_wb);
            fread(x, sizeof(double), 14*14, pooling_f_wb);

            // Вычисление сырых логитов
            double z[10]={0};
            for (int m=0; m<10; m++){
                for (int n=0; n<196; n++){
                    z[m]+=x[n] * W[n][m];  // Матричное умножение
                }
                z[m]+=b[m];  // Добавляем смещение
            }

            // Нахождение максимального значения 
            // для числовой стабильности softmax
            double max=z[0];
            for (int m=1; m<10; m++){
                if (z[m] > max){ max=z[m]; }
            }

            // Экспоненцирование разности сырых логитов 
            // и максимального значения - 
            // СТАБИЛИЗАЦИЯ и подготовка к softmax
            double sum=0.0;
            for (int m=0; m<10; m++){
                yy[m]=exp(z[m] - max);
                sum+=yy[m];
            }

            // А это уже самый настоящий softmax (нормализация)
            for (int m=0; m<10; m++){ yy[m]/=sum; }

            // Распознаём
            double maxx=0.0;
            for (int k=0; k<10; k++){
                if (yy[k]>maxx){ maxx=yy[k]; }
            }


            for (int k=0; k<10; k++){
                if (yy[k]==maxx){
                    if (k==label){ tochnost+=1.0; }
                    if (j==700 || j==200){
                        printer(label, height, weight, x);
                        printf("Это цифра: %d\n\n", k);
                    }
                }
            }
        }
    }
    for (int i=0; i<10; i++){ summa+=sum[i]; }
    printf("Точность: %f%%\n", tochnost/summa*100);
}


int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "");
    srand(time(NULL));
    int knopka;
    time_t start, end;
    double elapsed_time;

    do{
        printf("\n\nВведите команду:\n");
        printf("1 - Загрузить датасет в бинарный файл.\n");
        printf("2 - Фильтрация.\n");
        printf("3 - Пулинг.\n");
        printf("4 - Рассчитать весовые коэффициенты.\n");
        printf("5 - Обучить модель.\n");
        printf("6 - Загрузить тестовые изображения.\n");
        printf("7 - Отфильтровать тестовые изображения.\n");
        printf("8 - Отпуллинговать тестовые изображения.\n");
        printf("9 - Распознать тестовые изображения.\n");
        printf("-1 - Выйти из программы.\n");
        scanf_s("%d", &knopka);
        switch (knopka){
        case (1): {
            start=time(NULL);
            bin_data();

            end=time(NULL);
            elapsed_time=difftime(end, start);
            printf("Время работы программы: %f секунд\n", elapsed_time);
            break;
        }
        case (2):{
            apply_filter_to_all();
            break;
        }
        case (3):{
            pooling();
            break;
        }
        case(4):{
            start=time(NULL);
            weights();
            end=time(NULL);
            elapsed_time=difftime(end, start);
            printf("Время работы программы: %f секунд\n", elapsed_time);
            break;
        }
        case(5):{
            start=time(NULL);
            learning();
            end=time(NULL);
            elapsed_time=difftime(end, start);
            printf("Время работы программы: %f секунд\n", elapsed_time);
            break;
        }
        case(6):{
            bin_data_test();
            break;
        }
        case(7):{
            apply_filter_to_all_test();
            break;
        }
        case(8):{
            pooling_test();
            break;
        }
        case(9):{
            testing();
            break;
        }
        }
    } while (knopka!=-1);

    printf("Программа завершена.");
    return 0;
}

//
//  main.c
//  First_PVR
//
//  Created by Андрей Решетников on 14.09.15.
//  Copyright (c) 2015 mipt. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <omp.h>

struct Complex {
    float real;
    float imaginary;
};

float complex_radius(struct Complex x) {
    return sqrt(x.real * x.real + x.imaginary * x.imaginary);
}

void read_number(struct Complex *x) {
    scanf("%f %f", &x->real, &x->imaginary);
}

void print_number(struct Complex x) {
    printf("%f %f ", x.real, x.imaginary);
}

struct Matrix {
    struct Complex** cell;
    int size;
};

void read_matrix(struct Matrix *x) {
    scanf("%d", &x->size);
    x->cell = malloc(x->size * sizeof(struct Complex*));
    int i, j;
    for(i = 0; i < x->size; i++) {
        x->cell[i] = malloc(x->size * sizeof(struct Complex));
    }
    for(i = 0; i < x->size; i++)
        for (j = 0; j < x->size; j++) {
            scanf("%f %f", &x->cell[i][j].real, &x->cell[i][j].imaginary);
        }
}

void random_matrix(struct Matrix *x, int y) {
    //random(x->size)
    x->size = y;
    x->cell = malloc(x->size * sizeof(struct Complex*));
    int i, j;
    for(i = 0; i < x->size; i++) {
        x->cell[i] = malloc(x->size * sizeof(struct Complex));
    }
    for(i = 0; i < x->size; i++)
        for (j = 0; j < x->size; j++) {
            x->cell[i][j].real = rand() % 100;
            x->cell[i][j].imaginary = rand() % 100;
        }
}

void print_matrix(struct Matrix x) {
    int i, j;
    for(i = 0; i < x.size; i++) {
        for(j = 0; j < x.size; j++) {
            printf("%0.1f %0.1f   ", x.cell[i][j].real, x.cell[i][j].imaginary);
        }
        printf("\n");
    }
}

void destroy_matrix(struct Matrix *x) {
    int i;
    for (i = 0; i < x->size; i++) {
        free(x->cell[i]);
    }
    free(x->cell);
}

struct Index {
    int column;
    int line;
};

void find_min_max_sequential(struct Matrix x, struct Index *min, int *index_min, struct Index *max, int *index_max) {
    float min_rad = INT_MAX, max_rad = INT_MIN;
    int i, j;
    for (i = 0; i < x.size; i++)
        for (j = 0; j < x.size; j++) {
            if (complex_radius(x.cell[i][j]) < min_rad) {
                min_rad = complex_radius(x.cell[i][j]);
            }
            if (complex_radius(x.cell[i][j]) > max_rad) {
                max_rad = complex_radius(x.cell[i][j]);
            }
        }
    for (i = 0; i < x.size; i++)
        for (j = 0; j < x.size; j++) {
            if (complex_radius(x.cell[i][j]) == min_rad) {
                ((min)[*index_min]).column = i;
                ((min)[*index_min]).line = j;
                (*index_min)++;
            }
            if (complex_radius(x.cell[i][j]) == max_rad) {
                max[*index_max].column = i;
                max[*index_max].line = j;
                (*index_max)++;
            }
        }
}

struct Data_Of_Thread {
    int id, thread_number, size;
    struct Matrix array;
    struct Index thread_min, thread_max;
    float min, max;
};

void *thread_work(void *arguments) {
    struct Data_Of_Thread *x;
    x = arguments;
    float min_rad = INT_MAX, max_rad = INT_MIN;
    int i, j, k;
    k = x->id;
    while (k < x->size * x->size) {
        i = k / x->size;
        j = k % x->size;
            if (complex_radius(x->array.cell[i][j]) < min_rad) {
                min_rad = complex_radius(x->array.cell[i][j]);
                x->min = min_rad;
                x->thread_min.column = i;
                x->thread_min.line = j;
            }
            if (complex_radius(x->array.cell[i][j]) > max_rad) {
                max_rad = complex_radius(x->array.cell[i][j]);
                x->max = max_rad;
                x->thread_max.column = i;
                x->thread_max.line = j;
            }
        k += x->thread_number;
    }
    //printf("Id: %d %f %f\n",x->id, x->min, x->max);
    return NULL;
}

void find_min_max_parallel(int thread_number, struct Matrix x, float *min, struct Index *index_min,
                                                               float *max, struct Index *index_max, int size ) {
    pthread_t *threads;
    struct Data_Of_Thread *current_arg;
    threads = malloc(thread_number * sizeof(pthread_t));
    current_arg = malloc(thread_number * sizeof(struct Data_Of_Thread));
    int id;
    for (id = 0; id < thread_number; id++) {
        current_arg[id].id = id;
        current_arg[id].array = x;
        current_arg[id].size = size;
        current_arg[id].thread_number = thread_number;
        current_arg[id].max = INT_MIN;
        current_arg[id].min = INT_MAX;
        pthread_create(&threads[id], NULL, thread_work, &current_arg[id]);
    }
    
    for (id = 0; id < thread_number; id++) {
        pthread_join(threads[id], NULL);
    }
    
    
    /*int i;
    for (i = 0; i < thread_number; i++) {
        printf("%f %f\n", current_arg[i].max, current_arg[i].min);
        printf("Index maximum: %d %d\n", current_arg[i].thread_max.column, current_arg[i].thread_max.line);
        //printf("Index minimum: %d %d\n", parallel_index_min.column, parallel_index_min.line);
    }*/
    
    int i;
    float global_min = current_arg[0].min, global_max = current_arg[0].max;
    struct Index global_index_min = current_arg[0].thread_min, global_index_max = current_arg[0].thread_max;
    for (i = 1; i < thread_number; i++) {
        if (global_min > current_arg[i].min) {
            global_min = current_arg[i].min;
            global_index_min = current_arg[i].thread_min;
        }
        if (global_max < current_arg[i].max) {
            global_max = current_arg[i].max;
            global_index_max = current_arg[i].thread_max;
        }
    }
    
    *min = global_min;
    *max = global_max;
    *index_min = global_index_min;
    *index_max = global_index_max;
    free(threads);
}

void find_min_max_omp(int thread_number, struct Matrix x, float *min, struct Index *index_min,
                                                          float *max, struct Index *index_max) {
    float shared_min = INT_MAX, shared_max = INT_MIN;
    struct Index shared_index_min, shared_index_max;
    
    #pragma omp parallel num_threads(thread_number) 
    {
        float private_min = INT_MAX, private_max = INT_MIN;
        struct Index private_index_min, private_index_max;
        int i, j;
        #pragma omp for nowait
        for (i = 0; i < x.size; i++) {
            for (j = 0; j < x.size; j++)
                if (complex_radius(x.cell[i][j]) > private_max) {
                    private_max = complex_radius(x.cell[i][j]);
                    private_index_max.column = i;
                    private_index_max.line = j;
                }
                if (complex_radius(x.cell[i][j]) < private_min) {
                    private_min = complex_radius(x.cell[i][j]);
                    private_index_min.column = i;
                    private_index_min.line = j;
                }
        }
    
        #pragma omp critical 
        {
            if (private_max > shared_max) {
                shared_max = private_max;
                shared_index_max = private_index_max;
            }
            if (private_min < shared_min) {
                shared_min = private_min;
                shared_index_min = private_index_min;
            }
        }
    }
    
    *min = shared_min;
    *max = shared_max;
    *index_max = shared_index_max;
    *index_min = shared_index_min;
}

int main(int argc, const char * argv[]) {
    struct Matrix x;
    //read_matrix(&x);
    random_matrix(&x, 100);
    //print_matrix(x);
    printf("\n");
    
    struct Index *min, *max;
    int index_min = 0, index_max = 0;
    
    min = malloc(x.size * x.size * sizeof(struct Index));
    max = malloc(x.size * x.size * sizeof(struct Index));
    
    
    //-----------------------CLOCK---------------------------
    /*
    clock_t t;
    t = clock();
    */
    //-----------------------SEQUENTIAL---------------------------
    
    /*find_min_max_sequential(x, min, &index_min, max, &index_max);
    int i;*/
    /*printf("Min indexes: \n");
     for (i = 0; i < index_min; i++) {
     printf("%d %d\n", min[i].column, min[i].line);
     }
     
     printf("\nMax indexes: \n");
     for (i = 0; i < index_max; i++) {
     printf("%d %d\n", max[i].column, max[i].line);
     }*/
    
    //-------------------------Parallel-----------------------------
    /*
    float parallel_min, parallel_max;
    struct Index parallel_index_min, parallel_index_max;
    int thread_number = 32;
    find_min_max_parallel(thread_number, x, &parallel_min, &parallel_index_min, &parallel_max, &parallel_index_max, x.size);
    
    //printf("Minimum: %f\nMaximum: %f\n", parallel_min, parallel_max);
    //printf("Index maximum: %d %d\n", parallel_index_max.column, parallel_index_max.line);
    //printf("Index minimum: %d %d\n", parallel_index_min.column, parallel_index_min.line);
    
    */
    
    //----------------------------CLOCK------------------------------
    /*t = clock() - t;
    printf ("It took me %d clicks (%f seconds).\n",
            (int)t, ((double)t)/CLOCKS_PER_SEC);
    */
    
    
    
    //-----------------------------OMP-------------------------
    int thread_number = 4;
    float parallel_min, parallel_max;
    struct Index parallel_index_min, parallel_index_max;
    find_min_max_omp(thread_number, x, &parallel_min, &parallel_index_min, &parallel_max, &parallel_index_max);
    
    printf("Minimum: %f\nMaximum: %f\n", parallel_min, parallel_max);
    printf("Index maximum: %d %d\n", parallel_index_max.column, parallel_index_max.line);
    printf("Index minimum: %d %d\n", parallel_index_min.column, parallel_index_min.line);

    
    destroy_matrix(&x);
    return 0;
}
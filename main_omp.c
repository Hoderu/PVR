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
//#include <mpi.h>

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
    srand(time(0));
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
            for (j = 0; j < x.size; j++) {
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
    //random_matrix(&x, 10);
    //print_matrix(x);
    printf("\n");
    
    
    int i, j;
    struct Index *min, *max;
    int index_min = 0, index_max = 0;
    
    min = malloc(x.size * x.size * sizeof(struct Index));
    max = malloc(x.size * x.size * sizeof(struct Index));
    
    
    //-----------------------CLOCK---------------------------
    /*
    clock_t t;
    t = clock();
    */
 
    float parallel_min, parallel_max;
    struct Index parallel_index_min, parallel_index_max;
    int sizes_matrix[7] = {3,10,100,500,1000,3000,5000};
    int count_matrix = 7;
    int count_threads = 9;
    int thread_array[9] = {1, 2, 3, 4, 6, 8, 16, 24, 32};
    float thread_number;
    double time[count_matrix][count_threads];
    
    for (i = 0; i < count_matrix; i++)
        for (j = 0; j < count_threads; j++)
            time[i][j] = 0;
    
    clock_t t;
    //random_matrix(&x, 10);
    for (i = 0; i < count_matrix; i++)
        for (j = 0; j < count_threads; j++) {
            random_matrix(&x, sizes_matrix[i]);
            thread_number = thread_array[j];
            t = clock();
            find_min_max_omp(thread_number, x, &parallel_min, &parallel_index_min, &parallel_max, &parallel_index_max);
            t = clock() - t;
            double t_f = ((double)t)/CLOCKS_PER_SEC;
            time[i][j] = t_f;
    }
    
    
    /*printf("Min indexes: \n");
     for (i = 0; i < index_min; i++) {
     printf("%d %d\n", min[i].column, min[i].line);
     }
     
     printf("\nMax indexes: \n");
     for (i = 0; i < index_max; i++) {
     printf("%d %d\n", max[i].column, max[i].line);
     }*/
    
    FILE *graph = fopen("/Users/Hoderu/graph.html", "w+");
    fprintf(graph, "");
    fprintf(graph, "<!doctype html>\n<html>\n<head>\n<title>Dependent threads on size</title>\n"
            "<script src=\"Chart.js\"></script>\n</head>\n<body>\n");
    fprintf(graph, "\t<div style=\"width:100%\">\n<table style=\"width:50%\">\n");
    for (j = 0; j < count_matrix; ++j) {
        fprintf(graph,
                "<tr>\n<td style=\"width:50%\">\n<p>Matrix: %dx%d</p>\n"
                "<canvas id=\"canvas%d\" height=\"450\" width=\"600\"></canvas>\n</td>\n"
                "</tr>\n", sizes_matrix[j], sizes_matrix[j], j);
    }
    fprintf(graph,"</table>\n</div>\n\n<script>\n");
    for (j = 0; j < count_matrix; ++j) {
        fprintf(graph, "\n\t\tvar data%d = {\nlabels : [", j);
        for (i = 0; i < count_threads; ++i) {
            if (i > 0) {
                fprintf(graph, ",");
            }
            fprintf(graph, "\"%d\"", thread_array[i]);
        }
        fprintf(graph, "],\ndatasets : [\n{\nlabel: \"Time\",\nfillColor : \"rgba(0,0,220,0.2)\",\n"
                "strokeColor : \"rgba(0,0,220,1)\",\npointColor : \"rgba(0,0,220,1)\",\npointStrokeColor :\"#00f\",\n"
                "pointHighlightFill : \"#f00\",\npointHighlightStroke : \"#f00\",\ndata : [");
        for (i = 0; i < count_threads; ++i) {
            if (i > 0) {
                fprintf(graph, ",");
            }
            fprintf(graph, "%f", time[j][i]);
        }
        fprintf(graph, "]\n\t\t\t\t}\n]\n}");
    }
    fprintf(graph,"\n\twindow.onload = function(){\n");
    for (i = 0; i < count_matrix; ++i) {
        fprintf(graph, "var ctx = document.getElementById(\"canvas%d\").getContext(\"2d\");\n"
                "window.myLine = new Chart(ctx).Line(data%d, {\nresponsive: true,\n"
                "scaleBeginAtZero : true\n});\n", i, i);
    }
    fprintf(graph, "\t}\n\n\n\t</script>\n</body>\n</html>");
    fclose(graph);
    
    
    destroy_matrix(&x);
    return 0;
}
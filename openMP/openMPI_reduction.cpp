#include<stdio.h>
#include<stdlib.h>
//#include<iostream>
#include<omp.h>

double under_integral_func(double x) {
    return 1 / (1 + x*x);
}

int main(int argc, char* argv[]){

    int num_threads;
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        omp_set_num_threads(num_threads);
    } else {
        num_threads = 1;
        omp_set_num_threads(1);
    }

    double integral = 0;
    double sum = 0;

    double timestamp1 = omp_get_wtime();

#pragma omp parallel reduction(+:sum)
    {
        double integral_part = 0;

#pragma omp for
        for(int i = 1; i < 100000000; ++i) {
            double a, b;
            a = 1. * i / 100000000;
            b = 1. * (i + 1) / 100000000;

            integral_part += 4. * (under_integral_func(b) + under_integral_func(a)) / 2;

        }

        sum += integral_part;
    }
    integral = sum / 100000000;

    double timestamp2 = omp_get_wtime();
    double time = timestamp2 - timestamp1;

    printf("Integral: %f\n", integral);
    printf("Number of threads: %d\n", num_threads);
    printf("Time: %f\n", time);

    return 0;
}
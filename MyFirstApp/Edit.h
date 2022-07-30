#pragma once
extern "C" {
#include <stdio.h>
#include <stdlib.h>
    float* edit(FILE* fp);
    void save(FILE* fr, double dt);
    int JJlength();
    int SampInt();
    float trapJJ(int datatype,double dt, double T, double V, int s, int p);
    void scan(FILE* fp, double* dt, double* T, double* V);
}
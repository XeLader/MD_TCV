#include <stdio.h>
#include <stdlib.h>
#include <string.h>

    float *JJx, *JJy, *JJz, *JJave;
    int r, N, d, K = 0;
    float x, y, z, maxVis = 0.0, maxTC = 0.0;

    float * edit(FILE* fp, int axis)
    {

        char str[100];
        

        fgets(str, 100, fp);
        fgets(str, 100, fp);
        fgets(str, 100, fp);
        fscanf_s(fp, "%d %d\n", &r, &N);
        JJx = (float*)calloc(N, sizeof(float));
        JJy = (float*)calloc(N, sizeof(float));
        JJz = (float*)calloc(N, sizeof(float));
        JJave = (float*)calloc(N, sizeof(float));
        fscanf_s(fp, "%d %d %d %f %f %f\n", &r, &r, &r, JJx, JJy, JJz);
        fscanf_s(fp, "%d %d %d %f %f %f\n", &r, &d, &r, JJx+1, JJy+1, JJz+1);
        for (int i = 2; i < N; i++) {
            fscanf_s(fp, "%d %d %d %f %f %f\n", &r, &r, &r, JJx + i, JJy + i, JJz + i);
        }
        while (fgets(str, 100, fp) != NULL) {
            for (int i = 0; i < N; i++) {
                fscanf_s(fp, "%d %d %d %f %f %f\n", &r, &r, &r, JJx+i, JJy+i, JJz+i);
            }
        }
        fclose(fp);
        switch (axis) {
        case 1:
            return(JJx);
        case 2:
            return(JJy);
        case 3:
            return(JJz);
        case 4:
        {
            for (int i = 0; i < N; i++) {
                JJave[i] = (JJx[i] + JJy[i] + JJz[i]) / 3;
            }
            return(JJave);
        }
        }
        


    }

    void save(FILE* fr, double dt)
    {
        char e[20];
        if (dt == 0.0) {
            fprintf(fr, "N JJx JJx/JJ[0] JJy JJy/JJ[0] JJz JJz/JJ[0] JJave JJave/JJ[0]\n");
            for (int i = 0; i < N; i++)
                fprintf(fr, "%d %f %f %f %f %f %f %f %f\n", i * d, JJx[i], JJx[i] / JJx[0], JJy[i], JJy[i] / JJy[0], JJz[i], JJz[i] / JJz[0], JJave[i], JJave[i] / JJave[0]);
        }
        else {
            fprintf(fr, "N time JJx JJx/JJ[0] JJy JJy/JJ[0] JJz JJz/JJ[0] JJave JJave/JJ[0]\n");
            for (int i = 0; i < N; i++)
                fprintf(fr, "%d %f %f %f %f %f %f %f %f %f\n", i * d,dt*i*d, JJx[i], JJx[i] / JJx[0], JJy[i], JJy[i] / JJy[0], JJz[i], JJz[i] / JJz[0], JJave[i], JJave[i] / JJave[0]);
        }
        fclose(fr);
    }

    float* trapJJ(float* Cor,int datatype, double dt, double T, double V, int s, int p) {
        float * traparray = (float*)calloc(p, sizeof(float));
        const float kB = 1.3806504e-23, kCal2J = 4186.0 / 6.02214e23,
            atm2Pa = 101325.0, A2m = 1.0e-10, fs2s = 1.0e-15;
        float scale;
        if (datatype) {
            
            const float convert = kCal2J * kCal2J / fs2s / A2m;
            scale = convert / kB / T / T / V * s * dt;
        }
        else {
            const float convert = atm2Pa * atm2Pa * fs2s * A2m * A2m * A2m;
            scale = convert / (kB * T) * V * s * dt;
        }
        traparray[0] = Cor[0]*scale;
        float maxval = 0;
        for (int i = 1; i <= p - 1; i++) {
            traparray[i] = Cor[i]*scale + traparray[i-1];
            maxval = (traparray[i] > maxval) ? traparray[i] : maxval;
        }
        if (datatype)
            maxTC = maxval;
        else
            maxVis = maxval;
        return(traparray);
    }

    int JJlength() { 
        return(N);
    }
    int SampInt() {
        return(d);
    }

    float MaxVis()
    {
        return(maxVis);
    }

    float MaxTC()
    {
        return(maxTC);
    }
  
    void scan(FILE* fp, double* dt, double* T, double* V, int* St, double** Tab)
    {
        char str[300];
        const char vel[] = "velocity";
        const char ts[] = "timestep";
        const char vol[] = "Volume";
        const char cr[] = "create ";
        const char fx[] = "fix";
        const char NVE[] = "NVE";
        const char NPT[] = "NPT_fix";
        const char Per[] = "Per MPI rank";
        const char step[] = "Step";
        const char press[] = "Press";
        const char temp[] = "Temp";
        const char dens[] = "Density";
        const char loop[] = "Loop";


        int start=0, end=0, isNVE=0, isNPT=0, n=0;
        int position[5];
        char dtstr[5];
        char Tstr[6];
        char Vstr[32];

        double table[5][200];

        while (fgets(str, 300, fp) != NULL) {
            if (strncmp(str, ts, 8)==0)
            {
                start = 8;
                while (isspace(str[start]))
                    start++;
                if (str[start] == '$')
                    fgets(str, 300, fp);
                end = 0;
                while (isdigit(str[start+end]))
                        end++;
                strncpy_s(dtstr,5, &str[start], end);
                *dt = atof(dtstr);         
            }
            if (strncmp(str, vel, 8) == 0)
            {
                again:
                start = 8;
                while (strncmp(str+start, cr, 7))
                    start++;
                start += 7;
                if (str[start] == '$') {
                    fgets(str, 300, fp);
                    goto again;
                }    
                end = 0;
                while (isdigit(str[start + end]))
                    end++;
                strncpy_s(Tstr, 6, str+start, end);
                *T = atof(Tstr);
            }
            if (strncmp(str, fx, 3) == 0)
            {
            
                start = 3;
                while (isspace(str[start]))
                    start++;
                if (strncmp(str + start, NVE, 3) == 0)
                {
                    isNVE = 1;
                    isNPT = 0;
                }
                else if (strncmp(str + start, NPT, 7) == 0)
                {
                    isNVE = 0;
                    isNPT = 1;
                }
                else {
                    isNVE = 0;
                    isNPT = 0;
                }
                
            }
            if (strncmp(str, Per, 12) == 0) {
                if (isNVE) {
                    start = 0;
                    int Nspace = 0;
                    fgets(str, 300, fp);
                    while (strncmp(str + start, vol, 6) != 0) {
                        if (isspace(str[start]))
                            Nspace++;
                        start++;
                    }
                    start = 0;
                    fgets(str, 300, fp);
                    while (isspace(str[start]))
                        start++;
                    for (int i = 0; i < Nspace; i++) {
                        while (!isspace(str[start]))
                            start++;
                        while (isspace(str[start]))
                            start++;

                    }
                    end = 1;
                    while (!isspace(str[start + end]))
                        end++;
                    strncpy_s(Vstr, 32, str + start, end + 1);
                    *V = atof(Vstr);
                }
                else if (isNPT) {
                    start = 0;
                    end = 0;
                    int Nspace = 0;
                    fgets(str, 300, fp);
                    while (str[start]!='\n') {
                        if (isspace(str[start]))
                        {
                            Nspace++;
                            start++;
                        }
                        while (!isspace(str[start+end]))
                        {
                            end++;
                        }
                        if (end != 0) {
                            if (strncmp(str + start, step, end) == 0)
                                position[0] = Nspace;
                            if (strncmp(str + start, press, end) == 0)
                                position[1] = Nspace;
                            if (strncmp(str + start, temp, end) == 0)
                                position[2] = Nspace;
                            if (strncmp(str + start, vol, end) == 0)
                                position[3] = Nspace;
                            if (strncmp(str + start, dens, end) == 0)
                                position[4] = Nspace;
                            
                        }
                        start = start + end;
                        end = 0;
                    }
                    start = 0;
                    fgets(str, 300, fp);
         
                    while (strncmp(str, loop, 4) != 0) {
                        for (int i = 0; i < Nspace; i++) {
                            while (isspace(str[start]))
                                start++;
                            while (!isspace(str[start + end]))
                                end++;
                            strncpy_s(Vstr, 32, str + start, end);
                            start = start + end;
                            end = 0;
                            for (int j = 0; j < 5; j++)
                                if (position[j] == i) {
                                    Tab[j][n] = atof(Vstr);
                                }
                            end = 0;
                            
                        }
                        start = 0;
                        fgets(str, 300, fp); n++;
                    }

                }
            }
            
                
        }
        *St = n;
        fclose(fp);
    }
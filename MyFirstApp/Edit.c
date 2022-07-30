#include <stdio.h>
#include <stdlib.h>
#include <string.h>

    float *JJx, *JJy, *JJz, *JJave;
    int r, N, d, K = 0;
    float x, y, z;

    float * edit(FILE* fp)
    {

        char str[100];
        

        fgets(str, 100, fp);
        printf(str);
        fgets(str, 100, fp);
        printf(str);
        fgets(str, 100, fp);
        printf(str);
        fscanf_s(fp, "%d %d\n", &r, &N);
        printf("%d %d\n", r, N);
        JJx = (float*)calloc(N, sizeof(float));
        JJy = (float*)calloc(N, sizeof(float));
        JJz = (float*)calloc(N, sizeof(float));
        JJave = (float*)calloc(N, sizeof(float));
        fgets(str, 100, fp);
        fscanf_s(fp, "%d %d", &r, &d);
        for (int i = 0; i < N - 1; i++)
            fgets(str, 100, fp);
        while (fgets(str, 100, fp) != NULL) {
            for (int i = 0; i < N; i++) {
                fscanf_s(fp, "%d %d %d %f %f %f\n", &r, &r, &r, JJx+i, JJy+i, JJz+i);
            }
        }
        fclose(fp);
        for (int i = 0; i < N; i++) {
            JJave[i] = (JJx[i]+JJy[i]+JJz[i])/3;
        }
        return(JJave);
    }

    void save(FILE* fr, double dt)
    {
        char e[20];
        //strcpy(e, "edit_");
        //strcat(e, name);
        //printf(e);
        //fopen_s(&fr, e, "w");
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

    float trapJJ(int datatype, double dt, double T, double V, int s, int p) {
        const float kB = 1.3806504e-23, kCal2J = 4186.0 / 6.02214e23,
            atm2Pa = 101325.0, A2m = 1.0e-10, fs2s = 1.0e-15;
        float scale;
        if (datatype) {
            const float convert = atm2Pa * atm2Pa * fs2s * A2m * A2m * A2m;
            scale = convert / (kB * T) * V * s * dt;
        }
        else {
            const float convert = kCal2J * kCal2J / fs2s / A2m;
            scale = convert / kB / T / T / V * s * dt;
        }
        float trap = 0;
        for (int i = 1; i < p - 1; i++) {
            trap += JJave[i];
        }

        trap += JJave[0] / 2 + JJave[p - 1] / 2;
        return(trap* scale);
    }

    int JJlength() { 
        return(N);
    }
    int SampInt() {
        return(d);
    }
  
    void scan(FILE* fp, double* dt, double* T, double* V)
    {
        char str[300];
        const char vel[] = "velocity";
        const char ts[] = "timestep";
        const char vol[] = "Volume";
        const char cr[] = "create ";
        const char fx[] = "fix";
        const char NVE[] = "NVE";
        const char Per[] = "Per MPI rank";

        int start=0, end=0, isNVE=0;
        char dtstr[5];
        char Tstr[6];
        char Vstr[32];

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
                if (strncmp(str + start, NVE, 3)!=0) 
                    continue;
                isNVE++;
            }
            if (strncmp(str, Per, 12) == 0)
                if(isNVE){
                    start = 0;
                    int Nspace=0;
                    fgets(str, 300, fp);
                    while (strncmp(str+start, vol, 6)!=0) {
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
                        while(isspace(str[start]))
                            start++;

                    }
                    end = 1;
                    while (!isspace(str[start+end]))
                        end++;
                    strncpy_s(Vstr, 32, str + start, end+1);
                    *V = atof(Vstr);
            }
        }
        fclose(fp);
    }
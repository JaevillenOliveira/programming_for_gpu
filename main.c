#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {
    const long perimetro = 100;
    const int arr_size = 30000;

    int var_lim_a = (perimetro/3) - 1; 
    int var_lim_b = (perimetro/3);
    int var_lim_c = var_lim_b + (perimetro - var_lim_b * 3) + 1;

    int a[arr_size], b[arr_size], c[arr_size];

    clock_t start, end;
    FILE *fptr;

	fptr = fopen("pythagoreanTriplets.txt","w");
	fptr = freopen("pythagoreanTriplets.txt","a", fptr);

    start = clock();

    // 10 : 
    // lim: a = 2 b = 3 c = 5
    // a = [1 1 1 1 1 2 2]  
    // b = [2 2 2 3 3 3 3]
    // c = [3 4 5 4 5 4 5]
    // r = []

    // 10 : 
    // lim: a = 2 b = 3 c = 5
    // a = [1 1 2]  
    // b = [2 3 3]

    int i, j, k, res;
    long idx = 0;
    for (i = 1; i <= var_lim_a; i++){
        for (j = i+1; j <= var_lim_b; j++){
            for (k = j+1; k <= var_lim_c; k++){
                fprintf (fptr, "%d %d %d \n", i, j, k);
                idx++;
            }
        }
    }
    fptr = freopen("pythagoreanTriplets.txt","r", fptr);
    for (idx = 0; idx < arr_size; idx++){
        res = fscanf(fptr, "%d %d %d ", &a[idx], &b[idx], &c[idx]);        
        printf("%d %d %d \n", a[idx], b[idx], c[idx]);
        if(res == -1)
            break;
    }


/*     for (a = 1; a <= var_lim - 1; a++){
        for (b = a+1; b <= var_lim; b++){
            for (c = b+1; c <= var_lim + (perimetro - var_lim * 3) + 1; c++){

                if((a*a + b*b) == (c*c)){
                    fprintf (fptr, "%d %d %d \n", a, b, c);
                }
            }
        }
    } */
    fclose(fptr);

    end = clock();
    double time_taken = ((double)end - start)/CLOCKS_PER_SEC;

    FILE *timeFile;
    timeFile = fopen("timelog.txt","w");

    char lineToWrite[50], timetkstr[20];
    sprintf (timetkstr, "%f", time_taken);

    strcpy(lineToWrite, "    Time: ");
    strcat(lineToWrite, timetkstr);
    
    //write the line to the file
    fputs(lineToWrite, timeFile);
    fclose(timeFile);
}
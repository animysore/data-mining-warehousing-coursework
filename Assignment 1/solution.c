#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINELENGTH 1024

typedef struct Day {
    double open, high, low, close, turnover;
    long int shares;
    char date[15];
    //int day,month,year;
} Day;

// parses CSV data file and returns array of opening values 
Day* parseCsv(char* filename, int *n) {
    FILE *datafile = fopen(filename,"r"); // read file 
    char line[MAXLINELENGTH];  //holds each line read
    int numberOfLines =0;

    // count number of lines in file
    while (fgets(line,MAXLINELENGTH,datafile)) { numberOfLines++; }

    if (numberOfLines<2) { 
        printf("No data! First line needs to be header line\n");
        exit(-1);
    }

    Day *D = (Day*) malloc(sizeof(Day)*numberOfLines-1);
    int c = 0;
    
    rewind(datafile);
    //read file line by line till EOF
    fgets(line,MAXLINELENGTH,datafile); //ignore header line 
    while (fgets(line,MAXLINELENGTH,datafile)) {
        // use strtok to split each line with delimiter
        char* token;
        // field 0 - date
        token = strtok(line,",");
        strcpy(D[c].date,token);
        // field 1 - open
        token = strtok(NULL,",");
        D[c].open = atof(token);
        // field 2 - high
        token = strtok(NULL,",");
        D[c].high = atof(token);
        // field 3 - low
        token = strtok(NULL,",");
        D[c].low = atof(token);
        // field 4 - close
        token = strtok(NULL,",");
        D[c].close = atof(token);
        // field 5 - shares
        token = strtok(NULL,",");
        D[c].shares = atoi(token);
        // field 6 - turnover
        token = strtok(NULL,",");
        D[c++].turnover = atof(token);
    }
    // store length 
    *n = c;
    return D;
}
double normalize_open(double value) { 
    const double max = 15304.0, min = 14332.65;
    return (value-min)/(max-min);
}
void plot(double *x, double *y, int n, char * legend) {
    int i;//
    FILE *gnuplot = popen("gnuplot -p","w");
    fprintf(gnuplot,"set title \"C does Graphs!\" \nset key left top \n");
    fprintf(gnuplot,"plot '-'title \"%s\" w linespoints\n",legend);
    for (i=0; i<n; i++) { 
        fprintf(gnuplot,"%g %g\n",x[i],y[i]);
    }
    fprintf(gnuplot,"e\n");
    fflush(gnuplot);
}

void plot_multiple_lines(double *x, double *y, int datapoints, int lines, char * legend[]) {
    int i,j;//
    FILE *gnuplot = popen("gnuplot -p","w");
    fprintf(gnuplot,"set title \"C does Graphs!\" \nset key left top \n plot ");
    for (i=0;i<lines;i++) { 
        fprintf(gnuplot," %s using 1:2 title \"%s\"  w linespoints ",(i==0)?"'-'":"''", legend[i]);
    }
    for (i=0;i<lines;i++) {         
        for (j=0; j<datapoints; j++) { 
            fprintf(gnuplot,"%g %g\n",x[j],y[i*lines + j]);
        }
        fprintf(gnuplot,"e\n");
    }
    fflush(gnuplot);
}
void drawLine(Day *D, int n, int a_0, int a_1) { 
    int c = n;
    double *x = (double*) malloc(sizeof(double)*(n+1)); 
    double *y = (double*) malloc(sizeof(double)*(n+1)*2); 
    while (c>=-1){ 
        x[c] = c;
        y[c] = normalize_open(D[c].open);
        y[n+c] = a_0 + a_1*c;
        c--;
    }
    printf("drawinf\n");
    char *legend[2];
    legend[0] = "Actual";
    legend[1] = "Predicted";
    plot_multiple_lines(x,y,n,2,legend);
}
void predictWithRegression(Day* D, int n) { 
    int EPOCHS = 0;
    float ALPHA=0.0002;
    double a_0, a_1, x, y, error, errorSum, errorIntoXValSum, meanSquareError, epoch_record[100], errors[100]; 
    
    a_0 = 0.1;
    a_1 = 0.1;

    while(EPOCHS<1000) { 
        int i;
        meanSquareError = 0;
        errorSum = 0;
        errorIntoXValSum = 0;
        for (i=0; i<n; i++) {
            x = i;
            y = a_0 + a_1*i;       
            error = y- normalize_open(D[i].open);
            errorSum += error;
            errorIntoXValSum += error*x;
            meanSquareError += error*error;
            meanSquareError /= n;
        }
        for (i=0; i<n; i++) {
            a_0 -= ALPHA*2*errorSum/n;
            a_1 -= ALPHA*2*errorIntoXValSum/n;
        }
        EPOCHS++;
        if(EPOCHS%10==0){ 
            //printf("%lf\n",meanSquareError);
            errors[EPOCHS/10] = meanSquareError;
            epoch_record[EPOCHS/10] = EPOCHS;
        }
    }
    printf("drawing\n");
    
    drawLine(D,n,a_0,a_1);
    plot(epoch_record,errors,100,"mean sq error");
};
void printData(Day* D, int n) {
    int c = n;
    double *x = (double*) malloc(sizeof(double)*(c+1));
    double *y = (double*) malloc(sizeof(double)*(c+1));
    while(c>-1) {
        x[c] = c;
        y[c] = D[c].open;
        printf("%s\t%lf\t%lf\t%lf\t%lf\t%lu\t%lf\n",\
        D[c].date,D[c].open,D[c].high,D[c].low,D[c].close,D[c].shares,D[c].turnover);
        c--;
    }
    plot(x,y,n,"open day");
}
int main(int argc, char* argv[]) { 
    if(argc != 2) { 
        printf("USAGE ./a.out datafile.csv\n");
        return -1;
    }
    int numberOfRecords;
    Day *D = parseCsv(argv[1],&numberOfRecords);
    //printData(D,numberOfRecords);
    predictWithRegression(D,numberOfRecords);
    return 0;
}
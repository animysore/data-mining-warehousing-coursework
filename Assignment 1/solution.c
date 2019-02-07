#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINELENGTH 1024
#define MAX_OPEN 15304.0
#define MIN_OPEN 14332.65

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

// returns normalized value of a field, given max and min
double normalize(double value, double max, double min) { 
    return (value-min)/(max-min);
}

// returns inverse of normalization function, given original max and min
double inv_normalize(double value, double max, double min) { 
    return value*(max-min)+min;
}

// plot a graph using gnuplot. can plot multiple lines in same graph 
void plot(double *x, double *y, int datapoints, int lines, char * legend[], char * title) {
    int i,j;//
    FILE *gnuplot = popen("gnuplot -p","w");
    fprintf(gnuplot,"set title \"%s\" \nset key left top \n plot ",title);
    for (i=0;i<lines;i++) { 
        fprintf(gnuplot,"%s using 1:2 title \"%s\"  w linespoints ",(i==0)?"'-'":", ''", legend[i]);
    }
    fprintf(gnuplot, "\n");
    for (i=0;i<lines;i++) {         
        for (j=0; j<datapoints; j++) {
            fprintf(gnuplot,"%g %g\n",x[j],y[i*datapoints + j]);
        }
        fprintf(gnuplot,"e\n");
    }
    fflush(gnuplot);
}

// draw the final regression line using the a_0 and a_1 parameters we calculated
void drawLine(Day *D, int n, double a_0, double a_1) { 
    int c = n-1;
    double *x = (double*) malloc(sizeof(double)*(n+1)); 
    double *y = (double*) malloc(sizeof(double)*(n+1)*2); 
    while (c>=0){ 
        x[c] = c;
        y[c] = D[c].open;
        y[n+c] = inv_normalize(a_0 + a_1*c,MAX_OPEN,MIN_OPEN);
        c--;
    }
    char *legend[2];
    legend[0] = "Actual";
    legend[1] = "Predicted";
    plot(x,y,n,2,legend,"Comparing final prediction for linear regression");
}

// run the lineear regression algorithm with mean square errors and gradient descent 
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
            error = y- normalize(D[i].open,MAX_OPEN,MIN_OPEN);
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
    printf("Constants: %lf %lf,\n",a_0,a_1);
    printf("Prediction for OPEN on Feb 01 is : %lf\n", inv_normalize(a_0+a_1*23,MAX_OPEN,MIN_OPEN));
    printf("Prediction for OPEN on Feb 05 is : %lf\n", inv_normalize(a_0+a_1*28,MAX_OPEN,MIN_OPEN));
    drawLine(D,n,a_0,a_1);
    char * legend = "mean sq error";
    plot(epoch_record,errors,100,1,&legend,"Gradient descent visualization");
};

// display the entire dataset 
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
    char * legend = "Day Open value";
    plot(x,y,n,1,&legend,"Visualizing the data");
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
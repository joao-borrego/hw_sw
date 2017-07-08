/*
 * @file knn.c
 * @brief KNN classifier implementation using HW for calculating distances
 *
 * This program runs a KNN implementation with configurable parameter K.
 * The dataset must be uploaded to memory before execution.
 * Hardware Acceleration is used in the calculation of squared euclidean
 * distances between testing and training objects.
 */

#include <stdio.h>

#include <stdlib.h>
#include <math.h>

#include "data.h"

/** K-nearest neighbours parameter */
#define K 3

/** @brief Distance from test object A to trn object B, an label of B */
typedef struct DistLabelPair_Struct{
    float distance;   /**< Distance from a given test object to B */
    int label;        /**< Class label of B */
}DistLabelPair;

float distance(float* a, float* b, int size){
    
    int i;
    float diff;
    float sum = 0.0;
    for (i = 0; i < size; i++){
        diff = (a[i] - b[i]);
        sum += diff * diff;
    }
    return sum;
}

/**
 *  @brief Reads the dataset to memory 
 *
 */    
void readDataset(   float **trn_data,
                    float **tst_data,
                    int **trn_label,
                    int **tst_label){

    FILE *fp_trn_data, *fp_trn_label, *fp_tst_data, *fp_tst_label;

    float feature;
    int   label;

    float *tmp_trn_data = malloc(sizeof(float) * FEATURES * NUM_TRN_OBJ);
    float *tmp_tst_data = malloc(sizeof(float) * FEATURES * NUM_TST_OBJ);
    int *tmp_trn_label = malloc(sizeof(int) * NUM_TRN_OBJ);
    int *tmp_tst_label = malloc(sizeof(int) * NUM_TST_OBJ);
    
    int i, j;

    /* Open files for reading dataset */
    fp_trn_data  = fopen(TRN_DATA_BIN, "rb");
    fp_tst_data  = fopen(TST_DATA_BIN, "rb");
    fp_trn_label = fopen(TRN_LABEL_BIN, "rb");
    fp_tst_label = fopen(TST_LABEL_BIN, "rb");
    
    if (fp_trn_data == NULL ||
        fp_tst_data == NULL ||
        fp_trn_label == NULL ||
        fp_tst_label == NULL){
        printf("Error reading input files!\n");
        exit(-1);
    }

    /* Generate trn set from binary data files */
    for (i = 0; i < NUM_TRN_OBJ; i++){
        for (j = 0; j < FEATURES; j++){
            fread(&feature, sizeof(float), 1,  fp_trn_data);
            //printf("[%d] %f ", i, feature);
            tmp_trn_data[i*FEATURES + j] = feature;
        }
        fread(&label, sizeof(int), 1,  fp_trn_label);
        //printf("%d (%s)\n", label, label_strings[label]);
        tmp_trn_label[i] = label; 
    }

    for (i = 0; i < NUM_TST_OBJ; i++){
        for (j = 0; j < FEATURES; j++){
            fread(&feature, sizeof(float), 1,  fp_tst_data);
            //printf("[%d] %f ", i, feature);
            tmp_tst_data[i*FEATURES + j] = feature;
        }
        fread(&label, sizeof(int), 1,  fp_tst_label);
        //printf("%d (%s)\n", label, label_strings[label]);
        tmp_tst_label[i] = label;
    }

    fclose(fp_trn_data); fclose(fp_trn_label);
    fclose(fp_tst_data); fclose(fp_tst_label);
    
    *trn_data = tmp_trn_data;
    *tst_data = tmp_tst_data;
    *trn_label = tmp_trn_label;
    *tst_label = tmp_tst_label;
}

/** @brief Sorts an array of floats with Selection Sort for K iterations O(n*K)
 *
 * @param array Array of floats to be sorted
 * @param size The size of the array
 * @param k Number of iterations (sorted objects)
 * @return Void.
 */
void selectionSortK(DistLabelPair *array, int size, int k){
    int i, j;
    int min;
    float tmp_distance;
    int tmp_label;

    for (i = 0; i < k; i++){
        min = i;
        for (j = i + 1; j < size; j++){
            if (array[min].distance > array[j].distance){
                min = j;
            }
        }
        if (min != i){
            tmp_distance = array[i].distance; tmp_label = array[i].label;
            array[i].distance = array[min].distance; array[i].label = array[min].label;
            array[min].distance = tmp_distance; array[min].label = tmp_label;
        }
    }
}

/**
 * @brief main program
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success.
 */
int main(int argc, char** argv){

    int i,j;
    int correct = 0;    /**< Number of correctly classified objects */
    int votes[CLASSES]; /**< Array for storing the class of each K nearest neighbour */
    int assigned_label; /**< Label assigned to a single test object */
    float accuracy;     /**< correctly_classified / total */
   
    /* SW - Fill dataset arrays */
    float *data_trn;
    float *data_tst;
    int *label_trn;
    int *label_tst;  
    
    DistLabelPair **dist_label = malloc(sizeof(DistLabelPair*) * NUM_TST_OBJ);
    for (i = 0; i < NUM_TST_OBJ; i++){
        dist_label[i] = malloc(sizeof(DistLabelPair) * NUM_TRN_OBJ);
    }
    readDataset(&data_trn, &data_tst, &label_trn, &label_tst);    

    // DEBUG
    
    printf("\nTRAINING SET\n\n");
    for (i = 0; i < NUM_TRN_OBJ; i++){
        printf("trn: ");
        for (j= 0; j < FEATURES; j++){
            printf("%.2f ", data_trn[i*FEATURES + j]);
        }
        printf("%d (%s)\n", label_trn[i], label_strings[label_trn[i]]);
    }

    printf("\nTESTING SET\n\n");
    for (i = 0; i < NUM_TST_OBJ; i++){
        printf("tst: ");
        for (j= 0; j < FEATURES; j++){
            printf("%.2f ", data_tst[i*FEATURES + j]);
        }
        printf("%d (%s)\n", label_tst[i], label_strings[label_tst[i]]);
    }

    /* Calculate distance matrix */
    /* For object in testing set */
    for (i = 0; i < NUM_TST_OBJ; i++){
        for(j = 0; j < NUM_TRN_OBJ; j++){
        	dist_label[i][j].distance = distance( &(data_tst[i*FEATURES]), &(data_trn[j*FEATURES]), FEATURES);
        	dist_label[i][j].label = label_trn[j];
        	//printf("tst obj %d - distance to trn obj %d of class %d is %f \n", i, j, label_trn[j], dist_label[i][j].distance);
        }
    }

    /* From the distance matrix assign labels to testing objects */
    for (i = 0; i <  NUM_TST_OBJ; i++){

        selectionSortK((DistLabelPair*) (dist_label[i]), NUM_TRN_OBJ, K);
        for (j = 0; j < CLASSES; j++){
            votes[j] = 0;
        }

        for (j = 0; j < K; j++){
            votes[ dist_label[i][j].label ]++;
        }

        assigned_label = 0;
        int vote = 0;
        
        for (j = 0; j < CLASSES; j++){
            if (votes[j] > vote){
            	vote = votes[j];
                assigned_label = j;
            }
        }
        
        if (assigned_label == label_tst[i]){
            correct++;
        }
        printf("tst object %d assigned to class %d (%s)\n", i, assigned_label, label_strings[assigned_label]);
    }

    accuracy = (correct * 100.0)/NUM_TST_OBJ;
    printf("Total of %d correctly classified (%.2f%%)\n", correct, accuracy);

    return 0;
}

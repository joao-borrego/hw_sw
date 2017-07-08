#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gen_data_config.h"

int main (int argc, char** argv){

    FILE *trn_in, *tst_in;
    FILE *trn_data_out, *trn_label_out, *tst_data_out, *tst_label_out;

    float feature;
    int label;
    
    int i;
    char buffer[1024];

    /* Open files for reading and writing */
    trn_in = fopen(TRN_FILE, "r");
    tst_in = fopen(TST_FILE, "r");
    trn_data_out = fopen(TRN_DATA_BIN, "w");
    trn_label_out = fopen(TRN_LABEL_BIN, "w");
    tst_data_out = fopen(TST_DATA_BIN, "w");
    tst_label_out = fopen(TST_LABEL_BIN, "w");
    
    /* Generate trn set binary data files */
    printf("Generating training set binaries: %s and %s\n", TRN_DATA_BIN, TRN_LABEL_BIN);
    while(!feof(trn_in)){
        for (i = 0; i < FEATURES; i++){
            fscanf(trn_in, "%f,", &feature);
            printf("[%d] %f ", i, feature);
            fwrite(&feature, sizeof(float), 1, trn_data_out);
        }
        fscanf(trn_in, "%d\n", &label);
        printf("%d\n", label);
        fwrite(&label, sizeof(label), 1, trn_label_out);
    }

    /* Generate tst set binary data files */
    printf("Generating testing set binaries: %s and %s\n", TST_DATA_BIN, TST_LABEL_BIN);
    while(!feof(tst_in)){
        for (i = 0; i < FEATURES; i++){
            fscanf(tst_in, "%f,", &feature);
            printf("[%d] %f ", i, feature);
            fwrite(&feature, sizeof(float), 1, tst_data_out);
        }
        fscanf(tst_in, "%d\n", &label);
        printf("%d\n", label);
        fwrite(&label, sizeof(label), 1, tst_label_out);
    }

    fclose(trn_in); fclose(tst_in);
    fclose(trn_data_out); fclose(trn_label_out);
    fclose(tst_data_out); fclose(tst_label_out);

    return 0;
}
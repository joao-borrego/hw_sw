/*
 * @file data.h
 * @brief Dataset parameters
 */

#define FEATURES 4      /**< Number of features */
#define CLASSES 3       /**< Number of classes */
#define NUM_TRN_OBJ 100 /**< Number of training objects */
#define NUM_TST_OBJ 50  /**< Number of testing objects */

/** Memory addresses for binary files */
#define TRN_DATA_BASE_ADDR  (float*)    0x010000000 /**< Start of  training set feature vectors */
#define TST_DATA_BASE_ADDR  (float*)    0x011000000 /**< Start of  testing set feature vectors  */
#define TRN_LABEL_BASE_ADDR (int*)      0x012000000 /**< Start of  training set labels */
#define TST_LABEL_BASE_ADDR (int*)      0x013000000 /**< Start of  testing set labels  */

/** Dataset binary files names */
#define TRN_DATA_BIN "trn_data.bin"
#define TST_DATA_BIN "tst_data.bin"
#define TRN_LABEL_BIN "trn_label.bin"
#define TST_LABEL_BIN "tst_label.bin"

char label_strings[CLASSES][100] = { "Iris-setosa", "Iris-versicolor", "Iris-virginica" };

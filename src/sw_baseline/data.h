/*
 * @file data.h
 * @brief Dataset parameters
 */

/* Dataset Choice */

#define IRIS 1
//#define WINE 1

/* Dataset Properties */

/* Iris - https://archive.ics.uci.edu/ml/datasets/iris*/

#ifdef IRIS
/** Number of features */
#define FEATURES 4
/** Number of classes */
#define CLASSES 3
/** Number of training objects */
#define NUM_TRN_OBJ 100
/** Number of testing objects */
#define NUM_TST_OBJ 50
/** Size of a single feature in bytes (sp-float) */
#define SIZE_FEATURE 4
/** Label strings array */
char label_strings[CLASSES][100] = {
    "Iris-setosa", "Iris-versicolor", "Iris-virginica" };

#define TRN_DATA_BIN    "iris_trn_data.bin"
#define TST_DATA_BIN    "iris_tst_data.bin"
#define TRN_LABEL_BIN   "iris_trn_label.bin"
#define TST_LABEL_BIN   "iris_tst_label.bin"

#endif

/* Wine - https://archive.ics.uci.edu/ml/datasets/Wine+Quality */

/* 50/50 trn/tst split */

#ifdef WINE
#define FEATURES        12                      /**< Number of features */
#define CLASSES         2                       /**< Number of classes */
#define NUM_TRN_OBJ     3271                    /**< Number of training objects */
#define NUM_TST_OBJ     3226                    /**< Number of testing objects */
#define TRN_DATA_BIN    "wine_trn_data.bin"
#define TST_DATA_BIN    "wine_tst_data.bin"
#define TRN_LABEL_BIN   "wine_trn_label.bin"
#define TST_LABEL_BIN   "wine_tst_label.bin"
#define SIZE_FEATURE 4
char label_strings[CLASSES][100] = {
    "Red", "White" };
#endif
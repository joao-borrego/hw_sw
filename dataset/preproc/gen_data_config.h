
/**********************************************************
 * DATASET CHOICE
 *********************************************************/

/* PICK ONE! */

//#define IRIS 1
#define WINE 1

/**********************************************************
 * DATASET PROPERTIES
 *********************************************************/

/* Iris */

#ifdef IRIS
#define FEATURES        4                   /**< Number of features */
#define CLASSES         3                   /**< Number of classes */
#define NUM_TRN_OBJ     50                  /**< Number of training objects */
#define NUM_TST_OBJ     100                 /**< Number of testing objects */
#define TRN_FILE        "trn.data"          /**< Training set filename */
#define TST_FILE        "tst.data"          /**< Testing set filename */
#define TRN_DATA_BIN    "trn_data.bin"
#define TST_DATA_BIN    "tst_data.bin"
#define TRN_LABEL_BIN   "trn_label.bin"
#define TST_LABEL_BIN   "tst_label.bin"
#endif

/* Wine */

/* 50/50 trn/tst split */

#ifdef WINE
#define FEATURES        12                      /**< Number of features */
#define CLASSES         2                       /**< Number of classes */
#define NUM_TRN_OBJ     3271                    /**< Number of training objects */
#define NUM_TST_OBJ     3226                    /**< Number of testing objects */
#define TRN_FILE        "wine_trn_50.data"      /**< Training set filename */
#define TST_FILE        "wine_trn_50.data"      /**< Testing set filename */
#define TRN_DATA_BIN    "wine_trn_data.bin"
#define TST_DATA_BIN    "wine_tst_data.bin"
#define TRN_LABEL_BIN   "wine_trn_label.bin"
#define TST_LABEL_BIN   "wine_tst_label.bin"
#endif
/*
 * @file data_seq.h
 * @brief Dataset parameters
 */

/************************************************************************/

/* Dataset Properties */

/* Iris - https://archive.ics.uci.edu/ml/datasets/iris*/

//#define IRIS 1

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
#endif

/* Wine - https://archive.ics.uci.edu/ml/datasets/Wine+Quality */

#define WINE 1

#ifdef WINE
/**< Number of features */
#define FEATURES 12
/**< Number of classes */
#define CLASSES 2
/**< Number of training objects */
#define NUM_TRN_OBJ 3271
/**< Number of testing objects */
#define NUM_TST_OBJ 3226
/** Size of a single feature in bytes (sp-float) */
#define SIZE_FEATURE 4
/** Label strings array */
char label_strings[CLASSES][100] = {
    "Red", "White" };
#endif

/************************************************************************/

/* Memory addresses */

/* Input */

/** Base address for storing training set feature vectors */
#define TRN_DATA_BASE_ADDR  	(float *)	0x010000000
/** Base address for storing testing set feature vectors */
#define TST_DATA_BASE_ADDR  	(float *)	0x013000000
/** Base address for storing training set labels */
#define TRN_LABEL_BASE_ADDR 	(int *)		0x016000000
/** Base address for storing testing set labels */
#define TST_LABEL_BASE_ADDR 	(int *)		0x017000000

/* Output */

/** Base address for storing output labels (classification output) */
#define OUT_LABELS_BASE_ADDR 	(int *)		0x018000000

/** Base address for storing distance matrix (TST_OBJ x TRN_OBJ) */
#define OUT_DIST_BASE_ADDR 		(float *)	0x019000000

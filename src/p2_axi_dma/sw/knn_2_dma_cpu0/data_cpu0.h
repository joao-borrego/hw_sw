/*
 * @file data_cpu1.h
 * @brief Dataset parameters
 */

/* Dataset Properties */

/* Iris - https://archive.ics.uci.edu/ml/datasets/iris*/

#define IRIS 1

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

//#define WINE 1

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

/************************************************************************/

/* Workload Balancing */

/** Idx of the first tst object to be processed in CPU1 */
#define FIRST_CPU1 0
/** Idx of the last object to be processed in CPU1 */
#define LAST_CPU1 (NUM_TST_OBJ)/3 - 1
/** Idx of the first tst object to be processed in CPU0 */
#define FIRST_CPU0 LAST_CPU1 + 1
/** Idx of the last tst object to be processed in CPU0 */
#define LAST_CPU0 (NUM_TST_OBJ - 1)
/** Idx of the first tst object to be classified in CPU1 */
#define FIRST_CPU1_CLASS 0
/** Idx of the last tst object to be classified in CPU1 */
#define LAST_CPU1_CLASS (NUM_TST_OBJ)/2 - 1
/** Idx of the first tst object to be classified in CPU0 */
#define FIRST_CPU0_CLASS LAST_CPU1_CLASS + 1
/** Idx of the last tst object to be classified in CPU0 */
#define LAST_CPU0_CLASS (NUM_TST_OBJ - 1)

/************************************************************************/

/* Synchronisation */

/** Semaphore */
#define SEM_ADDR (int *)0xFFFF0000;
/** CPU0 has started */
#define START_0 20
/** CPU1 has started */
#define START_1 21
/** CPU0 has ended distance calculation */
#define END_DIST_0 22
/** CPU1 has ended distance calculation */
#define END_DIST_1 23
/** CPU0 has ended classification */
#define END_CLASS_0 24
/** CPU1 has ended classification */
#define END_CLASS_1 25

import csv
import random

filename = 'winequality.dataset'    # Dataset filename
fraction = 0.50                     # Training/test split fraction
output_trn = 'wine_trn.data'      	# Output training set filename
output_tst = 'wine_tst.data'      	# Output test set filename
features = 12                       # Number of features

def loadDataset(filename, split, trainingSet=[] , testSet=[]):
    with open(filename, 'r') as csvfile:
        lines = csv.reader(csvfile)
        dataset = list(lines)
        for x in range(len(dataset) - 1):
            for y in range(features):
                dataset[x][y] = float(dataset[x][y])
            if random.random() < split:
                trainingSet.append(dataset[x])
            else:
                testSet.append(dataset[x])

def writeDataset(trainingSet=[], testSet=[]):
    with open(output_trn, 'w') as trnfile:
        for line in trainingSet:
            string = "%s\n" % line
            string = string.replace("[", "").replace("]", "").replace("'", "")
            trnfile.write(string)
            
    with open(output_tst, 'w') as tstfile:
        for line in testSet:
            string = "%s\n" % line
            string = string.replace("[", "").replace("]", "").replace("'", "")
            tstfile.write(string)

trainingSet=[]
testSet=[]
loadDataset(filename, fraction, trainingSet, testSet)
writeDataset(trainingSet, testSet)
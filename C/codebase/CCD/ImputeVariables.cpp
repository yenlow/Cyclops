/*
 * ImputeVaraibles.cpp
 *
 *  Created on: Jul 28, 2012
 *      Author: Sushil Mittal
 */

#include "ImputeVariables.h"
#include "io/CSVInputReader.h"
#include "io/BBRInputReader.h"
#include <time.h>

double randn_notrig(double mu=0.0, double sigma=1.0) {
	static bool deviateAvailable=false;        //        flag
	static float storedDeviate;                        //        deviate from previous calculation
	double polar, rsquared, var1, var2;

	//        If no deviate has been stored, the polar Box-Muller transformation is
	//        performed, producing two independent normally-distributed random
	//        deviates.  One is stored for the next round, and one is returned.
	if (!deviateAvailable) {

		//        choose pairs of uniformly distributed deviates, discarding those
		//        that don't fall within the unit circle
		do {
			var1=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
			var2=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
			rsquared=var1*var1+var2*var2;
		} while ( rsquared>=1.0 || rsquared == 0.0);

		//        calculate polar tranformation for each deviate
		polar=sqrt(-2.0*log(rsquared)/rsquared);

		//        store first deviate and set flag
		storedDeviate=var1*polar;
		deviateAvailable=true;

		//        return second deviate
		return var2*polar*sigma + mu;
	}

	//        If a deviate is available from a previous call to this function, it is
	//        returned, and the flag is set to false.
	else {
		deviateAvailable=false;
		return storedDeviate*sigma + mu;
	}
}

void getComplement(vector<real>& weights){
	for(std::vector<real>::iterator it = weights.begin(); it != weights.end(); it++) {
		*it = 1 - *it;
	}
}

ImputeVariables::ImputeVariables(){
	ccd = NULL;
	model = NULL;
}

ImputeVariables::~ImputeVariables(){
	if (modelData)
		delete modelData;
}

void ImputeVariables::initialize(CCDArguments args){
	arguments = args;
	nImputations = arguments.numberOfImputations;
	if(arguments.fileFormat == "csv"){
		reader = new CSVInputReader<ImputationHelper>();
		static_cast<CSVInputReader<ImputationHelper>*>(reader)->readFile(arguments.inFileName.c_str());
		imputeHelper = static_cast<CSVInputReader<ImputationHelper>*>(reader)->getImputationPolicy();
		modelData = static_cast<CSVInputReader<ImputationHelper>*>(reader)->getModelData();
	}
	else if(arguments.fileFormat == "bbr"){
		reader = new BBRInputReader<ImputationHelper>();
		static_cast<BBRInputReader<ImputationHelper>*>(reader)->readFile(arguments.inFileName.c_str());
		imputeHelper = static_cast<BBRInputReader<ImputationHelper>*>(reader)->getImputationPolicy();
		modelData = static_cast<BBRInputReader<ImputationHelper>*>(reader)->getModelData();
	}
	else{
		cerr << "Invalid file format." << endl;
		exit(-1);
	}
	imputeHelper->saveOrigYVector(modelData->getYVector(), modelData->getNumberOfRows());
	imputeHelper->saveOrigNumberOfColumns(modelData->getNumberOfColumns());
	imputeHelper->sortColumns();
	vector<int> sortedColIndices = imputeHelper->getSortedColIndices();
	modelData->sortDataColumns(sortedColIndices);
	srand(time(NULL));
}

void ImputeVariables::impute(){
	
	vector<int> nMissingPerColumn = imputeHelper->getnMissingPerColumn();
	int nColsToImpute = 0;
	for(int j = 0; j < (int)nMissingPerColumn.size(); j++){
		if(nMissingPerColumn[j] > 0){
			nColsToImpute++;
		}
	}
	
	cout << "Total columns to impute = " << nColsToImpute << endl;

	for(int i = 0; i < nImputations; i++){
		for(int j = 0; j < (int)nMissingPerColumn.size(); j++){
			if(nMissingPerColumn[j] > 0){
				cout << "Imputing column " << j - ((int)nMissingPerColumn.size() - nColsToImpute) << endl;
				imputeColumn(j);
			}
		}
		modelData->setYVector(imputeHelper->getOrigYVector());
		modelData->setNumberOfColumns(imputeHelper->getOrigNumberOfColumns());

		writeImputedData(i);

		resetModelData();
		resetMeanVar();
	}
}

void ImputeVariables::imputeColumn(int col){
	
	int nRows = modelData->getNumberOfRows();

	vector<real> y(nRows,0.0);
	getColumnToImpute(col, &y[0]);

	modelData->setYVector(y);
	modelData->setNumberOfColumns(col);

	vector<real> weights;

	imputeHelper->setWeightsForImputation(col,weights,nRows);

	initializeCCDModel(col,weights);

	ccd->update(arguments.maxIterations, arguments.convergenceType, arguments.tolerance);
	getComplement(weights);

	vector<real> yPred(nRows);

	ccd->getPredictiveEstimates(&yPred[0], &weights[0]);

	if(modelData->getFormatType(col) == DENSE)
		randomizeImputationsLS(yPred, weights, col);
	else
		randomizeImputationsLR(yPred, weights, col);
	//---------------------
	//char fname[100];
	//sprintf(fname,"Release/DATA_%d.txt",i);
	//FILE* fp = fopen(fname,"a");
	//vector<int> zz(modelData->getNumberOfRows(),0);
	//if(modelData->getFormatType(j) == DENSE){
	//	real* y = modelData->getDataVector(j);
	//	for(int ii = 0; ii < modelData->getNumberOfRows(); ii++){
	//		fprintf(fp,"%f ",y[ii]);
	//	}

	//}
	//else{
	//	int* z = modelData->getCompressedColumnVector(j);
	//	for(int ii = 0; ii < modelData->getNumberOfEntries(j); ii++){
	//		zz[z[ii]] = 1;
	//	}
	//	for(int ii = 0; ii < nRows; ii++)
	//		fprintf(fp,"%d ",zz[ii]);
	//}
	//fprintf(fp,"\n");
	//fclose(fp);
	//----------------------

	if (ccd)
		delete ccd;
	if (model)
		delete model;
}

void ImputeVariables::getColumnToImpute(int col, real* y){
	int nRows = modelData->getNumberOfRows();
	if(modelData->getFormatType(col) == DENSE){
		real* dataVec = modelData->getDataVector(col);
		for(int i = 0; i < nRows; i++)
			y[i] = dataVec[i];
	}
	else{
		int* columnVec = modelData->getCompressedColumnVector(col);
		for(int i = 0; i < modelData->getNumberOfEntries(col); i++)
			y[columnVec[i]] = 1.0;
	}
}

void ImputeVariables::initializeCCDModel(int col, vector<real> weights){
	if(modelData->getFormatType(col) == DENSE)	//Least Squares
		model = new ModelSpecifics<LeastSquares<real>,real>(*modelData);
	else										//Logistic Regression
		model = new ModelSpecifics<LogisticRegression<real>,real>(*modelData);

	ccd = new CyclicCoordinateDescent(modelData, *model);
	ccd->setWeights(&weights[0]);

	if(arguments.useNormalPrior)
		ccd->setPriorType(NORMAL);
	if (arguments.hyperPriorSet)
		ccd->setHyperprior(arguments.hyperprior);
}

void ImputeVariables::randomizeImputationsLR(vector<real> yPred, vector<real> weights, int col){
	if(modelData->getFormatType(col) == INDICATOR){
		int nRows = modelData->getNumberOfRows();
		vector<int> y;
		for(int i = 0; i < nRows; i++){
			if(weights[i]){
				real r = (real)rand()/RAND_MAX;
				if(r < yPred[i])
					y.push_back(i);
			}
		}
		modelData->addToColumnVector(col,y);
	}
	else{
		real* y = modelData->getDataVector(col);
		for(int i = 0; i < modelData->getNumberOfRows(); i++){
			if(weights[i]){
				real r = (real)rand()/RAND_MAX;
				if(r < yPred[i])
					y[i] = 1.0;
				else
					y[i] = 0.0;
			}
		}
	}
}

void ImputeVariables::randomizeImputationsLS(vector<real> yPred, vector<real> weights, int col){
	
	int n = 0;
	
	int nRows = modelData->getNumberOfRows();
	vector<real> dist(nRows,0.0);

	int nColsDone = (int)xMean.size();
	for(int j = nColsDone; j < col; j++){
		real* dataVec;
		int* columnVec;
		int nEntries = 0;
		FormatType formatType = modelData->getFormatType(j);
		if(formatType == DENSE){
			dataVec = modelData->getDataVector(j);
		}
		else{
			columnVec = modelData->getCompressedColumnVector(j);
			nEntries = modelData->getNumberOfEntries(j);
		}
		real mean,var;
		imputeHelper->getSampleMeanVariance(col,mean,var,dataVec,columnVec,formatType,nRows,nEntries);
		xMean.push_back(mean);
		xVar.push_back(var);
	}

	real* y = modelData->getDataVector(col);

	real sigma = 0.0;
	for(int i = 0; i < nRows; i++){
		if(!weights[i]){
			sigma += (y[i] - yPred[i]) * (y[i] - yPred[i]);
			n++;
		}
		else{
			vector<real> x(col);
			modelData->getDataRow(i,&x[0]);
			for(int j = 0; j < col; j++){
				if(xVar[j] > 1e-10)
					dist[i] += (x[j] - xMean[j]) * (x[j] - xMean[j]) / xVar[j] ;
			}
		}
	}
	sigma = sigma/(n-2);

	for(int i = 0; i < nRows; i++){
		if(weights[i]){
			double predInterval = 1.96 * sigma * sqrt(1 + 1/n + dist[i]/(n-1));
			y[i] = randn_notrig(yPred[i],predInterval);
//			y[i] = yPred[i];
		}
	}
}

void ImputeVariables::writeImputedData(int imputationNumber){
	string outFileName = arguments.inFileName;
	int dotind = outFileName.find_last_of('.');
	std::ostringstream addendum;
	addendum << "_imputed_" << imputationNumber;
	outFileName.insert(dotind,addendum.str());
	//		reader->writeFile(outFileName.c_str());

}

void ImputeVariables::resetModelData(){
	int nCols = modelData->getNumberOfColumns();
	for(int j = 0; j < nCols; j++){
		if(modelData->getFormatType(j) == INDICATOR){
			vector<int> missing;
			imputeHelper->getMissingEntries(j,missing);
			modelData->removeFromColumnVector(j,missing);
		}
	}
}

void ImputeVariables::resetMeanVar(){
	vector<int> nMissingPerColumn = imputeHelper->getnMissingPerColumn();
	int ind = 0;
	for(int i = (int)nMissingPerColumn.size()-2; i >= 0; i--){
		if(nMissingPerColumn[i] > 0){
			xMean.erase(xMean.begin()+i);
			xVar.erase(xVar.begin()+i);
		}
	}
}
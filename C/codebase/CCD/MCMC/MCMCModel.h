/*
 * MCMCModel.h
 *
 *  Created on: Jul 24, 2013
 *      Author: tshaddox
 */

#ifndef MCMCModel_H_
#define MCMCModel_H_

#include <set>
#include <algorithm>
#include <string>

#include <Eigen/Dense>
#include <Eigen/Cholesky>
#include <Eigen/Core>
#include "Parameter.h"
#include "CyclicCoordinateDescent.h"
#include "../AbstractModelSpecifics.h"
#include "BetaParameter.h"
#include "HyperpriorParameter.h"


namespace bsccs {
class MCMCModel {
public:
	MCMCModel();
	virtual ~MCMCModel();

	void generateCholesky();
	void initialize(CyclicCoordinateDescent& ccdIn, long int seed);
	void initializeHessian();

	void clearHessian();

	void Beta_HatRestorableSet(bool restorable);
	void BetaRestorableSet(bool restorable);
	void SigmaSquaredRestorableSet(bool restorable);
	void Beta_HatStore();
	void BetaStore();
	void SigmaSquaredStore();

	bool getNewLogPriorAndLikelihood();
	void setNewLogPriorAndLikelihood(bool newOrNot);

	void acceptChanges();

	void store();
	void restore();

	void logState();

	int getBetaSize();
	void setBeta(Eigen::VectorXf& newBeta);

	void writeVariances();

	void resetWithNewSigma();

	void syncCCDwithModel(set<int>& lastFixedIndices);

	void setFixedIndices(set<int>& fixedIndices);
	void setVariableIndices();
	set<int> getVariableIndices();
	set<int> getFixedIndices();
	void printFixedIndices();
	int getFixedSize();
	void setModelProbability(double modelProbabilityIn);
	double getModelProbability();
	string getModelKey();

	bool getUseHastingsRatio();
	void setUseHastingsRatio(bool useHastingsRatio);

	double getLogLikelihood();
	double getStoredLogLikelihood();
	void setLogLikelihood(double newLikelihood);

	double getLogPrior();
	double getStoredLogPrior();
	void setLogPrior(double newPrior);

	BetaParameter& getBeta();
	BetaParameter& getBeta_Hat();
	HyperpriorParameter& getSigmaSquared();

	Eigen::LLT<Eigen::MatrixXf> getCholeskyLLT();
	Eigen::MatrixXf& getHessian();

	double getTuningParameter();
	void setTuningParameter(double nextTuningParameter);


	CyclicCoordinateDescent& getCCD();

private:
	Eigen::LLT<Eigen::MatrixXf> CholDecom;

	Eigen::MatrixXf HessianMatrix;
	Eigen::MatrixXf HessianMatrixInverse;


	vector<vector<bsccs::real> > hessian;

	CyclicCoordinateDescent* ccd;

	set<int> fixedBetaIndices;
	string modelKey;
	set<int> variableBetaIndices;
	BetaParameter Beta_Hat;
	BetaParameter Beta;
	HyperpriorParameter SigmaSquared;

	double modelProbability;

	double logLikelihood;
	double logPrior;

	double storedLogLikelihood;
	double storedLogPrior;

	bool useHastingsRatio;

	bool newLogPriorAndLikelihood;

	int J;

	double tuningParameter;



};
}





#endif /* MCMCModel_H_ */
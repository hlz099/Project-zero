/*
 * MGALTphase.h
 *
 *  Created on: Jul 15, 2012
 *      Author: Jacob
 */

#ifndef MGALTPHASE_H_
#define MGALTPHASE_H_

#include "phase.h"
#include "journey.h"
#include "missionoptions.h"
#include "universe.h"
#include "STM.h"
//#include "UniversalKeplerPropagator.h"

#include <vector>

namespace EMTG {

	class MGA_LT_phase: public EMTG::phase {
	public:
		//constructor
		MGA_LT_phase();
		MGA_LT_phase(int j, int p, missionoptions* options);

		//destructor
		virtual ~MGA_LT_phase();

		//evaluate function
		//return 0 if successful, 1 if failure
		int evaluate(double* X, int* Xindex, double* F, int* Findex, double* G, int* Gindex, int needG, double* current_epoch, double* current_state, double* current_deltaV, double* boundary1_state, double* boundary2_state, int j, int p, EMTG::Astrodynamics::universe* Universe, missionoptions* options);

		//output function
		//return 0 if successful, 1 if failure
		int output(missionoptions* options, const double& launchdate, int j, int p, EMTG::Astrodynamics::universe* Universe, int* eventcount);

		//bounds calculation function
		//return 0 if successful, 1 if failure
		int calcbounds(vector<double>* Xupperbounds, vector<double>* Xlowerbounds, vector<double>* Fupperbounds, vector<double>* Flowerbounds, vector<string>* Xdescriptions, vector<string>* Fdescriptions, vector<int>* iAfun, vector<int>* jAvar, vector<int>* iGfun, vector<int>* jGvar, vector<string>* Adescriptions, vector<string>* Gdescriptions, vector<double>* synodic_periods, int j, int p, EMTG::Astrodynamics::universe* Universe, missionoptions* options);

		//top-level function to calculate the match point derivatives
		int calculate_match_point_derivatives(	double* G,
												int* Gindex,
												const int& j, 
												const int& p,
												missionoptions* options, 
												EMTG::Astrodynamics::universe* Universe);

		//function to calculate the derivative of a match point constraint with respect to a decision variable in the forward propagation
		int calculate_match_point_forward_propagation_derivatives(	double* G,
																	int* Gindex,
																	const int& j, 
																	const int& p,
																	missionoptions* options, 
																	EMTG::Astrodynamics::universe* Universe,
																	const int& step,
																	const int& stepnext,
																	double& dxdu,
																	double& dydu,
																	double& dzdu,
																	double& dxdotdu,
																	double& dydotdu,
																	double& dzdotdu,
																	double& dmdu,
																	double& dtdu);

		//function to calculate the derivative of a match point constraint with respect to a decision variable in the backward propagation
		int calculate_match_point_backward_propagation_derivatives(	double* G,
																	int* Gindex,
																	const int& j, 
																	const int& p,
																	missionoptions* options, 
																	EMTG::Astrodynamics::universe* Universe,
																	const int& backstep,
																	const int& stepnext,
																	double& dxdu,
																	double& dydu,
																	double& dzdu,
																	double& dxdotdu,
																	double& dydotdu,
																	double& dzdotdu,
																	double& dmdu,
																	double& dtdu);

		//time information
		vector <double> event_epochs;

		//state information
		vector<double> match_point_state;
		vector<double> throttle;
		vector<double> dVmax;
		vector< vector<double> > dV;
		vector< vector<double> > ForceVector;

		//Propagator
		//Astrodynamics::UniversalKeplerPropagator Propagator;
	};

} /* namespace EMTG */
#endif /* MGALTPHASE_H_ */

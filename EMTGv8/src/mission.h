/*
 * mission.h
 *
 *  Created on: Jul 17, 2012
 *      Author: Jacob
 */

#ifndef MISSION_H_
#define MISSION_H_

#include "problem.h"
#include "journey.h"
#include "file_utilities.h"

#include <vector>

#include "boost/ptr_container/ptr_vector.hpp"

namespace EMTG {

class mission: public EMTG::problem 
{
public:
	//constructor
	mission();
	mission(int* Xouter, missionoptions* options_in, boost::ptr_vector<Astrodynamics::universe>& TheUniverse_in, int thread_ID_assigned, int problem_ID_assigned);

	//destructor
	virtual ~mission();

	//methods
	//evaluate function
	//return 0 if successful, 1 if failure
	virtual int evaluate(double* X, double* F, double* G, int needG, const vector<int>& iGfun, const vector<int>& jGvar);

	//output function
	//return 0 if successful, 1 if failure
	virtual int output();

	//output mission structure
	virtual int output_mission_tree(string filename);

	//bounds calculation function
	//return 0 for success, 1 for failure
	virtual int calcbounds();

	//outer-loop parse function
	//return 0 for success, 1 for failure
	virtual int parse_outer_loop(int* Xouter);

	//GMAT output function(s)
	virtual void output_GMAT_preamble();
	virtual void output_GMAT_mission();

	//function to create an FBLT-S initial guess from an FBLT input
	virtual vector<double> create_initial_guess(vector<double> XFBLT, vector<string>& NewXDescriptions);

	//function to interpolate an initial guess
	virtual void interpolate(int* Xouter, const vector<double>& initialguess);

	//function to find constraint/objective function dependencies due to an spiral anywhere in the mission
	void find_dependencies_due_to_escape_spiral(vector<double>* Xupperbounds,
												vector<double>* Xlowerbounds,
												vector<double>* Fupperbounds,
												vector<double>* Flowerbounds,
												vector<string>* Xdescriptions,
												vector<string>* Fdescriptions,
												vector<int>* iAfun,
												vector<int>* jAvar,
												vector<int>* iGfun,
												vector<int>* jGvar,
												vector<string>* Adescriptions,
												vector<string>* Gdescriptions,
												missionoptions* options,
												const int& Findex);

	void find_dependencies_due_to_capture_spiral(vector<double>* Xupperbounds,
												vector<double>* Xlowerbounds,
												vector<double>* Fupperbounds,
												vector<double>* Flowerbounds,
												vector<string>* Xdescriptions,
												vector<string>* Fdescriptions,
												vector<int>* iAfun,
												vector<int>* jAvar,
												vector<int>* iGfun,
												vector<int>* jGvar,
												vector<string>* Adescriptions,
												vector<string>* Gdescriptions,
												missionoptions* options,
												const int& Findex);

	void find_dependencies_due_to_capture_spiral_in_final_journey(vector<double>* Xupperbounds,
																vector<double>* Xlowerbounds,
																vector<double>* Fupperbounds,
																vector<double>* Flowerbounds,
																vector<string>* Xdescriptions,
																vector<string>* Fdescriptions,
																vector<int>* iAfun,
																vector<int>* jAvar,
																vector<int>* iGfun,
																vector<int>* jGvar,
																vector<string>* Adescriptions,
																vector<string>* Gdescriptions,
																missionoptions* options,
																const int& Findex);

	//vector of journeys
	boost::ptr_vector<journey> journeys;

	//vector of universes
	boost::ptr_vector<Astrodynamics::universe> TheUniverse;

	//fields
	int number_of_journeys;
	double current_deltaV; //value to hold the current deltaV as we progress through the mission
	double current_epoch; //value to hold the current epoch as we progress through the mission
	double current_state[7]; //array to hold the current spacecraft state as we progress through the mission
	double dry_mass; //in kg

	//derivative information
	vector<int> objective_function_G_indices;
	vector<int> timeconstraints_G_indices;
	vector<double> timeconstraints_X_scale_ranges;
	int derivative_of_flight_time_with_respect_to_launch_date_G_index;
	vector<int> derivative_of_flight_time_with_respect_to_journey_initial_mass_increment_ratios_for_spirals;
	vector<int> dry_mass_constraint_G_indices;
	vector<int> dry_mass_constraint_X_indices;
	vector<int> objectivefunction_X_indices;
	vector<int> objectivefunction_G_indices;
	vector<double> objectivefunction_X_scale_ranges;

	//time information
	double max_TU; //largest time unit, for constraint scaling
};

} /* namespace EMTG */
#endif /* MISSION_H_ */

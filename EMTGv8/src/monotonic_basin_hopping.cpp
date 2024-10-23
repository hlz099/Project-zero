//Monotonic Basin Hopping
//for EMTG version 8
//Jacob Englander 7-27-2012

#include "problem.h"
#include "monotonic_basin_hopping.h"
#include "EMTG_math.h"
#include "SNOPT_user_function.h"

#include <iostream>
#include <fstream>

#include "boost/random/uniform_int.hpp"
#include "boost/random/uniform_real.hpp"
#include "boost/random/mersenne_twister.hpp"

#include "snopt.h"
#include "snoptProblem.h"
#include "snfilewrapper.h"

#ifdef _use_WORHP
#include "EMTG_WORHP_interface.h"
#endif

using namespace std;

namespace EMTG { namespace Solvers {
	//constructors
	MBH::MBH() {}

	MBH::MBH(EMTG::problem* Problem_input)
	{
		//initialize the MBH variables
		initialize(Problem_input);

		//create the dynamic SNOPT variables
		neF = Problem->total_number_of_constraints;

		lenA  = Problem->total_number_of_NLP_parameters*Problem->total_number_of_constraints;

		iAfun = new integer[lenA];
		jAvar = new integer[lenA];
		A  = new doublereal[lenA];

		lenG   = Problem->total_number_of_NLP_parameters*Problem->total_number_of_constraints;
		iGfun = new integer[lenG];
		jGvar = new integer[lenG];

		x      = new doublereal[Problem->total_number_of_NLP_parameters];
		xlow   = new doublereal[Problem->total_number_of_NLP_parameters];
		xupp   = new doublereal[Problem->total_number_of_NLP_parameters];
		xmul   = new doublereal[Problem->total_number_of_NLP_parameters];
		xstate = new    integer[Problem->total_number_of_NLP_parameters];

		F      = new doublereal[neF];
		Flow   = new doublereal[neF];
		Fupp   = new doublereal[neF];
		Fmul   = new doublereal[neF];
		Fstate = new integer[neF];

		nxnames = 1;
		nFnames = 1;
		xnames = new char[nxnames*8];
		Fnames = new char[nFnames*8];

		DummyReal = new doublereal[10];

		ObjRow = 0;
		ObjAdd = 0;

		//initialize a few more SNOPT variables whose quantities do not change with an MBH run
		for (int k=0; k < Problem->total_number_of_NLP_parameters; ++k)
		{
			xlow[k] = 0.0;
			xupp[k] = 1.0;
		}
		for (int k=0; k < Problem->total_number_of_constraints; ++k)
		{
			Flow[k] = Problem->Flowerbounds[k];
			Fupp[k] = Problem->Fupperbounds[k];
		}

		//we don't have any real intuition for what the intial "states" of the F functions will be
		//the same applies for the constraint Lagrange multipliers, so set all of the Fstates = 0 and Fmuls to 0.0
		for (int k=0; k < Problem->total_number_of_constraints; ++k)
		{
			Fstate[k] = 0;
			Fmul[k] = 0.0;
		}

		SNOPTproblem = new snoptProblem(true);

		SNOPTproblem->setProblemSize( Problem->total_number_of_NLP_parameters, neF );
		SNOPTproblem->setObjective  ( ObjRow, ObjAdd );
		SNOPTproblem->setUserMem    ( 5000, (char*) Problem, 500, (integer*) &SNOPT_start_time, 500, DummyReal);
		SNOPTproblem->setA          ( lenA, iAfun, jAvar, A );
		SNOPTproblem->setG          ( lenG, iGfun, jGvar );
		SNOPTproblem->setXNames     ( xnames, nxnames );
		SNOPTproblem->setFNames     ( Fnames, nFnames );
		SNOPTproblem->setProbName   ( "EMTG" );
		SNOPTproblem->setUserFun    ( SNOPT_user_function );
		SNOPTproblem->setIntParameter("Iterations limit", 100*Problem->options.snopt_major_iterations);
		SNOPTproblem->setIntParameter("Major iterations limit", Problem->options.snopt_major_iterations);
		SNOPTproblem->setIntParameter( "Derivative option", 0 );
		SNOPTproblem->setIntParameter("Minor print level", 0);
		SNOPTproblem->setRealParameter("Major feasibility tolerance", Problem->options.snopt_feasibility_tolerance);
		
		SNOPTproblem->setIntParameter("Major Print Level", 1);
		SNOPTproblem->setRealParameter("Optimality tolerance", 1.0e-6);
		if (Problem->options.check_derivatives)
		{
			SNOPTproblem->setIntParameter("Print file", 1 );
			SNOPTproblem->setIntParameter("Summary file", 1);
			SNOPTproblem->setIntParameter("Verify level", 3); //0 = cheap test 1 = individual gradients checked (OK or BAD) 2 = Individual columns of the Jacobian are checked 3 = 1 and 2 happen -1 = Derivative checking is disabled
		}
		if (Problem->options.mission_type < 2 || Problem->options.quiet_NLP) //for MGA, MGA-DSM missions
		{
			SNOPTproblem->setIntParameter("Major Print Level", 0);
			SNOPTproblem->setIntParameter( "Print No", 0 );
			SNOPTproblem->setIntParameter( "Summary file", 0 );
			SNOPTproblem->setParameter("Suppress parameters");
		}
		else
		{
#ifdef QUIET_SNOPT
			SNOPTproblem->setIntParameter("Major Print Level", 0);
			SNOPTproblem->setIntParameter("Print No", 0);
#endif
		}

		if (Problem->options.NLP_solver_mode)
			SNOPTproblem->setParameter("Minimize");
		else
			SNOPTproblem->setParameter("Feasible point");

		switch (Problem->options.objective_type)
		{
		case 0: //minimum dV
			fcrit = 100;
		case 1: //minimum time
			fcrit = 100;
		case 2: //maximum final mass
			fcrit = 0.01;
		case 3: //GTOC1
			fcrit = 0.0;
		}

		//set up the random number generator
		DoubleDistribution = boost::uniform_real<>(0.0, 1.0);

		//search through the problem object and identify which decision variables are flight time variables
		if (Problem->options.MBH_time_hop_probability > 0.0)
		{
			for (int entry = 0; entry < Problem->total_number_of_NLP_parameters; ++entry)
				if ( Problem->Xdescriptions[entry].find("flight time") < 1024)
				{
					this->time_variable_indices.push_back(entry);
				}
		}
	}

	//destructor
	MBH::~MBH()
	{
		//delete the SNOPT object
		delete SNOPTproblem;

		//delete all temporary SNOPT arrays
		delete [] iAfun;
		delete [] jAvar;
		delete [] A;

		delete [] iGfun;
		delete [] jGvar;
		
		delete [] x;
		delete [] xlow;
		delete [] xupp;
		delete [] xmul;
		delete [] xstate;

		delete [] F;
		delete [] Flow;
		delete [] Fupp;
		delete [] Fmul;
		delete [] Fstate;
		delete [] xnames;
		delete [] Fnames;

		delete [] DummyReal;
	}

	
	//method to initialize the MBH solver
	//resets all storage fields
	int MBH::initialize(EMTG::problem* Problem_input)
	{
		Problem = Problem_input;


		//size the storage vectors
		Xtrial_scaled.resize(Problem->total_number_of_NLP_parameters, 0);
		Xlocalbest_scaled.resize(Problem->total_number_of_NLP_parameters, 0);
		Xcurrent_scaled.resize(Problem->total_number_of_NLP_parameters, 0);
		Xbest_scaled.resize(Problem->total_number_of_NLP_parameters, 0);
		Xbest.resize(Problem->total_number_of_NLP_parameters, 0);

		archive.clear();
		archive_scores.clear();

		//clear the scores
		ftrial = EMTG::math::LARGE;
		fcurrent = EMTG::math::LARGE;
		fbest = EMTG::math::LARGE;
		most_feasible = EMTG::math::LARGE;
		
		//clear the counters
		number_of_solutions = 0;
		number_of_improvements = 0;
		number_of_failures_since_last_improvement = 0;
		number_of_resets = 0;

		//reset the Jacobian computed flag
		computed_Jacobian = false;
		jacfullrankflag = true;

		//reset the Jacobian printed flag
		this->printed_sparsity = false;

		//seed the random number generator based on the node's clock
		RNG.seed(std::time(0));

		return 0;
	}

	//function to reset to a new point, either at the beginning of the problem or when a certain number of failures is reached
	int MBH::reset_point()
	{
		//reset the number of failures
		this->number_of_failures_since_last_improvement = 0;

		//increment the number of global resets
		++this->number_of_resets;

		//generate a new random trial point
		for (int k = 0; k < Problem->total_number_of_NLP_parameters; ++k)
			this->Xtrial_scaled[k] = DoubleDistribution(RNG);

		fcurrent = EMTG::math::LARGE;

		return 0;
	}

	//function to seed a new point based on an input trial point
	int MBH::seed(vector<double> seed_vector)
	{
		//reset the number of failures
		number_of_failures_since_last_improvement = 0;

		//read a trial point
		for (int k = 0; k < Problem->total_number_of_NLP_parameters; ++k)
			Xtrial_scaled[k] = (seed_vector[k] - Problem->Xlowerbounds[k]) / (Problem->Xupperbounds[k] - Problem->Xlowerbounds[k]);

		fcurrent = EMTG::math::LARGE;

		return 0;
	}

	//function to perform a "hop" operation
	int MBH::hop()
	{
		if (Problem->options.MBH_hop_distribution == 0)
		{
			//perform a uniform "hop"
			for (int k=0; k < Problem->total_number_of_NLP_parameters; ++k)
			{
				double r = DoubleDistribution(RNG);

				step_size = 2 * (r - 0.5) * Problem->options.MBH_max_step_size;

				Xtrial_scaled[k] = Xcurrent_scaled[k] + step_size;
			}
		}
		else if (Problem->options.MBH_hop_distribution == 1)
		{
			//perform a Cauchy "hop"
			for (int k=0; k < Problem->total_number_of_NLP_parameters; ++k)
			{
				///double r = DoubleDistribution(RNG);
				//int s = DoubleDistribution(RNG) > 0.5 ? 1 : -1;

				//step_size = s * Problem->options.MBH_max_step_size * 1.0/(EMTG::math::PI*(1.0 + r*r));

				//Cauchy generator v2 as per instructions from Arnold Englander 1-12-2014 using Cauchy CDF

				double r = 0.0;

				while (r < math::SMALL)
					r = DoubleDistribution(RNG);

				step_size = Problem->options.MBH_max_step_size * tan(math::PI*(r - 0.5));

				Xtrial_scaled[k] = Xcurrent_scaled[k] + step_size;
			}
		}
		else if (Problem->options.MBH_hop_distribution == 2)
		{
			double alpha = Problem->options.MBH_Pareto_alpha;
			//perform a Pareto hop
			for (int k=0; k < Problem->total_number_of_NLP_parameters; ++k)
			{
				//Pareto distribution from x = 1.0 with alpha, modified to start from x = 0.0

				//s*(((alpha-1)/xU_min)/power((xU_min/u),-alpha))
				double r = ( (alpha - 1.0) / math::SMALL ) / pow((math::SMALL / (math::SMALL + DoubleDistribution(RNG))), -alpha);
				int s = DoubleDistribution(RNG) > 0.5 ? 1 : -1;

				step_size = s * Problem->options.MBH_max_step_size * r;

				Xtrial_scaled[k] = Xcurrent_scaled[k] + step_size;
			}
		}
		else if (Problem->options.MBH_hop_distribution == 3)
		{
			//perform a Gaussian hop
			double sigma = Problem->options.MBH_max_step_size;
			double sigma2 = sigma * sigma;
			for (int k=0; k < Problem->total_number_of_NLP_parameters; ++k)
			{
				double r = DoubleDistribution(RNG);
				int s = DoubleDistribution(RNG) > 0.5 ? 1 : -1;

				step_size = s / (sigma * sqrt(math::TwoPI)) * exp(-r*r / (2*sigma2));

				Xtrial_scaled[k] = Xcurrent_scaled[k] + step_size;
			}
		}

		/* uncomment these lines to re-enable MBH clipping. Currently we let SNOPT self-clip
		if (Xtrial_scaled[k] > 1.0)
			Xtrial_scaled[k] = 1.0;
		else if (Xtrial_scaled[k] < 0.0)
			Xtrial_scaled[k] = 0.0;
			*/

		return 0;
	}

	//function to perform a "time hop" operation
	int MBH::time_hop()
	{
		//loop through any time variables and if (uniform random < threshold) then add/subtract a synodic period
		for (int timeindex = 0; timeindex < time_variable_indices.size(); ++timeindex)
		{
			if (DoubleDistribution(RNG) < Problem->options.MBH_time_hop_probability)
			{
				int k = time_variable_indices[timeindex];
				int s = DoubleDistribution(RNG) > 0.5 ? 1 : -1;
				Xtrial_scaled[k] = Xcurrent_scaled[k] + s * Problem->synodic_periods[timeindex] / (Problem->Xupperbounds[k] - Problem->Xlowerbounds[k]);
			}
		}

		return 0;
	}

	//function to perform a "slide" operation, i.e. run SNOPT
	int MBH::slide()
	{
		//Step 1: set the current state equal to the initial guess
		for (int k = 0; k < Problem->total_number_of_NLP_parameters; ++k)
		{
			xstate[k] = 0;
			x[k] = Xtrial_scaled[k];
		}

		//print the sparsity file and XF files if this is the first pass, otherwise don't to save time and hard drive cycles
		if (!this->printed_sparsity)
		{
			this->printed_sparsity = true;
			Problem->output_Jacobian_sparsity_information(Problem->options.working_directory + "//" + Problem->options.mission_name + "_" + Problem->options.description + "_SparsityDescriptions.csv");
			Problem->output_problem_bounds_and_descriptions(Problem->options.working_directory + "//" + Problem->options.mission_name + "_" + Problem->options.description + "XFfile.csv");
		}

		if (this->Problem->options.NLP_solver_type == 1)
		{
			//run WORHP
#ifdef _use_WORHP
			this->WORHP_interface = new EMTG_WORHP_interface(Problem);
			this->WORHP_interface->SetInitialGuess(Xtrial_scaled.data());
			this->WORHP_interface->Solve();
			delete this->WORHP_interface;
#else
			cout << "WORHP not installed" << endl;
#endif
		}
		else
		{
			//run SNOPT

			//Step 2: attempt to calculate the Jacobian
			SNOPTproblem->setX          ( x, xlow, xupp, xmul, xstate );
			SNOPTproblem->setF          ( F, Flow, Fupp, Fmul, Fstate );

			if (!computed_Jacobian || !jacfullrankflag)
			{
				vector<bool> cjacflag(neF);
				if (true /*Problem->options.derivative_type > 0*/)
				{
					Problem->output_Jacobian_sparsity_information(Problem->options.working_directory + "//" + Problem->options.mission_name + "_" + Problem->options.description + "_SparsityDescriptions.csv");

					for (size_t entry = 0; entry < Problem->Adescriptions.size(); ++entry)
					{
						iAfun[entry] = Problem->iAfun[entry];
						jAvar[entry] = Problem->jAvar[entry];
						A[entry] = Problem->A[entry];
					}
					for (size_t entry = 0; entry < Problem->Gdescriptions.size(); ++entry)
					{
						iGfun[entry] = Problem->iGfun[entry];
						jGvar[entry] = Problem->jGvar[entry];
					}
				
					SNOPTproblem->setNeA(Problem->Adescriptions.size());
					SNOPTproblem->setNeG(Problem->Gdescriptions.size());

					computed_Jacobian = true;
				}
				else
				{
					SNOPTproblem->computeJac    ();
					write_sparsity("sparsitySNJac.txt");
					Problem->output_Jacobian_sparsity_information(Problem->options.working_directory + "//" + Problem->options.mission_name + "_" + Problem->options.description + "_SparsityDescriptions_SNJAC.csv");

					//Step 3: If the Jacobian calculation succeeded, then check if it is full rank
					if (SNOPTproblem->getInform() == 102)
					{
						computed_Jacobian = true;
			
						for (int k=0; k<neF; ++k)
							cjacflag[k] = false;

						for (int i=0; i<SNOPTproblem->neG; ++i)
							cjacflag[iGfun[i]-1] = true;

						for (int i=0; i<SNOPTproblem->neA; ++i)
							cjacflag[iAfun[i]-1] = true;
			
						jacfullrankflag = true;
						for (int i=0; i<neF-1; ++i)
						{
							if (!cjacflag[i])
								jacfullrankflag = false;
						}
					}
				}	
			}

			//If the Jacobian is full rank, then run SNOPT
			if (jacfullrankflag)
			{
				SNOPT_start_time = time(NULL);
				SNOPTproblem->solve( 0 );
			}
			else
			{
				cout << "Jacobian is not full rank. SNOPT cannot be run." << endl;
			}
		}

		//Step 4: store the results
		for (int k = 0; k < Problem->total_number_of_NLP_parameters; ++k)
		{
			Xtrial_scaled[k] = x[k];
		}
		Problem->unscale(x);
		if (computed_Jacobian || this->Problem->options.NLP_solver_type == 1)
		{
			try
			{
				Problem->evaluate(&(Problem->X[0]), F, &Problem->G[0], 0, Problem->iGfun, Problem->jGvar);
			}
			catch (int errorcode) //integration step error
			{
				cout << "EMTG::Invalid initial point or failure in objective function." << endl;
				F[0] = EMTG::math::LARGE;
			}
		}

		else
		{
			cout << "EMTG::Invalid initial point or failure in objective function." << endl;
			F[0] = EMTG::math::LARGE;
		}
		for (int k = 0; k < Problem->total_number_of_constraints; ++k)
		{
			Problem->F[k] = F[k];
		}

		return (int) jacfullrankflag;
	}

	
	//function to run MBH
	int MBH::run()
	{
		bool new_point;
		bool seeded_step = false;;

		//If we have seeded MBH, start from that seed. Otherwise we will need to generate a new random point.
		if ( !(Problem->options.seed_MBH) )
			new_point = true;
		else
		{
			seeded_step = true;
			new_point = false;
		}

		int number_of_attempts = true;
		bool continue_flag = true;

		//print the archive header
		string archive_file = Problem->options.working_directory + "//" + Problem->options.mission_name + "_" + Problem->options.description + "archive.emtg_archive";
		print_archive_header(archive_file);

		time_t tstart = time(NULL);

		do
		{
			++number_of_attempts;
			if (new_point)
			{
				//Step 1: generate a random new point
				reset_point();

				number_of_failures_since_last_improvement = 0;
			}
			else if (!seeded_step)
			{
				//Step 1 (alternate): perturb the existing point
				hop();

				if (Problem->options.MBH_time_hop_probability > 0.0)
					time_hop();
			}

			//Step 2: apply the slide operator
			//Step 2.1: If this is an MGA or MGA-DSM problem, make the finite differencing step coarse
			if (Problem->options.mission_type < 2)
			{
				cout << "Performing coarse optimization step" << endl;
				SNOPTproblem->setRealParameter("Difference interval", 1.0e-2);
			}

			int ransnopt = slide();

			//Step 3: determine if the new trial point is feasible and if so, operate on it
			double feasibility = check_feasibility();

			//Step 3.01 if this is an MGA or MGA-DSM problem, make the finite differencing step tight and run again
			if (Problem->options.mission_type < 2 && (SNOPTproblem->getInform() <= 3 || feasibility < Problem->options.snopt_feasibility_tolerance))
			{
				cout << "Coarse optimization step succeeded with J = " << F[0] << endl;
				cout << "Performing fine optimization step" << endl;
				SNOPTproblem->setRealParameter("Difference interval", 1.5e-8);
				slide();

				if (SNOPTproblem->getInform() <= 3 || feasibility < Problem->options.snopt_feasibility_tolerance)
				{
					cout << "Fine optimization step succeeded with J = " << F[0] << endl;
				}
			}

			if (SNOPTproblem->getInform() <= 3 || feasibility < Problem->options.snopt_feasibility_tolerance)
			{
				//Step 3.1: if the trial point is feasible, add it to the archive
				Problem->unscale(&Xtrial_scaled[0]);
				archive.push_back(Problem->X);
				archive_scores.push_back(F[0]);
				archive_timestamps.push_back(time(NULL) - tstart);
				archive_step_count.push_back(number_of_attempts);
				archive_reset_count.push_back(number_of_resets);

				print_archive_line(archive_file, number_of_solutions);
				cout << "Hop evaluated mission " << Problem->options.description << " with fitness " << F[0] << endl;

				++number_of_solutions;

				//Step 3.2: if the point came from a hop operation and is superior to the current point, adopt it as the new current point
				if (!(new_point))
				{
					if (F[0] < fcurrent)
					{
						fcurrent = F[0];
						Xcurrent_scaled = Xtrial_scaled;

						cout << "New local best" << endl;

						++number_of_improvements;

						number_of_failures_since_last_improvement = 0;
					}
					else
						++number_of_failures_since_last_improvement;
				}
				else //if this is from a reset, adopt it as the current point and turn off the "new point" flag
				{
					fcurrent = F[0];
					Xcurrent_scaled = Xtrial_scaled;
					new_point = false;
				}

				//Step 3.3 update the global best if applicable
				if (F[0] < fbest)
				{
					fbest = F[0];
					Xbest_scaled = Xtrial_scaled; 
					Problem->unscale(&Xbest_scaled[0]);
					Problem->Xopt = Problem->X; //we store the unscaled Xbest

					cout << "New global best" << endl;

					//Write out a results file for the current global best
					Problem->evaluate(&(Problem->X[0]), F, &Problem->G[0], 0, Problem->iGfun, Problem->jGvar);
					Problem->options.outputfile = Problem->options.working_directory + "//" + Problem->options.mission_name + "_" + Problem->options.description + ".emtg";
					Problem->output();
				}

			}
			else
			{
				++number_of_failures_since_last_improvement;

				if (feasibility < most_feasible)
				{
					most_feasible = feasibility;
					Problem->unscale(x);
					closest_to_feasible_solution = Problem->X;
				}
			}

			if (number_of_failures_since_last_improvement >= Problem->options.MBH_max_not_improve)
				new_point = true;
				
			/*********************************************************************************************************************************************
			The following code is written to stop MBH early when a feasible solution is not found after 10% of the allowed execution time is elapsed.
			This feature is considered undesirable as of 6/24/2013 and has been disabled.
			**********************************************************************************************************************************************
			time_t deltaT = time(NULL) - tstart;
			if (deltaT >= 0.1 * Problem->options.MBH_max_run_time && (fbest > fcrit || number_of_solutions == 0) && deltaT > 1800)
			{
				break;
			}
			*/
		} while (number_of_attempts < Problem->options.MBH_max_trials && (time(NULL) - tstart) < Problem->options.MBH_max_run_time);

		cout << endl;
		if (number_of_solutions > 0)
			cout << "Best value found was " << fbest << endl;
		else
			cout << "No feasible solutions found." << endl;

		return number_of_solutions;
	}

	//function to print the archive header to a text file
	int MBH::print_archive_header(string filename)
	{
		//print the archive in CSV format
		//the final entry in each line is the fitness value

		ofstream outputfile (filename.c_str(), ios::trunc);

		//archive column headers
		for (int entry = 0; entry < Problem->total_number_of_NLP_parameters; ++entry)
			outputfile << Problem->Xdescriptions[entry] << ",";
		outputfile << "reset count,";
		outputfile << "step count,";
		outputfile << "solution timestamp,";
		outputfile << "Objective function" << endl;



		outputfile.close();

		return 0;
	}

	//function to print the archive line to a text file
	int MBH::print_archive_line(string filename, int linenumber)
	{
		//print the archive in CSV format
		//the final entry in each line is the fitness value

		ofstream outputfile (filename.c_str(), ios::app);

		//archive lines
		for (int entry = 0; entry < Problem->total_number_of_NLP_parameters; ++entry)
			outputfile << archive[linenumber][entry] << ",";
		outputfile << archive_reset_count[linenumber] << ",";
		outputfile << archive_step_count[linenumber] << ",";
		outputfile << archive_timestamps[linenumber] << ",";
		outputfile << archive_scores[linenumber] << endl;

		outputfile.close();

		return number_of_solutions;
	}

	//function to write out the Jacobian sparsity
	int MBH::write_sparsity(string filename)
	{
		ofstream sparsityfile(filename.c_str(), ios::trunc);

		for (int k = 0; k < SNOPTproblem->neA; ++k)
			sparsityfile << "Linear, " << iAfun[k] << "," << jAvar[k] << endl;

		for (int k = 0; k < SNOPTproblem->neG; ++k)
			sparsityfile << "Nonlinear, " << iGfun[k] << "," << jGvar[k] << endl;

		sparsityfile.close();

		return 0;
	}

	//function to check feasibility of a solution
	double MBH::check_feasibility()
	{
		double max_constraint_violation = 0.0;
		int worst_constraint;
		for (int k = 1; k < Problem->total_number_of_constraints; ++k)
		{
			if (F[k] > Fupp[k])
			{
				if (F[k] - Fupp[k] > max_constraint_violation)
				{
					max_constraint_violation = F[k] - Fupp[k];
					worst_constraint = k;
				}
			}
			else if (F[k] < Flow[k])
			{
				if (Flow[k] - F[k] > max_constraint_violation)
				{
					max_constraint_violation = Flow[k] - F[k];
					worst_constraint = k;
				}
			}
		}

		return max_constraint_violation / EMTG::math::norm(&Xtrial_scaled[0], Problem->total_number_of_NLP_parameters);
	}

}} //close namespace
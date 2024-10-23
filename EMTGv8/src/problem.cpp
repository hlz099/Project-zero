/*
 * problem.cpp
 *
 *  Created on: Jul 17, 2012
 *      Author: Jacob
 */

#include "problem.h"
#include "mission.h"
#include "monotonic_basin_hopping.h"
#include "AdaptiveConstrainedDiffEvolve.h"
#include "EMTG_math.h"

#include <iostream>

namespace EMTG {

	problem::problem() :
		thread_ID(0),
		problem_ID(0),
		total_number_of_constraints(0),
		total_number_of_NLP_parameters(0)
	{
		// default constructor should never be called
	}

	problem::~problem() {
		// default destructor should never be called
	}

	//functions to scale and unscale the decision vector
	int problem::unscale(double* Xscaled)
	{
		for (int k = 0; k < total_number_of_NLP_parameters; ++k)
		{
			if (Xscaled[k] < 0.0)
				X[k] = Xlowerbounds[k];
			else if (Xscaled[k] > 1.0)
				X[k] = Xupperbounds[k];
			else
				X[k] = Xscaled[k] * (Xupperbounds[k] - Xlowerbounds[k]) + Xlowerbounds[k];
		}

		return 0;
	}

	int problem::scale(double* Xscaled)
	{
		for (int k = 0; k < total_number_of_NLP_parameters; ++k)
			Xscaled[k] = (X[k] - Xlowerbounds[k]) / (Xupperbounds[k] - Xlowerbounds[k]);

		return 0;
	}

	//outer-loop parse function
	//return 0 for success, 1 for failure
	int problem::parse_outer_loop(int* Xouter, int n_outer_loop) {return 0;}

	int problem::optimize() {
		// call the appropriate optimizer, passing along a pointer to the problem object so that it can be evaluated
		//success, so return 0

		int number_of_solutions;

		switch (options.run_inner_loop)
		{
		case 0: //run trialX
			{
				this->Xopt = options.current_trialX;

				this->evaluate(&this->Xopt[0], &F[0], &G[0], 0, iGfun, jGvar);

				std::cout << "Fitness = " << F[0] << endl;

				options.outputfile = options.working_directory + "//" + options.mission_name + "_" + options.description + ".emtg";

				if (options.check_derivatives)
					this->check_and_print_derivatives(options.working_directory + "//" + options.mission_name + "_" + options.description + "derivcheck.csv");

				break;
			}
		case 1: //run a batch of decision vectors
			{
				//TODO problem::optimize() run a batch of decision vectors
				break;
			}
		case 2: //run MBH
			{
				EMTG::Solvers::MBH solver(this);

				if (options.seed_MBH)
					solver.seed(options.current_trialX);

				number_of_solutions = solver.run();
			
				options.outputfile = options.working_directory + "//" + options.mission_name + "_" + options.description + ".emtg";

				if (number_of_solutions == 0 || solver.fbest > 1.0e+10)
				{
					Xopt = Xlowerbounds;
					options.outputfile = options.working_directory + "//FAILURE_" + options.mission_name + "_" + options.description + ".emtg";
				}
				evaluate(&Xopt[0], &F[0], &G[0], 0, iGfun, jGvar);

				break;
			}
		case 3:
			{
				EMTG::Solvers::AdaptiveConstrainedDiffEvolve solver(this);

				options.outputfile = options.working_directory + "//" + options.mission_name + "_" + options.description + ".emtg";

				double bestJ = EMTG::math::LARGE;
				for (int trial = 0; trial < 100; ++trial)
				{
					solver.Initialize(this);
					solver.GenerateNewPopulation();

					solver.Evolve();

					if (solver.BestConstraintNorm < options.snopt_feasibility_tolerance && solver.BestObjectiveValue < bestJ)
					{
						bestJ = solver.BestObjectiveValue;
						unscale(solver.BestX.data());
						Xopt = X;
						evaluate(&Xopt[0], &F[0], &G[0], 0, iGfun, jGvar);

						this->output();
					}
				}

				break;
			}
		case 4: //run SNOPT on an existing initial guess
			{
				if (!(options.current_trialX.size() == Xdescriptions.size()))
				{
					cout << "Invalid initial guess. trialX vector is the wrong length. Aborting." << endl;
					throw 10000;
				}

				EMTG::Solvers::MBH solver(this);

				solver.seed(options.current_trialX);

				solver.slide();

				unscale(&(solver.Xtrial_scaled[0]));

				Xopt = X;

				options.outputfile = options.working_directory + "//" + options.mission_name + "_" + options.description + ".emtg";
			
				try
				{
					evaluate(&Xopt[0], &F[0], &G[0], 0, iGfun, jGvar);
				}
				catch (int errorcode)
				{
					cout << "EMTG::Invalid initial point or failure in objective function." << endl;
					if (errorcode == 13)
						cout << "EMTG::Integrator failure" << endl;
					F[0] = EMTG::math::LARGE;
				}

				break;
			}
		}

		return 0;
	}


	//function to output X and F bounds, descriptions
	int problem::output_problem_bounds_and_descriptions(string filestring)
	{
		ofstream outputfile(filestring.c_str(), ios::trunc);

		for (size_t k = 0; k < Xdescriptions.size(); ++k)
			outputfile << k << "," << Xdescriptions[k] << "," << Xlowerbounds[k] << "," << Xupperbounds[k] << "," << X[k] << endl;

		for (size_t k = 0; k < Fdescriptions.size(); ++k)
			outputfile << k << "," << Fdescriptions[k] << "," << Flowerbounds[k] << "," << Fupperbounds[k] << "," << F[k] << endl;

		outputfile.close();

		return 0;
	}

	//function to output the Jacobian sparsity information
	int problem::output_Jacobian_sparsity_information(string filestring)
	{
		ofstream outputfile(filestring.c_str(), ios::trunc);

		outputfile <<  "Linear Constraints (A):" << endl;
		for (size_t k = 0; k < Adescriptions.size(); ++k)
			outputfile << iAfun[k] << "," << jAvar[k] << "," << k << "," << Adescriptions[k] << endl;

		outputfile << endl;

		outputfile << "Nonlinear Constraints (G):" << endl;
		for (size_t k = 0; k < Gdescriptions.size(); ++k)
			outputfile << iGfun[k] << "," << jGvar[k] << "," << k << "," << Gdescriptions[k] << endl;

		outputfile.close();

		return 0;
	}

	int problem::check_and_print_derivatives(string filestring)
	{
		//this function checks all of the analytical derivatives via central differencing about the current solution "Xopt"
		vector<double> G_analytical;
		vector<double> X_perturbed;
		vector<double> G_central_differenced(this->Gdescriptions.size());
		vector<double> Fforward(this->Fdescriptions.size());
		vector<double> Fbackward(this->Fdescriptions.size());
		double perturbation_step = 1.0e-6;

		//evaluate the analytical derivatives
		this->evaluate(this->Xopt.data(), this->F.data(), this->G.data(), 1, this->iGfun, this->jGvar);

		G_analytical = this->G;

		ofstream outputfile(filestring.c_str(), ios::trunc);
		outputfile.precision(15);

		//write out a report file in csv format
		//iGfun, jGvar, analytical value, central-difference value, abs(difference), rel. error, sign flip (Y/N), text description of entry
		outputfile << "iGfun, jGvar, analytical value, central-difference value, abs(error), relative error, sign flip(Y/N), text description" << endl;

		//evaluate all of the derivatives via finite differencing, INCLUDING those which were not specified analytically (since we can't tell anyway)
		for (int gIndex = 0; gIndex < Gdescriptions.size(); ++gIndex)
		{
			//reset X_perturbed
			X_perturbed = Xopt;

			//compute the central-difference forward step value of the constraint
			X_perturbed[jGvar[gIndex]] += perturbation_step;
			
			this->evaluate(X_perturbed.data(), Fforward.data(), this->G.data(), 0, this->iGfun, this->jGvar);

			double c_forward = Fforward[iGfun[gIndex]];

			//compute the central-difference backward step value of the constraint
			X_perturbed[jGvar[gIndex]] = Xopt[jGvar[gIndex]];
			X_perturbed[jGvar[gIndex]] -= perturbation_step;

			this->evaluate(X_perturbed.data(), Fbackward.data(), this->G.data(), 0, this->iGfun, this->jGvar);

			double c_backward = Fbackward[iGfun[gIndex]];

			//compute the derivative
			G_central_differenced[gIndex] = (c_forward - c_backward) / (2.0 * perturbation_step) * (Xupperbounds[jGvar[gIndex]] - Xlowerbounds[jGvar[gIndex]]);

			//compute error
			double error = fabs(G_analytical[gIndex] - G_central_differenced[gIndex]);
			double relative_error = error / (G_analytical[gIndex] + 1.0e-20);

			string signflip = "N";
			if ( (G_central_differenced[gIndex] > 0 && G_analytical[gIndex] < 0) || (G_central_differenced[gIndex] < 0 && G_analytical[gIndex] > 0) && !(G_analytical[gIndex] == 0))
				signflip = "Y";
			outputfile << iGfun[gIndex] << "," << jGvar[gIndex] << "," << G_analytical[gIndex] << "," << G_central_differenced[gIndex] << "," << error << "," << relative_error << "," << signflip << "," << Gdescriptions[gIndex] << endl;
		}

		outputfile.close();		

		return 0;
	}

} /* namespace EMTG */

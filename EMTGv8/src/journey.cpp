/*
 * journey.cpp
 *
 *  Created on: Jul 17, 2012
 *      Author: Jacob
 */

#include "phase.h"
#include "MGAphase.h"
#include "MGADSMphase.h"
#include "MGALTphase.h"
#include "FBLTphase.h"
#include "FBLTSphase.h"
#include "MGANDSMphase.h"
#include "journey.h"
#include "missionoptions.h"
#include "universe.h"
#include "EMTG_math.h"

#include <iostream>
#include <sstream>
#include <fstream>

namespace EMTG
{

	journey::journey() :
			number_of_phases(0)
	{
		// default constructor does nothing (is never used)
	}

	journey::journey(missionoptions* options, int j, EMTG::Astrodynamics::universe& Universe)
	{
		//designate a central body
		central_body_name = options->journey_central_body[j];

		//initialize the boundary states array
		vector<double> state_dummy(9);
		boundary_states.push_back(state_dummy);

		//create the phases
		number_of_phases = options->number_of_phases[j];

		for (int p = 0; p < number_of_phases; ++p)
		{
			switch (options->phase_type[j][p])
			{
				case 0:
					{
						//this phase is an MGA phase
						phases.push_back(new MGA_phase(j, p, options));
					}
				break;
				case 1:
					{
						//this phase is an MGA-DSM phase
						phases.push_back(new MGA_DSM_phase(j, p, options));
					}
				break;
				case 2:
					{
						//this phase is an MGA-LT phase
						phases.push_back(new MGA_LT_phase(j, p, options));
					}
				break;
				case 3:
					{
						//this phase is an FBLT phase
						phases.push_back(new FBLT_phase(j, p, options));
					}
				break;
				case 4:
					{
						//this phase is an FBLT-S phase
						phases.push_back(new FBLTS_phase(j, p, options));
					}
				case 5:
					{
						//this phase is an MGA-NDSM phase
						phases.push_back(new MGA_NDSM_phase(j, p, options));
					}
				break;
				case 6:
					{
						//this phase is a DTLT phase
						cout << "DTLT not yet implemented" << endl;
						throw 1711;
					}
				break;
			}

			boundary_states.push_back(state_dummy);
		}

		//which journey am I?
		journey_index = j;

		//initialize the journey initial mass increment multiplier
		journey_initial_mass_increment_scale_factor = 1.0;
	}

	journey::~journey()
	{
		// destructor doesn't need to do anything
	}

	int journey::calcbounds(vector<double>* Xupperbounds, vector<double>* Xlowerbounds, vector<double>* Fupperbounds, vector<double>* Flowerbounds, vector<string>* Xdescriptions, vector<string>* Fdescriptions, vector<int>* iAfun, vector<int>* jAvar, vector<int>* iGfun, vector<int>* jGvar, vector<double>* A, vector<string>* Adescriptions, vector<string>* Gdescriptions, vector<double>* synodic_periods, int j, EMTG::Astrodynamics::universe& Universe, missionoptions* options)
	{
		stringstream prefixstream;
		prefixstream << "j" << j << ": ";
		string prefix = prefixstream.str();
		this->first_X_entry_in_journey = Xupperbounds->size();

		//phase-specific bounds and constraints
		for (int p = 0; p < options->number_of_phases[j]; ++p)
			phases[p].calcbounds(Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, synodic_periods, j, p,  &Universe, options);

		//journey-specific bounds and constraints
	
		//journey time constraints
		if (options->journey_timebounded[j] == 1) //bounded journey flight time
		{
			Flowerbounds->push_back(options->journey_flight_time_bounds[j][0] / options->journey_flight_time_bounds[j][1] - 1);
			Fupperbounds->push_back(0.0);
			Fdescriptions->push_back(prefix + "journey flight time bounds");

			//Generate the Jacobian entries for the journey flight time constraint
			//note:
			//1. Only the time variables present in the current journey affect the journey flight time constraint
			//2. The journey time constraint is linear but we express it as a nonlinear constraint
			for (size_t entry = this->first_X_entry_in_journey; entry < Xdescriptions->size(); ++entry)
			{
				if ((*Xdescriptions)[entry].find("phase flight time") < 1024)
				{
					iGfun->push_back(Fdescriptions->size() - 1);
					jGvar->push_back(entry);
					stringstream EntryNameStream;
					EntryNameStream << "Derivative of journey flight time constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << entry << "]: " << (*Xdescriptions)[entry];
					Gdescriptions->push_back(EntryNameStream.str());
					timeconstraints_G_indices.push_back(iGfun->size() - 1);
					timeconstraints_X_scale_ranges.push_back((*Xupperbounds)[entry] - (*Xlowerbounds)[entry]);
				}
			}

			//check for dependencies due to an escape spiral in this or a previous phase
			this->find_dependencies_due_to_escape_spiral(Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, options, Fdescriptions->size() - 1);
			this->find_dependencies_due_to_capture_spiral(Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, options, Fdescriptions->size() - 1);
		}
		if (options->journey_timebounded[j] == 2) //bounded journey arrival date
		{
			Flowerbounds->push_back(options->journey_arrival_date_bounds[j][0] / options->journey_arrival_date_bounds[j][1] - 1);
			Fupperbounds->push_back(0.0);
			Fdescriptions->push_back(prefix + "journey arrival date bounds");

			//Generate the Jacobian entries for the journey arrival date constraint
			//note:
			//1. Only the time variables present in the current journey affect the arrival date constraint
			//2. The journey arrival date constraint is linear but we express it as a nonlinear constraint
			for (size_t entry = 0; entry < Xdescriptions->size(); ++entry)
			{
				if ((*Xdescriptions)[entry].find("time") < 1024)
				{
					iGfun->push_back(Fdescriptions->size() - 1);
					jGvar->push_back(entry);
					stringstream EntryNameStream;
					EntryNameStream << "Derivative of journey arrival date constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << entry << "]: " << (*Xdescriptions)[entry];
					Gdescriptions->push_back(EntryNameStream.str());
					timeconstraints_G_indices.push_back(iGfun->size() - 1);
					timeconstraints_X_scale_ranges.push_back((*Xupperbounds)[entry] - (*Xlowerbounds)[entry]);
				}
				else if ((*Xdescriptions)[entry].find("epoch") < 1024)
				{
					iGfun->push_back(Fdescriptions->size() - 1);
					jGvar->push_back(entry);
					stringstream EntryNameStream;
					EntryNameStream << "Derivative of journey arrival date constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << entry << "]: " << (*Xdescriptions)[entry];
					Gdescriptions->push_back(EntryNameStream.str());
					timeconstraints_G_indices.push_back(iGfun->size() - 1);
					timeconstraints_X_scale_ranges.push_back((*Xupperbounds)[entry] - (*Xlowerbounds)[entry]);
				}
			}

			//check for dependencies due to an escape spiral in this or a previous phase
			this->find_dependencies_due_to_escape_spiral(Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, options, Fdescriptions->size() - 1);
			this->find_dependencies_due_to_capture_spiral(Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, options, Fdescriptions->size() - 1);
		}

		return 0;
	}

	//evaluate function
	//return 0 if successful, 1 if failure
	int journey::evaluate(double* X, int* Xindex, double* F, int* Findex, double* G, int* Gindex, int needG, int j, double* current_epoch, double* current_state, double* current_deltaV, EMTG::Astrodynamics::universe& Universe, missionoptions* options)
	{
		int errcode = 0;

		//journey start epoch is equal to the current epoch plus whatever the journey initial wait time is going to be
		//if the journey starts with a flyby (i.e. after a terminal intercept in the previous journey) then there is no wait time
		//for the first journey, current epoch is zero, so one must take the launch date as the start epoch
		double journey_start_epoch;
		double central_body_state[6];

		if (j == 0)
			journey_start_epoch = X[0];
		else if (options->journey_departure_type[j] == 3)
			journey_start_epoch =  *current_epoch;
		else
			journey_start_epoch =  *current_epoch + X[*Xindex];

		//if applicable, vary the journey initial mass increment
		if (options->journey_starting_mass_increment[j] > 0.0 && options->journey_variable_mass_increment[j])
		{
			journey_initial_mass_increment_scale_factor = X[*Xindex];
			++(*Xindex);
		}	

		//process all of the phases
		for (int p = 0; p < number_of_phases; ++p)
		{
			if (!(errcode))
			{
				//if this is NOT the first journey, transform the coordinate system into the central body frame of the current journey
				if (j > 0)
				{
					Universe.locate_central_body(*current_epoch, central_body_state, options);
					for (int k = 0; k < 6; ++k)
						current_state[k] -= central_body_state[k];
				}

				if (options->journey_starting_mass_increment[j] > 0.0 && options->journey_variable_mass_increment[j])
				{
					phases[p].journey_initial_mass_increment_scale_factor = journey_initial_mass_increment_scale_factor;
				}	

				errcode = phases[p].evaluate(X, Xindex, F, Findex, G, Gindex, needG, current_epoch, current_state, current_deltaV, boundary_states[p].data(), boundary_states[p+1].data(), j, p, &Universe, options);	

				//at the end of the journey, transform the coordinate system back into the central body frame of the Sun (EMTG global reference frame)
				Universe.locate_central_body(*current_epoch, central_body_state, options);
				for (int k = 0; k < 6; ++k)
					current_state[k] += central_body_state[k];
			}
			else
			{
				std::cout << "error " << errcode << std::endl;
				break;
			}
		}

		//journey-level constraints
		//journey time constraints
		if (options->journey_timebounded[j])
		{
			double journey_end_epoch = *current_epoch;
			if (options->journey_timebounded[j] == 1) //bounded flight time
			{
				F[*Findex] = (journey_end_epoch - journey_start_epoch) / options->journey_flight_time_bounds[j][1] - 1;
				++(*Findex);

				if (options->derivative_type > 0 && needG)
				{
					for (size_t entry = 0; entry < timeconstraints_G_indices.size(); ++entry)
					{
						G[timeconstraints_G_indices[entry]] = timeconstraints_X_scale_ranges[entry] / options->journey_flight_time_bounds[j][1];
					}
				}
			}
			else
			{
				F[*Findex] = journey_end_epoch / options->journey_arrival_date_bounds[j][1] - 1;
				++(*Findex);

				if (options->derivative_type > 0 && needG)
				{
					//the first n-1 derivatives are with respect to flight times
					for (size_t entry = 0; entry < timeconstraints_G_indices.size(); ++entry)
					{
						G[timeconstraints_G_indices[entry]] = timeconstraints_X_scale_ranges[entry] / options->journey_arrival_date_bounds[j][1];
					}
				}
			}
		}
		return 0;
	}

	//output function
	//return 0 if successful, 1 if failure
	int journey::output(missionoptions* options, const double& launchdate, int j, EMTG::Astrodynamics::universe& Universe, int* eventcount)
	{
		int errcode = 0;

		//first output a bunch of header stuff
		ofstream outputfile;
		outputfile.open (options->outputfile.c_str(), ios::out | ios::app);

		vector<string> phase_type_codes;
		phase_type_codes.push_back("MGA");
		phase_type_codes.push_back("MGA-DSM");
		phase_type_codes.push_back("MGA-LT");
		phase_type_codes.push_back("FBLT");

		outputfile << endl;
		outputfile << "Journey: " << j+1 << endl;
		outputfile << "Journey name: " << options->journey_names[j] << endl;
		outputfile << "Central Body: " << central_body_name << endl;
		outputfile << "Radius (km): " << Universe.central_body_radius << endl;
		outputfile << "mu (km^2/s^3): " << Universe.mu << endl;
		outputfile << "Characteristic length unit (km): " << Universe.LU << endl;

		if (options->mission_type == 2) //MGALT
			outputfile << "Thruster duty cycle: " << options->engine_duty_cycle << endl;
		outputfile << endl;

		//next, column headers
		
		//column headers line 1
		outputfile.width(5); outputfile << "#";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(16); outputfile << "JulianDate";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(11); outputfile << "MM/DD/YYYY";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(12); outputfile << "event type";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(25); outputfile << "location";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(15); outputfile << "step size";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "altitude";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "BdotR";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "BdotT";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(8); outputfile << "RA";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(8); outputfile << "DEC";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "C3";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " x";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " y";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " z";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " xdot";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " ydot";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " zdot";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " dV_x";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " dV_y";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " dV_z";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " T_x";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " T_y";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << " T_z";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(17); outputfile << "|dV| (km/s)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "Avail. Thrust";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "Isp";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "Avail. Power";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "Mass Flow";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "mass";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "number of";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "active power";
		outputfile << endl;

		//column headers line 2
		outputfile.width(5); outputfile << "";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(16); outputfile << " STK: JED";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(11); outputfile << "";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(12); outputfile << "";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(25); outputfile << "";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(15); outputfile << "(days)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "(km)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "(km)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "(km)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(8); outputfile << "degrees";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(8); outputfile << "degrees";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "(km^2/s^2)";
		outputfile.width(3); outputfile << " | ";
		if (options->output_units == 0)
		{
			outputfile.width(19); outputfile << "(km)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km/s)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km/s)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km/s)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km/s)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km/s)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(km/s)";
			outputfile.width(3); outputfile << " | ";
		}
		else if (options->output_units == 1)
		{
			outputfile.width(19); outputfile << "(LU)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU/day)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU/day)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU/day)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU/day)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU/day)";
			outputfile.width(3); outputfile << " | ";
			outputfile.width(19); outputfile << "(LU/day)";
			outputfile.width(3); outputfile << " | ";
		}
		outputfile.width(19); outputfile << "(N)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "(N)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "(N)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(17); outputfile << "throttle (0-1)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "(N)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "(s)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "(kW)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(19); outputfile << "rate (kg/s)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "(kg)";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "active engines";
		outputfile.width(3); outputfile << " | ";
		outputfile.width(14); outputfile << "(kW)";
		outputfile << endl;
	

		for (int k = 0; k < 615; ++k)
			outputfile << "-";
		outputfile << endl;

		outputfile.close();

		for (int p = 0; p < number_of_phases; ++p)
		{
			errcode = phases[p].output(options, launchdate, j, p, &Universe, eventcount);
			if (!(errcode == 0))
				return errcode;
		}

		//print journey end information

		outputfile.open (options->outputfile.c_str(), ios::out | ios::app);

		//skip two lines
		outputfile << endl;
		outputfile << endl;

		//print the journey-end v-infinity in ICRF frame
		outputfile << "Journey-end v-infinity in ICRF frame: " << phases[options->number_of_phases[j] - 1].dVarrival[0] << " " << phases[options->number_of_phases[j] - 1].dVarrival[1] << " " << phases[options->number_of_phases[j] - 1].dVarrival[2] << endl;
		outputfile << "Journey mass increment: " << phases[options->number_of_phases[j] - 1].current_mass_increment * phases[options->number_of_phases[j] - 1].journey_initial_mass_increment_scale_factor << " kg" << endl;
	
		//**************************************************************************************************************
		//print the boundary states
		//create a 3-element storage vector that will be used every time something needs to be rotated to the local frame
		math::Matrix<double> r(3,1);
		math::Matrix<double> v(3,1);
		math::Matrix<double> disp_r(3,1);
		math::Matrix<double> disp_v(3,1);

		//construct the rotation matrix from ICRF to the local frame
		Universe.LocalFrame.construct_rotation_matrices(phases[0].phase_start_epoch + 2400000.5);
		outputfile << "Boundary states:" << endl;
		for (int i = 0; i < number_of_phases + 1; ++i)
		{
			outputfile << "Boundary: " << i+1;
			r(0) = boundary_states[i][0];
			r(1) = boundary_states[i][1];
			r(2) = boundary_states[i][2];
			v(0) = boundary_states[i][3];
			v(1) = boundary_states[i][4];
			v(2) = boundary_states[i][5];
			disp_r = Universe.LocalFrame.R_from_ICRF_to_local * r;
			disp_v = Universe.LocalFrame.R_from_ICRF_to_local * v;
			for (int j = 0; j < 3; ++j)
				outputfile << " " << disp_r(j);
			for (int j = 0; j < 3; ++j)
				outputfile << " " << disp_v(j);
			outputfile << endl;
		}

		//**************************************************************************************************************
		//output the flyby periapse states
		if (options->mission_type > 0 && options->number_of_phases[j] > 1)
		{
			outputfile << endl;
			outputfile << "Flyby periapse states (body-centered J2000 Earth Equatorial Frame):" << endl;

			//declare a vector to hold the periapse state
			math::Matrix<double> periapse_state(6,1);

			for (int p = 1; p < options->number_of_phases[j]; ++p)
			{
				//calculate the periapse state for a symmetric flyby
				periapse_state = this->phases[p].calculate_flyby_periapse_state(this->phases[p].V_infinity_in, this->phases[p].V_infinity_out, this->phases[p].flyby_altitude, *this->phases[p].Body1);

				//print the periapse state
				outputfile << "Flyby: " << p << " (" << this->phases[p].Body1->name << ")";
				for (int k = 0; k < 6; ++k)
					outputfile << " " << periapse_state(k);
				outputfile << endl;
			}
		}

		//skip two lines
		outputfile << endl;
		outputfile << endl;

		outputfile << "End journey" << endl;

		outputfile.close();
	

		return 0;
	}

	//function to find constraint dependecies due to an escape spiral in this or a previous journey
	void journey::find_dependencies_due_to_escape_spiral(vector<double>* Xupperbounds,
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
														int j,
														missionoptions* options,
														const int& Findex)
{
		//loop over journeys
		for (int jj = 0; jj <= j; ++jj)
		{
			//we don't have any dependencies if there is no escape spiral in that journey
			if (options->journey_departure_type[jj] == 5)
			{
				//loop over all variables in the decision vector and determine the first entry in the journey of interest
				int first_entry_in_jj;
				stringstream pjprefix_stream;
				pjprefix_stream << "j" << jj << "p";
				string pjprefix = pjprefix_stream.str();
				for (int Xentry = 0; Xentry < Xdescriptions->size() - 1; ++Xentry)
				{
					if ( (*Xdescriptions)[Xentry].find(pjprefix) < 1024)
					{
						first_entry_in_jj = Xentry;
						break;
					}
				}//end loop over all variables in the decision vector

				//this constraint has a derivative with respect to the mass at the beginning of any journey that has an escape spiral
				//therefore we have derivatives with respect to:
				//1. if the first journey, the initial mass scale factor if enabled
				if (jj == 0 && options->allow_initial_mass_to_vary)
				{
					for (int Xentry = 0; Xentry < Xdescriptions->size() - 1; ++Xentry)
					{
						if ( (*Xdescriptions)[Xentry].find("initial mass multiplier (0-1)") < 1024)
						{
							//first check for duplicates
							bool duplicateflag = false;
							stringstream entry_tag_stream;
							entry_tag_stream << "F[" << Findex << "] with respect to X[" << Xentry << "]";
							for (int Gentry = Gdescriptions->size()-1; Gentry >=0; ++Gentry)
							{
								if ( (*Gdescriptions)[Gentry].find(entry_tag_stream.str()) < 1024)
								{
									duplicateflag = true;
									break;
								}
							}
							if (!duplicateflag)
							{
								iGfun->push_back(Findex);
								jGvar->push_back(Xentry);
								stringstream EntryNameStream;
								EntryNameStream << "Derivative of " << (*Fdescriptions)[Findex] << " F[" << Fdescriptions->size() - 1 << "] with respect to X[" << Xentry << "]: " << (*Xdescriptions)[Xentry];
								Gdescriptions->push_back(EntryNameStream.str());
								break;
							}
						}
					}
				}
				//2. if NOT the first journey, the arrival mass at the end of the previous journey
				if (jj > 0)
				{
					for (int Xentry = first_entry_in_jj; Xentry > 0; --Xentry)
					{
						if ( (*Xdescriptions)[Xentry].find("arrival mass") < 1024)
						{
							//first check for duplicates
							bool duplicateflag = false;
							stringstream entry_tag_stream;
							entry_tag_stream << "F[" << Findex << "] with respect to X[" << Xentry << "]";
							for (int Gentry = Gdescriptions->size()-1; Gentry >=0; --Gentry)
							{
								if ( (*Gdescriptions)[Gentry].find(entry_tag_stream.str()) < 1024)
								{
									duplicateflag = true;
									break;
								}
							}
							if (!duplicateflag)
							{
								iGfun->push_back(Findex);
								jGvar->push_back(Xentry);
								stringstream EntryNameStream;
								EntryNameStream << "Derivative of " << (*Fdescriptions)[Findex] << " F[" << Fdescriptions->size() - 1 << "] with respect to X[" << Xentry << "]: " << (*Xdescriptions)[Xentry];
								Gdescriptions->push_back(EntryNameStream.str());
								break;
							}
						}
					}
				}
				//3. if present, the journey initial mass increment scale factor
				//we must first check for duplicates associated with the current constraint, because this can occur
				
				for (int Xentry = first_entry_in_jj; Xentry < Xdescriptions->size() - 1; ++Xentry)
				{
					if ( (*Xdescriptions)[Xentry].find("journey initial mass scale factor") < 1024)
					{
						bool duplicateflag = false;
						stringstream entry_tag_stream;
						entry_tag_stream << "F[" << Findex << "] with respect to X[" << Xentry << "]";
						for (int Gentry = Gdescriptions->size()-1; Gentry >=0; --Gentry)
						{
							if ( (*Gdescriptions)[Gentry].find(entry_tag_stream.str()) < 1024)
							{
								duplicateflag = true;
								break;
							}
						}
						if (!duplicateflag)
						{
							iGfun->push_back(Findex);
							jGvar->push_back(Xentry);
							stringstream EntryNameStream;
							EntryNameStream << "Derivative of " << (*Fdescriptions)[Findex] << " constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << Xentry << "]: " << (*Xdescriptions)[Xentry];
							Gdescriptions->push_back(EntryNameStream.str());
							break;
						}
					}

				}

				//this constraint has a derivative with respect to the Isp at the beginning of any journey that has an escape spiral
				if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
				{
					for (int Xentry = first_entry_in_jj; Xentry < Xdescriptions->size() - 1; ++Xentry)
					{
						if ( (*Xdescriptions)[Xentry].find("Escape spiral Isp") < 1024)
						{
							//first check for duplicates
							bool duplicateflag = false;
							stringstream entry_tag_stream;
							entry_tag_stream << "F[" << Findex << "] with respect to X[" << Xentry << "]";
							for (int Gentry = Gdescriptions->size()-1; Gentry >=0; --Gentry)
							{
								if ( (*Gdescriptions)[Gentry].find(entry_tag_stream.str()) < 1024)
								{
									duplicateflag = true;
									break;
								}
							}
							if (!duplicateflag)
							{
								iGfun->push_back(Fdescriptions->size() - 1);
								jGvar->push_back(Xentry);
								stringstream EntryNameStream;
								EntryNameStream << "Derivative of " << (*Fdescriptions)[Findex] << " F[" << Fdescriptions->size() - 1 << "] with respect to X[" << Xentry << "]: " << (*Xdescriptions)[Xentry];
								Gdescriptions->push_back(EntryNameStream.str());
								break;
							}
						}
					}
				}	
			}
		}//end loop over journeys
	}

	//function to find constraint dependecies due to an capture spiral in this or a previous journey
	void journey::find_dependencies_due_to_capture_spiral(vector<double>* Xupperbounds,
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
														int j,
														missionoptions* options,
														const int& Findex)
{
		//loop over journeys
		for (int jj = 0; jj <= j; ++jj)
		{
			//we have a dependency on the arrival mass and the capture spiral Isp (if applicable) for each preceding journey that has a capture spiral
			if (options->journey_arrival_type[jj] == 7)
			{
				//loop over all variables in the decision vector and determine the first entry in the journey of interest
				int last_entry_in_jj;
				stringstream pjprefix_stream;
				pjprefix_stream << "j" << jj << "p";
				string pjprefix = pjprefix_stream.str();
				for (int Xentry = Xdescriptions->size() - 1; Xentry > 0 ; --Xentry)
				{
					if ( (*Xdescriptions)[Xentry].find(pjprefix) < 1024)
					{
						last_entry_in_jj = Xentry;
						break;
					}
				}//end loop over all variables in the decision vector

				//this constraint has a derivative with respect to the arrival mass at the end of any journey that has a capture spiral
				for (int Xentry = last_entry_in_jj; Xentry > 0; --Xentry)
				{
					if ( (*Xdescriptions)[Xentry].find("arrival mass") < 1024)
					{
						//first check for duplicates
						bool duplicateflag = false;
						stringstream entry_tag_stream;
						entry_tag_stream << "F[" << Findex << "] with respect to X[" << Xentry << "]";
						for (int Gentry = Gdescriptions->size()-1; Gentry >=0; --Gentry)
						{
							if ( (*Gdescriptions)[Gentry].find(entry_tag_stream.str()) < 1024)
							{
								duplicateflag = true;
								break;
							}
						}
						if (!duplicateflag)
						{
							iGfun->push_back(Findex);
							jGvar->push_back(Xentry);
							stringstream EntryNameStream;
							EntryNameStream << "Derivative of " << (*Fdescriptions)[Findex] << " F[" << Findex << "] with respect to X[" << Xentry << "]: " << (*Xdescriptions)[Xentry];
							Gdescriptions->push_back(EntryNameStream.str());
							break;
						}
					}
				}
				
				//this constraint has a derivative with respect to the Isp at the beginning of any journey that has a capture spiral
				if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
				{
					for (int Xentry = last_entry_in_jj; Xentry < Xdescriptions->size() - 1; ++Xentry)
					{
						if ( (*Xdescriptions)[Xentry].find("Capture spiral Isp") < 1024)
						{
							//first check for duplicates
							bool duplicateflag = false;
							stringstream entry_tag_stream;
							entry_tag_stream << "F[" << Findex << "] with respect to X[" << Xentry << "]";
							for (int Gentry = Gdescriptions->size()-1; Gentry >=0; ++Gentry)
							{
								if ( (*Gdescriptions)[Gentry].find(entry_tag_stream.str()) < 1024)
								{
									duplicateflag = true;
									break;
								}
							}
							if (!duplicateflag)
							{
								iGfun->push_back(Findex);
								jGvar->push_back(Xentry);
								stringstream EntryNameStream;
								EntryNameStream << "Derivative of " << (*Fdescriptions)[Findex] << " F[" << Fdescriptions->size() - 1 << "] with respect to X[" << Xentry << "]: " << (*Xdescriptions)[Xentry];
								Gdescriptions->push_back(EntryNameStream.str());
								break;
							}
						}
					}
				}
			}
		}//end loop over journeys
	}

} /* namespace EMTG */

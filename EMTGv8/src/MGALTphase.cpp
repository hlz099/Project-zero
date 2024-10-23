/*
 * MGALTphase.cpp
 *
 *  Created on: Jul 15, 2012
 *      Author: Jacob
 */

#include "MGALTphase.h"
#include "Astrodynamics.h"
#include "missionoptions.h"
#include "mjd_to_mdyhms.h"
#include "EMTG_math.h"
#include "universe.h"
#include "EMTG_Matrix.h"
#include "UniversalKeplerPropagator.h"
#include "kepler_lagrange_laguerre_conway.h"

#include "SpiceUsr.h"

#include <sstream>
#include <fstream>

namespace EMTG {

	MGA_LT_phase::MGA_LT_phase() {
	//default constructor does nothing
	}

	MGA_LT_phase::MGA_LT_phase(int j, int p, missionoptions* options) {

		//must resize all data vectors to the correct length
		vector<double> state_dummy(7);
		vector<double> dV_or_control_dummy(3);

		for (int step = 0; step < options->num_timesteps; ++step) {
			spacecraft_state.push_back(state_dummy);
			dV.push_back(dV_or_control_dummy);
			control.push_back(dV_or_control_dummy);
			ForceVector.push_back(dV_or_control_dummy);
			dagravdRvec.push_back(dV_or_control_dummy);
			dagravdtvec.push_back(dV_or_control_dummy);
		}

		match_point_state.resize(7);

		event_epochs.resize(options->num_timesteps);
		dVmax.resize(options->num_timesteps);
		available_power.resize(options->num_timesteps);
		available_mass_flow_rate.resize(options->num_timesteps);
		available_thrust.resize(options->num_timesteps);
		available_Isp.resize(options->num_timesteps);
		active_power.resize(options->num_timesteps);
		number_of_active_engines.resize(options->num_timesteps);
		throttle.resize(options->num_timesteps);

		//size the vectors that will be used to calculate the b-plane
		V_infinity_in.resize(3, 1);
		V_infinity_out.resize(3, 1);
		BoundaryR.resize(3, 1);
		BoundaryV.resize(3, 1);

		//set the bodies
		boundary1_location_code = options->sequence[j][p];
		boundary2_location_code = options->sequence[j][p+1];

		//size the vectors of state transition matrices
		Forward_STM.resize(options->num_timesteps/2 + 1);
		Backward_STM.resize(options->num_timesteps/2 + 1);

		current_mass_increment = 0.0;
		journey_initial_mass_increment_scale_factor = 1.0;

		//size the time step vector
		time_step_sizes.resize(options->num_timesteps);

		//size the vectors necessary to compute the patch-point derivatives
		if (p == 0 && options->allow_initial_mass_to_vary)
			G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier.resize(7);

		if (options->journey_variable_mass_increment[j])
			G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier.resize(7);

		dTdP.resize(options->num_timesteps);
		dmdotdP.resize(options->num_timesteps);
		dTdIsp.resize(options->num_timesteps);
		dmdotdIsp.resize(options->num_timesteps);
		dPdr.resize(options->num_timesteps);
		dPdt.resize(options->num_timesteps);
		dFSRPdr.resize(options->num_timesteps);

		//set derivatives for spirals
		this->spiral_escape_dm_after_dm_before = 1.0;

	}

	MGA_LT_phase::~MGA_LT_phase() {
		//destructor doesn't have to do anything
	}

	//evaluate function
	//return 0 if successful, 1 if failure
	int MGA_LT_phase::evaluate(double* X, int* Xindex, double* F, int* Findex, double* G, int* Gindex, int needG, double* current_epoch, double* current_state, double* current_deltaV, double* boundary1_state, double* boundary2_state, int j, int p, EMTG::Astrodynamics::universe* Universe, missionoptions* options) 
	{
		//declare some local variables
		int errcode = 0;
		double local_throttle, impulse_magnitude;

		//******************************************************************
		//Steps 1-4: Process the left boundary condition
		process_left_boundary_condition(X, Xindex, F, Findex, G, Gindex, needG, current_epoch, current_state, current_deltaV, boundary1_state, boundary2_state, j, p, Universe, options);
	
		//******************************************************************
		//Step 5: For MGA-LT, we need to know the state of the spacecraft at the right hand side (end) of the phase in order to propagate backward
		process_right_boundary_condition(X, Xindex, F, Findex, G, Gindex, needG, current_epoch, current_state, current_deltaV, boundary1_state, boundary2_state, j, p, Universe, options);

		//******************************************************************
		//Step 6: thrust and propagate forward and back

		//Step 6.1: determine the length the timesteps
		double step_size_normalization_coefficient = 0.0;
		if (options->step_size_distribution == 3 || options->step_size_distribution == 4)
		{
			time_step_distribution_scale_or_stdv = X[*Xindex];
			++(*Xindex);
		}
		else
			time_step_distribution_scale_or_stdv = options->step_size_stdv_or_scale;

		for (int step = 0; step < options->num_timesteps; ++step)
		{
			time_step_sizes[step] = compute_timestep_width_from_distribution(step + 0.5, options, time_step_distribution_scale_or_stdv);
			step_size_normalization_coefficient += time_step_sizes[step];
		}

		double total_available_thrust_time = TOF;
		
		if (j == 0 && p == 0 && options->forced_post_launch_coast > 1.0e-6)
			total_available_thrust_time -= options->forced_post_launch_coast;
		else if ( (p > 0 || p == 0 && (options->journey_departure_type[j] == 3 || options->journey_departure_type[j] == 4) ) && options->forced_flyby_coast > 1.0e-6)
			total_available_thrust_time -= options->forced_flyby_coast;
		if ( (p < options->number_of_phases[j] - 1 ||  (options->journey_arrival_type[j] == 2 || options->journey_arrival_type[j] == 5) ) && options->forced_flyby_coast > 1.0e-6)
			total_available_thrust_time -= options->forced_flyby_coast;

		for (int step = 0; step < options->num_timesteps; ++step)
		{
			time_step_sizes[step] *= total_available_thrust_time / step_size_normalization_coefficient;
		}

		//Step 6.2: propagate forward
		phase_time_elapsed_forward = 0.0;
		//first initialize the forward integration
		//the following array holds the spacecraft state at "half steps," i.e. integration steps where a burn is not applied
		double spacecraft_state_forward[7];

		for (int k = 0; k < 7; ++k)
			spacecraft_state_forward[k] = state_at_beginning_of_phase[k];

		//Step 6.2.0.1 propagate forward over the first half-step
		if (j == 0 && p == 0 && options->forced_post_launch_coast > 1.0e-6)
		{
			//if this is a launch AND we are doing a forced post-launch initial coast
			Kepler::KeplerLagrangeLaguerreConway(spacecraft_state_forward, spacecraft_state[0].data(), Universe->mu, time_step_sizes[0]*86400/2.0 + options->forced_post_launch_coast*86400, Forward_STM[0], (options->derivative_type > 1 && needG ? true : false));
			phase_time_elapsed_forward += time_step_sizes[0]/2.0 + options->forced_post_launch_coast;
		}
		else if ( (p > 0 || p == 0 && (options->journey_departure_type[j] == 3 || options->journey_departure_type[j] == 4) ) && options->forced_flyby_coast > 1.0e-6)
		{
			//if we are coming out of a flyby and we are doing a forced post-flyby coast
			Kepler::KeplerLagrangeLaguerreConway(spacecraft_state_forward, spacecraft_state[0].data(), Universe->mu, time_step_sizes[0]*86400/2.0 + options->forced_flyby_coast*86400, Forward_STM[0], (options->derivative_type > 1 && needG ? true : false));
			phase_time_elapsed_forward += time_step_sizes[0]/2.0 + options->forced_flyby_coast;
		}
		else
		{
			//if there is no forced initial coast, business as usual
			Kepler::KeplerLagrangeLaguerreConway(spacecraft_state_forward, spacecraft_state[0].data(), Universe->mu, time_step_sizes[0]*86400/2.0, Forward_STM[0], (options->derivative_type > 1 && needG ? true : false));
			phase_time_elapsed_forward += time_step_sizes[0]/2.0;
		}
		
		
	

		for (int step = 0; step < options->num_timesteps/2; ++step)
		{
			//step 6.2.2 get the current mass
			if (step == 0)
				spacecraft_state[step][6] = spacecraft_state_forward[6];
			else
				spacecraft_state[step][6] = spacecraft_state[step-1][6];

			//step 6.2.3 extract the burn parameters from the decision vector
			control[step][0] = X[*Xindex];
			control[step][1] = X[*Xindex+1];
			control[step][2] = X[*Xindex+2];
			(*Xindex) += 3;

			//extract the specific impulse for this step (VSI only)
			if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
			{
				available_Isp[step] = X[*Xindex];
				++(*Xindex);
			}
			//"mass leak" throttle of 1.0e-10 to ensure that we always have at least some thrusting (prevents derivative from being zero)
			local_throttle = math::norm(control[step].data(), 3) + 1.0e-10;
			throttle[step] = local_throttle;
			if (options->derivative_type > 1 && needG)
			{
				G[control_vector_G_indices[step][0]] = 2.0 * control[step][2] / local_throttle;
				G[control_vector_G_indices[step][1]] = 2.0 * control[step][1] / local_throttle;
				G[control_vector_G_indices[step][2]] = 2.0 * control[step][0] / local_throttle;
				(*Gindex) += 3;
			}

			F[*Findex] = local_throttle;
			++(*Findex);
							
			//step 6.2.4 encode the burn epoch
			event_epochs[step] = phase_start_epoch + phase_time_elapsed_forward;

			//step 6.2.5 determine the maximum size of the burn
			//it is calculated as "thrust / mass * time"
			EMTG::Astrodynamics::force_model(options,
				Universe,
				spacecraft_state[step].data(),
				&event_epochs[step],
				X,
				control[step].data(),
				&available_thrust[step],
				&available_mass_flow_rate[step],
				&available_Isp[step],
				&available_power[step],
				&active_power[step],
				&number_of_active_engines[step],
				ForceVector[step].data(),
				(options->derivative_type > 1 && needG ? 1 : 0),
				&dTdP[step],
				&dmdotdP[step],
				&dTdIsp[step],
				&dmdotdIsp[step],
				&dPdr[step],
				&dPdt[step],
				&dFSRPdr[step],
				dagravdRvec[step],
				dagravdtvec[step]);

			dVmax[step] = options->engine_duty_cycle * available_thrust[step] / spacecraft_state[step][6] * (time_step_sizes[step]*86400) / 1000;

			//step 6.2.6 apply the burn
			for (size_t k = 0; k < 3; ++k)
			{
				dV[step][k] = ForceVector[step][k] / spacecraft_state[step][6] * (time_step_sizes[step]*86400) / 1000;
				spacecraft_state[step][k+3] += dV[step][k];
			}
			impulse_magnitude = local_throttle * dVmax[step];
			*current_deltaV += impulse_magnitude;

			//step 6.2.7 determine the propagation time and propagate
			double propagation_time;
			if (step == options->num_timesteps / 2 - 1)
			{
				//on the last step the propagation time is only half of the current timestep
				propagation_time = time_step_sizes[step] / 2.0;
				Kepler::KeplerLagrangeLaguerreConway(spacecraft_state[step].data(), spacecraft_state_forward, Universe->mu, propagation_time*86400, Forward_STM[step+1], (options->derivative_type > 1 && needG ? true : false));
			}
			else
			{
				//on all other steps the propagation time is half of the current step plus half of the next step
				propagation_time = (time_step_sizes[step] + time_step_sizes[step + 1]) / 2.0;
				Kepler::KeplerLagrangeLaguerreConway(spacecraft_state[step].data(), spacecraft_state[step+1].data(), Universe->mu, propagation_time*86400, Forward_STM[step+1], (options->derivative_type > 1 && needG ? true : false));
			}
			phase_time_elapsed_forward += propagation_time;

			//step 6.2.8 update the spacecraft mass
			spacecraft_state[step][6] -= local_throttle * options->engine_duty_cycle * available_mass_flow_rate[step] * (time_step_sizes[step]*86400);

		}
	
		//Step 6.2.9 copy the final forward propagation mass to the match point
		spacecraft_state_forward[6] = spacecraft_state[options->num_timesteps / 2 - 1][6];

		//Step 6.3: propagate backward
		phase_time_elapsed_backward = 0.0;
		//first initialize the backward integration
		//the following array holds the spacecraft state at "half steps," i.e. integration steps where a burn is not applied
		double spacecraft_state_backward[7];
		for (int k = 0; k < 7; ++k)
			spacecraft_state_backward[k] = state_at_end_of_phase[k];

		//Step 6.3.1 propagate backward over the first half-step
		if ( (p < options->number_of_phases[j] - 1 ||  (options->journey_arrival_type[j] == 2 || options->journey_arrival_type[j] == 5) ) && options->forced_flyby_coast > 1.0e-6)
		{
			//if we are going into a flyby or intercept
			Kepler::KeplerLagrangeLaguerreConway(spacecraft_state_backward, spacecraft_state[options->num_timesteps-1].data(), Universe->mu, -(time_step_sizes[options->num_timesteps-1] / 2.0 + options->forced_flyby_coast)* 86400, Backward_STM[0], (options->derivative_type > 1 && needG ? true : false));
			phase_time_elapsed_backward += time_step_sizes[0]/2.0 + options->forced_flyby_coast;
		}
		else
		{
			//if there is no forced terminal coast, business as usual
			Kepler::KeplerLagrangeLaguerreConway(spacecraft_state_backward, spacecraft_state[options->num_timesteps-1].data(), Universe->mu, -time_step_sizes[options->num_timesteps-1]*86400/2.0, Backward_STM[0], (options->derivative_type > 1 && needG ? true : false));
			phase_time_elapsed_backward += time_step_sizes[options->num_timesteps-1]/2.0;
		}

		
	

		for (int step = 0; step < options->num_timesteps/2; ++step)
		{
			//translate into backward steps
			int backstep = options->num_timesteps - 1 - step;

			//step 6.3.2 get the current mass
			if (step == 0)
				spacecraft_state[backstep][6] = spacecraft_state_backward[6];
			else
				spacecraft_state[backstep][6] = spacecraft_state[backstep+1][6];

			//step 6.3.3 extract the burn parameters from the decision vector
			//extract the specific impulse for this step (VSI only)
			if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
			{
				control[backstep][0] = X[*Xindex+4*(backstep - options->num_timesteps/2)];
				control[backstep][1] = X[*Xindex+1+4*(backstep - options->num_timesteps/2)];
				control[backstep][2] = X[*Xindex+2+4*(backstep - options->num_timesteps/2)];
				available_Isp[backstep] = X[*Xindex+3+4*(backstep - options->num_timesteps/2)];
			}
			else
			{
				control[backstep][0] = X[*Xindex+3*(backstep - options->num_timesteps/2)];
				control[backstep][1] = X[*Xindex+1+3*(backstep - options->num_timesteps/2)];
				control[backstep][2] = X[*Xindex+2+3*(backstep - options->num_timesteps/2)];
			}

			//"mass leak" throttle of 1.0e-10 to ensure that we always have at least some thrusting (prevents derivative from being zero)
			local_throttle = math::norm(control[backstep].data(), 3) + 1.0e-10;
			throttle[backstep] = local_throttle;
			if (options->derivative_type > 0 && needG)
			{
				G[control_vector_G_indices[backstep][0]] = 2.0 * control[backstep][2] / local_throttle;
				G[control_vector_G_indices[backstep][1]] = 2.0 * control[backstep][1] / local_throttle;
				G[control_vector_G_indices[backstep][2]] = 2.0 * control[backstep][0] / local_throttle;
				(*Gindex) += 3;
			}
			F[*Findex + (backstep - options->num_timesteps/2)] = local_throttle;

			//step 6.3.4 encode the burn epoch
			event_epochs[backstep] = phase_end_epoch - phase_time_elapsed_backward;

						
			//step 6.3.5 determine the maximum size of the burn
			//it is calculated as "thrust / mass * time"
			//what is the mass at the burn point?
			//the maximum deltaV corresponds to the maximum mass flow rate
			//so m(-) = m(+) + throttle * mdot_max * deltaT

			EMTG::Astrodynamics::force_model(options, 
											Universe,
											spacecraft_state[backstep].data(),
											&event_epochs[backstep], 
											X, 
											control[backstep].data(),
											&available_thrust[backstep],
											&available_mass_flow_rate[backstep],
											&available_Isp[backstep], 
											&available_power[backstep],
											&active_power[backstep],
											&number_of_active_engines[backstep],
											ForceVector[backstep].data(),
											(options->derivative_type > 1 && needG ? 1 : 0),
											&dTdP[backstep],
											&dmdotdP[backstep],
											&dTdIsp[backstep],
											&dmdotdIsp[backstep],
											&dPdr[backstep],
											&dPdt[backstep],
											&dFSRPdr[backstep],
											dagravdRvec[backstep],
											dagravdtvec[step]);

			double mass_before_impulse = spacecraft_state[backstep][6] + local_throttle * options->engine_duty_cycle * available_mass_flow_rate[backstep] * (time_step_sizes[backstep]*86400);
			dVmax[backstep] = options->engine_duty_cycle * available_thrust[backstep] / mass_before_impulse * (time_step_sizes[backstep]*86400) / 1000;

			//step 6.3.6 apply the burn
			for (int k = 0; k < 3; ++k)
			{
				dV[backstep][k] = ForceVector[backstep][k] / mass_before_impulse * (time_step_sizes[backstep]*86400) / 1000;
				spacecraft_state[backstep][k+3] -= dV[backstep][k];
			}
			impulse_magnitude = local_throttle * dVmax[backstep];
			*current_deltaV += impulse_magnitude;
		
			//step 6.3.7 update the spacecraft mass
			spacecraft_state[backstep][6] = mass_before_impulse;
					

			//step 6.3.8 determine the propagation time and propagate
			double propagation_time;
			if (backstep == options->num_timesteps / 2)
			{
				//on the last step the propagation time is only half of the current timestep
				propagation_time = time_step_sizes[backstep] / 2.0;
				Kepler::KeplerLagrangeLaguerreConway(spacecraft_state[backstep].data(), spacecraft_state_backward, Universe->mu, -propagation_time*86400, Backward_STM[step+1], (options->derivative_type > 1 && needG ? true : false));
			}
			else
			{
				//on all other steps the propagation time is half of the current step plus half of the next step
				propagation_time = (time_step_sizes[backstep] + time_step_sizes[backstep - 1]) / 2.0;
				Kepler::KeplerLagrangeLaguerreConway(spacecraft_state[backstep].data(), spacecraft_state[backstep-1].data(), Universe->mu, -propagation_time*86400, Backward_STM[step+1], (options->derivative_type > 1 && needG ? true : false));
			}
			phase_time_elapsed_backward += propagation_time;

		}

		//Step 6.2.9 copy the final backward propagation mass to the match point
		spacecraft_state_backward[6] = spacecraft_state[options->num_timesteps / 2][6];

		//step Xindex back to the end of the arc
		if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
			(*Xindex) += 4 * options->num_timesteps/2;
		else
			(*Xindex) += 3 * options->num_timesteps/2;

		//step Findex back to the end of the arc
		(*Findex) += options->num_timesteps/2;

		//Step 6.4: enforce match point constraint
		for (size_t k=0; k<3; ++k)
		{
			//position
			F[*Findex+k] = (spacecraft_state_backward[k] - spacecraft_state_forward[k]) / Universe->LU;
		
			//velocity
			F[*Findex+k+3] = (spacecraft_state_backward[k+3] - spacecraft_state_forward[k+3]) / Universe->LU * Universe->TU;

			//match point state
			match_point_state[k] = spacecraft_state_forward[k];
			match_point_state[k+3] = spacecraft_state_forward[k+3];
		}
		//mass
		F[*Findex+6] = (spacecraft_state_backward[6] - spacecraft_state_forward[6])/(options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
		(*Findex) += 7;

		match_point_state[6] = spacecraft_state_forward[6];

		//Step 6.5: If required, compute the match point derivatives
		if (options->derivative_type > 1 && needG)
			calculate_match_point_derivatives(G, Gindex, j, p, options, Universe);

		//******************************************************************
		//Step 7: process the arrival, if applicable
		if (p == options->number_of_phases[j] - 1)
		{
			if (options->journey_arrival_type[j] == 3 || options->journey_arrival_type[j] == 5)
				dV_arrival_magnitude = 0.0;
		
			//note that "3" for journey_arrival_type indicates a "low-thrust rendezvous," which means we are there already and we don't need to do anything
			else
			{
				//compute the arrival deltaV
				if (boundary2_location_code > 0) //ending at body
					dV_arrival_magnitude = process_arrival(state_at_end_of_phase+3, boundary2_state, current_state+3, Body2->mu, Body2->r_SOI, F, Findex, j, options, Universe);
				else //ending at point on central body SOI, fixed point, or fixed orbit
					dV_arrival_magnitude = process_arrival(state_at_end_of_phase+3, boundary2_state, current_state+3, Universe->mu, Universe->r_SOI, F, Findex, j, options, Universe);
	
				//drop the electric propulsion stage
				state_at_end_of_phase[6] -= options->EP_dry_mass;

				//apply the arrival burn
				state_at_end_of_phase[6] *= exp(-dV_arrival_magnitude * 1000/ ((options->IspChem > 0 ? options->IspChem : 1e-6)* options->g0));
				*current_deltaV += dV_arrival_magnitude;

				//if this is a terminal intercept, we need to compute derivatives
				if (needG && options->journey_arrival_type[j] == 2)
				{
					double C3_desired = options->journey_final_velocity[j][1] * options->journey_final_velocity[j][1];
					//derivative with respect to x component of terminal velocity
					G[terminal_velocity_constraint_G_indices[0]] = 2.0 * terminal_velocity_constraint_X_scale_ranges[0] * X[terminal_velocity_constraint_X_indices[0]] / C3_desired;

					//derivative with respect to y component of terminal velocity
					G[terminal_velocity_constraint_G_indices[1]] = 2.0 * terminal_velocity_constraint_X_scale_ranges[1] * X[terminal_velocity_constraint_X_indices[1]] / C3_desired;

					//derivative with respect to z component of terminal velocity
					G[terminal_velocity_constraint_G_indices[2]] = 2.0 * terminal_velocity_constraint_X_scale_ranges[2] * X[terminal_velocity_constraint_X_indices[2]] / C3_desired;
				}
				if ( needG && options->journey_arrival_declination_constraint_flag[j] && (options->journey_arrival_type[j] == 0 || options->journey_arrival_type[j] == 2) ) //intercept with bounded v-infinity or orbit insertion
				{
					double Vx = X[arrival_declination_constraint_X_indices[0]];
					double Vy = X[arrival_declination_constraint_X_indices[1]];
					double Vz = X[arrival_declination_constraint_X_indices[2]];
					double A = sqrt(Vx*Vx + Vy*Vy);
					double B = Vx*Vx + Vy*Vy + Vz*Vz;

					//derivative with respect to x component of terminal velocity
					G[arrival_declination_constraint_G_indices[0]] = arrival_declination_constraint_X_scale_ranges[0] * Vx*Vz / (A*B) / options->journey_arrival_declination_bounds[j][1];

					//derivative with respect to y component of terminal velocity
					G[arrival_declination_constraint_G_indices[1]] = arrival_declination_constraint_X_scale_ranges[0] * Vy*Vz / (A*B) / options->journey_arrival_declination_bounds[j][1];

					//derivative with respect to z component of terminal velocity
					G[arrival_declination_constraint_G_indices[2]] = arrival_declination_constraint_X_scale_ranges[0] * -A/B / options->journey_arrival_declination_bounds[j][1];
				}
				else if (needG && options->journey_arrival_type[j] == 6)
				{
					double r = math::norm(boundary2_state, 3) / Universe->LU;
					double denominator = r * r * r * Universe->LU * Universe->LU;
					double LUTU2 = (Universe->TU / Universe->LU) * (Universe->TU / Universe->LU);
					double sqLU = sqrt(Universe->LU);

					//derivative with respect to x component of position
					G[escape_constraint_G_indices[0]] = escape_constraint_X_scale_ranges[0] * X[escape_constraint_X_indices[0]] / denominator;

					//derivative with respect to x component of position
					G[escape_constraint_G_indices[1]] = escape_constraint_X_scale_ranges[1] * X[escape_constraint_X_indices[1]] / denominator;

					//derivative with respect to x component of position
					G[escape_constraint_G_indices[2]] = escape_constraint_X_scale_ranges[2] * X[escape_constraint_X_indices[2]] / denominator;

					//derivative with respect to x component of terminal velocity
					G[escape_constraint_G_indices[3]] = escape_constraint_X_scale_ranges[3] * X[escape_constraint_X_indices[3]] * LUTU2;

					//derivative with respect to y component of terminal velocity
					G[escape_constraint_G_indices[4]] = escape_constraint_X_scale_ranges[4] * X[escape_constraint_X_indices[4]] * LUTU2;

					//derivative with respect to z component of terminal velocity
					G[escape_constraint_G_indices[5]] = escape_constraint_X_scale_ranges[5] * X[escape_constraint_X_indices[5]] * LUTU2;
				}
			}
			
		}
	
		//******************************************************************
		//Step 8: update the current epoch
		*current_epoch += this->TOF;

		//******************************************************************
		//Step 9: update the current state
		if (p == (options->number_of_phases[j] - 1) && options->journey_arrival_type[j] == 7)
		{//for terminal phases ending in a capture spiral
			for (int k = 0; k < 7; ++k)
				current_state[k] = this->spiral_capture_state_after_spiral[k];
			*current_epoch += this->spiral_capture_time;
		}
		else
		{
			for (int k = 0; k < 7; ++k)
				current_state[k] = this->state_at_end_of_phase[k];
		}

		return 0;
	}


	//bounds calculation function
	//return 0 if successful, 1 if failure
	int MGA_LT_phase::calcbounds(vector<double>* Xupperbounds, vector<double>* Xlowerbounds, vector<double>* Fupperbounds, vector<double>* Flowerbounds, vector<string>* Xdescriptions, vector<string>* Fdescriptions, vector<int>* iAfun, vector<int>* jAvar, vector<int>* iGfun, vector<int>* jGvar, vector<string>* Adescriptions, vector<string>* Gdescriptions, vector<double>* synodic_periods, int j, int p, EMTG::Astrodynamics::universe* Universe, missionoptions* options)
	{
		//this function calculates the upper and lower bounds for the decision and constraint vectors for MGA-LT
		//create a prefix string with journey and phase information
		stringstream prefixstream;
		prefixstream << "j" << j << "p" << p << ": ";
		string prefix = prefixstream.str();
		int first_X_entry_in_phase = Xupperbounds->size();

		//**************************************************************************
		//calculate bounds on variables and constraints governing the left boundary
		calcbounds_left_boundary(prefix, first_X_entry_in_phase, Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, p, Universe, options);

		//**************************************************************************
		//if EMTG is choosing an input power or Isp for the phase (for REP/NEP models), then this information must be encoded
		calcbounds_phase_thruster_parameters(prefix, first_X_entry_in_phase, Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, p, Universe, options);

		//**************************************************************************
		//next, we need to encode the phase flight time
		calcbounds_flight_time(prefix, first_X_entry_in_phase, Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, synodic_periods, j, p, Universe, options);

		//**************************************************************************
		//calculate bounds on variables and constraints governing the right boundary
		calcbounds_right_boundary(prefix, first_X_entry_in_phase, Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, p, Universe, options);

		//**************************************************************************
		//if we are using a variable scale/standard deviation distribution to pick the control points
		if (options->step_size_distribution == 3 || options->step_size_distribution == 4)
		{
			Xlowerbounds->push_back(1.0);
			Xupperbounds->push_back(options->step_size_stdv_or_scale);
			Xdescriptions->push_back(prefix + "step size distribution standard deviation or scale factor");
		}


		//**************************************************************************
		//next, we need to include the decision variables and constraints for each burn
		//now, for each timestep
		for (int w=0; w < options->num_timesteps; ++w)
		{
			stringstream stepstream;
			stepstream << w;
			//u_x
			Xlowerbounds->push_back(-1.0);
			Xupperbounds->push_back(1.0);
			Xdescriptions->push_back(prefix + "step " + stepstream.str() + " u_x");

			//u_y
			Xlowerbounds->push_back(-1.0);
			Xupperbounds->push_back(1.0);
			Xdescriptions->push_back(prefix + "step " + stepstream.str() + " u_y");

			//u_z
			Xlowerbounds->push_back(-1.0);
			Xupperbounds->push_back(1.0);
			Xdescriptions->push_back(prefix + "step " + stepstream.str() + " u_z");

			//for variable specific impulse propulsion systems, we must also encode the Isp for this time step
			if (options->engine_type == 4 || options->engine_type == 13)
			{
				Xlowerbounds->push_back(options->IspLT_minimum);
				Xupperbounds->push_back(options->IspLT);
				Xdescriptions->push_back(prefix + "step " + stepstream.str() + " Isp");
			}
			else if (options->engine_type == 12)
			{
				Xlowerbounds->push_back(3000.0);
				Xupperbounds->push_back(5000.0);
				Xdescriptions->push_back(prefix + "step " + stepstream.str() + " Isp");
			}

			//and the throttle magnitude constraint
			//throttle = 0
			Flowerbounds->push_back(0.0);
			Fupperbounds->push_back(1.0);
			Fdescriptions->push_back(prefix + "step " + stepstream.str() + " throttle magnitude constraint");

			//Jacobian entries for the throttle magnitude constraint
			//The throttle magnitude constraint is dependent only on the preceding three throttle components
			vector<int> step_G_indices;
			vector<double> dummyvalues(3);
			int vary_Isp_flag = (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13) ? 1 : 0;
			for (size_t entry = Xdescriptions->size() - 1 - vary_Isp_flag; entry > Xdescriptions->size() - 4 - vary_Isp_flag; --entry)
			{
				iGfun->push_back(Fdescriptions->size() - 1);
				jGvar->push_back(entry);
				stringstream EntryNameStream;
				EntryNameStream << "Derivative of " << prefix + "step " << w << " throttle magnitude constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << entry << "]: " << (*Xdescriptions)[entry];
				Gdescriptions->push_back(EntryNameStream.str());

				//store the position in the G vector
				step_G_indices.push_back(iGfun->size() - 1);
			}
			control_vector_G_indices.push_back(step_G_indices);
		}




		//**************************************************************************
		//finally, we encode the match point continuity constraints and their Jacobian entries,
		//noting that every patch point constraint in the phase has a derivative with respect to every variable in the phase
		//in addition, the patch point constraints have a derivative with respect to the previous phase's arrival mass
		//and the patch point constraints have a derivatives with respect to all previous time variables, including the launch date
		calcbounds_LT_match_points(prefix, first_X_entry_in_phase, Xupperbounds, Xlowerbounds, Fupperbounds, Flowerbounds, Xdescriptions, Fdescriptions, iAfun, jAvar, iGfun, jGvar, Adescriptions, Gdescriptions, j, p, Universe, options);

		//***************************************************************************
		//Construct a helper array "rectangular prism" of G indices for match point derivatives with respect to state variables within the phase
		vector < vector <int> > timeslice;
		vector <int> scanline;
		vector<string> constraint_type;
		constraint_type.push_back(" patch point x constraint");
		constraint_type.push_back(" patch point y constraint");
		constraint_type.push_back(" patch point z constraint");
		constraint_type.push_back(" patch point xdot constraint");
		constraint_type.push_back(" patch point ydot constraint");
		constraint_type.push_back(" patch point zdot constraint");
		constraint_type.push_back(" patch point m constraint");

		for (int step = 0; step < options->num_timesteps + 2; ++step)
		{
			timeslice.clear();
		
			//loop over constraint type dimension
			for (int c = 0; c < 7; ++c)
			{
				scanline.clear();

				string constraintname = prefix + constraint_type[c];

				for (size_t entry = 0; entry < Gdescriptions->size(); ++entry)
				{
					if ( (*Gdescriptions)[entry].find(constraintname) < 1024)
					{
						if (step == 0) //derivatives with respect to initial velocity and other parameters applied in the first step
						{
							//the first step of the first phase of a journey is abnormal because instead of encoding an XYZ vector, we have encoded a magnitude and two angles
							if (p == 0)
							{
								if ( (*Gdescriptions)[entry].find("magnitude of outgoing velocity asymptote") < 1024)
								{
									scanline.push_back(entry);     //derivative with respect to magnitude
									scanline.push_back(entry + 1); //derivative with respect to RA
									scanline.push_back(entry + 2); //derivative with respect to DEC
								}
							}
							//otherwise look for an XYZ initial velocity increment and a departure mass
							else
							{
								if ( (*Gdescriptions)[entry].find("initial velocity increment x") < 1024)
								{
									scanline.push_back(entry);     //derivative with respect to initial velocity increment x
									scanline.push_back(entry + 1); //derivative with respect to initial velocity increment y
									scanline.push_back(entry + 2); //derivative with respect to initial velocity increment z
								}
							}

							if ( (*Gdescriptions)[entry].find("initial mass multiplier (0-1)") < 1024)
							{
								G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[c] = entry;
							}
							if ( /*p == 0 &&*/ (*Gdescriptions)[entry].find("journey initial mass scale factor") < 1024)
							{
								G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[c] = entry;
							}
						}
						//derivatives with respect to final velocity - these do not exist for a terminal rendezvous phase
						else if (step == options->num_timesteps + 1 && (!(p == options->number_of_phases[j] - 1 && ((options->journey_arrival_type[j] == 1) || options->journey_arrival_type[j] == 3) || options->journey_arrival_type[j] == 5)))
						{
							if ( (*Gdescriptions)[entry].find("terminal velocity increment x") < 1024)
							{
								scanline.push_back(entry);	   //derivative with respect to terminal velocity increment x
								scanline.push_back(entry + 1); //derivative with respect to terminal velocity increment y
								scanline.push_back(entry + 2); //derivative with respect to terminal velocity increment z
							}
						}
						//derivatives with respect to the control parameters
						else
						{
							stringstream stepstream;
							stepstream << step - 1;

							string controlname = prefix + "step " + stepstream.str() + " u_x";

							if ( (*Gdescriptions)[entry].find(controlname) < 1024)
							{
								scanline.push_back(entry);	   //derivative with respect to u_x
								scanline.push_back(entry + 1); //derivative with respect to u_y
								scanline.push_back(entry + 2); //derivative with respect to u_z

								if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
									scanline.push_back(entry + 3); //derivative with respect to Isp
							}
						}
					} //end if ( (*Gdescriptions)[entry].find(constraintname) )
				} //end loop over Gdescriptions
			
				timeslice.push_back(scanline);
			
			} //end loop over constraint entries
			match_point_constraint_G_indices.push_back(timeslice);
	
		} //end loop over time steps

		//find derivative entries of match-point constraints with respect to time variables
		for (int c = 0; c < 7; ++c)
		{
			vector<int> constraint_slice;
			string constraintname = prefix + constraint_type[c];

			for (size_t entry = 0; entry < Gdescriptions->size(); ++entry)
			{
				if ( (*Gdescriptions)[entry].find(constraintname) < 1024)
				{
					if ( (*Gdescriptions)[entry].find("launch epoch") < 1024 || (*Gdescriptions)[entry].find("phase flight time") < 1024)
					{
						constraint_slice.push_back(entry);
					}
				}
			}

			G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables.push_back(constraint_slice);
		}

		//***************************************************************************
		//if this is the last phase, encode any constraints for the arrival processing
		if (p == options->number_of_phases[j] - 1)
		{
			if (options->journey_arrival_type[j] == 2) //intercept with bounded v-infinity
			{
				Flowerbounds->push_back((options->journey_final_velocity[j][0] / options->journey_final_velocity[j][1])*(options->journey_final_velocity[j][0] / options->journey_final_velocity[j][1]) - 1);
				Fupperbounds->push_back(0.0);
				Fdescriptions->push_back(prefix + "arrival C3 constraint");

				//Jacobian entry for a bounded v-infinity intercept
				//this is a nonlinear constraint dependent only on the terminal velocity vector for this phase
				for (size_t entry = first_X_entry_in_phase; entry < Xdescriptions->size(); ++entry)
				{
					if ((*Xdescriptions)[entry].find("terminal velocity") < 1024)
					{
						iGfun->push_back(Fdescriptions->size() - 1);
						jGvar->push_back(entry);
						stringstream EntryNameStream;
						EntryNameStream << "Derivative of " << prefix << " arrival v-infinity constraint constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << entry << "]: " << (*Xdescriptions)[entry];
						Gdescriptions->push_back(EntryNameStream.str());
						terminal_velocity_constraint_G_indices.push_back(iGfun->size() - 1);
						terminal_velocity_constraint_X_indices.push_back(entry);
						terminal_velocity_constraint_X_scale_ranges.push_back((*Xupperbounds)[entry] - (*Xlowerbounds)[entry]);
					}
				}
			}

			if ( options->journey_arrival_declination_constraint_flag[j] && (options->journey_arrival_type[j] == 0 || options->journey_arrival_type[j] == 2) ) //intercept with bounded v-infinity or orbit insertion
			{
				Flowerbounds->push_back(options->journey_arrival_declination_bounds[j][0] / options->journey_arrival_declination_bounds[j][1] - 1.0);
				Fupperbounds->push_back(0.0);
				Fdescriptions->push_back(prefix + "arrival declination constraint");

				//Jacobian entry for arrival declination constraint
				//this is a nonlinear constraint dependent only on the terminal velocity vector for this phase
				for (size_t entry = first_X_entry_in_phase; entry < Xdescriptions->size(); ++entry)
				{
					if ((*Xdescriptions)[entry].find("terminal velocity") < 1024)
					{
						iGfun->push_back(Fdescriptions->size() - 1);
						jGvar->push_back(entry);
						stringstream EntryNameStream;
						EntryNameStream << "Derivative of " << prefix << " arrival v-infinity constraint constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << entry << "]: " << (*Xdescriptions)[entry];
						Gdescriptions->push_back(EntryNameStream.str());
						arrival_declination_constraint_G_indices.push_back(iGfun->size() - 1);
						arrival_declination_constraint_X_indices.push_back(entry);
						arrival_declination_constraint_X_scale_ranges.push_back((*Xupperbounds)[entry] - (*Xlowerbounds)[entry]);
					}
				}
			}

			if (options->journey_arrival_type[j] == 6) //enforce escape constraint, E = 0
			{
				Flowerbounds->push_back(-math::SMALL);
				Fupperbounds->push_back(math::LARGE);
				Fdescriptions->push_back(prefix + "arrival escape condition (zero energy constraint)");

				//Jacobian entry for zero energy condition
				//this is a nonlinear constraint dependent on the terminal velocity vector
				//note that it is NOT dependent on position because these phases always end at the SOI and SOI distance is constant
				for (size_t entry = first_X_entry_in_phase; entry < Xdescriptions->size(); ++entry)
				{
					if ((*Xdescriptions)[entry].find("terminal velocity") < 1024 || (*Xdescriptions)[entry].find("right boundary point") < 1024)
					{
						iGfun->push_back(Fdescriptions->size() - 1);
						jGvar->push_back(entry);
						stringstream EntryNameStream;
						EntryNameStream << "Derivative of " << prefix << " escape constraint constraint F[" << Fdescriptions->size() - 1 << "] with respect to X[" << entry << "]: " << (*Xdescriptions)[entry];
						Gdescriptions->push_back(EntryNameStream.str());
						escape_constraint_G_indices.push_back(iGfun->size() - 1);
						escape_constraint_X_indices.push_back(entry);
						escape_constraint_X_scale_ranges.push_back((*Xupperbounds)[entry] - (*Xlowerbounds)[entry]);
					}
				}
			}

		
		}
		
		return 0;
	}

	//output function
	//return 0 if successful, 1 if failure
	int MGA_LT_phase::output(missionoptions* options, const double& launchdate, int j, int p, EMTG::Astrodynamics::universe* Universe, int* eventcount)
	{
		//Step 1: store data that will be used for the printing
		double empty_vector[] = {0,0,0};
		double phase_time_elapsed = 0.0;
		string event_type;
		if (p == 0)
		{
			if (j == 0 && boundary1_location_code > 0 && options->LV_type >= 0)
				event_type = "launch";
			else
				event_type = "departure";
		}
		else
		{
			event_type = "upwr_flyby";
			math::Matrix<double> periapse_state = calculate_flyby_periapse_state(V_infinity_in, V_infinity_out, flyby_altitude, *Body1);
			math::Matrix<double> periapse_R(3,1);
			periapse_R(0) = periapse_state(0);
			periapse_R(1) = periapse_state(1);
			periapse_R(2) = periapse_state(2);
			Bplane.define_bplane(V_infinity_in, BoundaryR, BoundaryV);
			Bplane.compute_BdotR_BdotT_from_periapse_position(Body1->mu, V_infinity_in, periapse_R, &BdotR, &BdotT);
			this->RA_departure = atan2(V_infinity_in(1), V_infinity_in(0));
			this->DEC_departure = asin(V_infinity_in(2) / V_infinity_in.norm());
		}

		string boundary1_name;
		string boundary2_name;

		switch (boundary1_location_code)
		{
			case -1:
				{
					boundary2_name = "free point";
					break;
				}
			case -2: //begin at SOI
				{
					boundary1_name = "Hyp-arrival";
					break;
				}
			default:
				boundary1_name = (Universe->bodies[boundary1_location_code - 1].name);
		}

		switch (boundary2_location_code)
		{
			case -1:
				{
					boundary2_name = "free point";
					break;
				}
			default:
				boundary2_name = (Universe->bodies[boundary2_location_code - 1].name);
		}
		double initial_Isp;
		if (j == 0 && p == 0)
		{
			if (options->journey_departure_type[j] == 1 || options->LV_type == -1)
			{
				initial_Isp =  options->IspDS;
			}
			else
			{
				initial_Isp =  -1;
			}
		}
		else
		{
			initial_Isp = options->IspChem;
		}

		//Step 2: all phases have at least 2+n events: departure, arrival, and n thrust/coast
		//*****************************************************************************
		//first let's print the departure/flyby
	
		//Step 2.01 if this phase starts with an escape spiral, we have to encode the spiral start date
		if (options->journey_departure_type[j] == 5 && p == 0)
		{
			event_type = "departure";

			this->write_summary_line(options,
									Universe,
									eventcount, 
									this->phase_start_epoch - this->spiral_escape_time,
									"begin_spiral",
									this->Body1->name,
									this->spiral_escape_time,
									0.0,
									0.0,
									0.0,
									0.0,
									0.0,
									0.0,
									this->spiral_escape_state_before_spiral,
									empty_vector,
									empty_vector,
									this->spiral_escape_dv,
									this->spiral_escape_thrust,
									this->spiral_escape_Isp,
									this->spiral_escape_power,
									this->spiral_escape_mdot,
									this->spiral_escape_number_of_engines,
									this->spiral_escape_active_power);
									
		}

		//then print the departure itself
		for (int k = 0; k < 3; ++k)
			dVdeparture[k] = V_infinity_out(k);

		write_summary_line(options,
							Universe,
							eventcount,
							this->phase_start_epoch,
							event_type,
							boundary1_name,
							0,
							(p > 0 ? flyby_altitude : Bradius),
							this->Btheta,
							(p > 0 ? this->flyby_turn_angle : -1),
							this->RA_departure,
							this->DEC_departure,
							this->C3_departure,
							this->state_at_beginning_of_phase,
							this->dVdeparture,
							empty_vector,
							(p == 0 ? (options->journey_departure_type[j] == 5 ? 0.0 : this->dV_departure_magnitude) : this->flyby_outgoing_v_infinity),
							-1,
							initial_Isp,
							-1,
							0,
							0,
							0);

		//*****************************************************************************
		//next, if there was an initial coast, we must print it
		//we'll have to start by propagating to the halfway point of the initial coast step
		if ( (j == 0 && p == 0 && options->forced_post_launch_coast > 1.0e-6) || ( (p > 0 || p == 0 && (options->journey_departure_type[j] == 3 || options->journey_departure_type[j] == 4) ) && options->forced_flyby_coast > 1.0e-6) )
		{
			double state_at_initial_coast_midpoint[7];
			double initial_coast_duration;
			double F, G, Ft, Gt;
			state_at_initial_coast_midpoint[6] = state_at_beginning_of_phase[6];

			if (j == 0 && p == 0 && options->forced_post_launch_coast > 0.0)
			{
				//initial coast after launch
				initial_coast_duration = options->forced_post_launch_coast;
			}
			else
			{
				//initial coast after flyby
				initial_coast_duration = options->forced_flyby_coast;
			}

			Kepler::KeplerLagrangeLaguerreConway(state_at_beginning_of_phase, state_at_initial_coast_midpoint, Universe->mu, initial_coast_duration * 86400 / 2.0, &F, &G, &Ft, &Gt);
			
			write_summary_line(	options,
								Universe,
								eventcount,
								phase_start_epoch + 0.5 * initial_coast_duration,
								"force-coast",
								"deep-space",
								initial_coast_duration,
								-1,
								-1,
								-1,
								0.0,
								0.0,
								0.0,
								state_at_initial_coast_midpoint,
								empty_vector,
								empty_vector,
								0.0,
								0.0,
								0.0,
								0.0,
								0.0,
								0,
								0.0);

			phase_time_elapsed += initial_coast_duration;
		}

		//*****************************************************************************
		//next, we must print each thrust arc


		for (int step = 0; step < options->num_timesteps; ++step)
		{
			double thrust_vector[3];
			double impulse_magnitude = sqrt(dV[step][0]*dV[step][0] + dV[step][1]*dV[step][1] + dV[step][2]*dV[step][2]);
			double throttle_magnitude = sqrt(control[step][0]*control[step][0] + control[step][1]*control[step][1] + control[step][2]*control[step][2]);
			double current_mass_flow_rate;

			double current_Isp, current_thrust, current_power;
			if (options->engine_type == 0) //fixed thrust/Isp
			{
				current_thrust = options->Thrust;
				current_Isp = options->IspLT;
				current_power = -1;
				current_mass_flow_rate = current_thrust / current_Isp / options->g0;
			}
			else //thrust, Isp are functions of power
			{
				current_thrust =  available_thrust[step];
				current_Isp = available_Isp[step];
				current_power = available_power[step];
				current_mass_flow_rate = current_thrust / current_Isp / options->g0;
			}

			if (EMTG::math::norm(control[step].data(), 3) > 1.0e-2 && fabs(current_thrust) > 1.0e-6)
			{
				event_type = "SFthrust";
			}
			else
			{
				event_type = "coast";
			}

			for (int k = 0; k < 3; ++k)
				thrust_vector[k] = control[step][k] * options->engine_duty_cycle * current_thrust;

			double state[7];
			state[6] = spacecraft_state[step][6];
			for (int k = 0; k < 6; ++k)
				state[k] = spacecraft_state[step][k];
			if (step >= options->num_timesteps / 2)
			{
				for (int k = 0; k < 3; ++k)
					state[k+3] += dV[step][k];
			}

			write_summary_line(options,
							Universe,
							eventcount,
							this->phase_start_epoch + phase_time_elapsed + 0.5 * this->time_step_sizes[step],
							event_type,
							"deep-space",
							this->time_step_sizes[step],
							-1,
							-1,
							-1,
							atan2(this->dV[step][1] + math::SMALL, this->dV[step][0]) * EMTG::math::PI / 180.0,
							asin(this->dV[step][2] / impulse_magnitude) * EMTG::math::PI / 180.0,
							0,
							state,
							//spacecraft_state[step].data(),
							this->dV[step].data(),
							thrust_vector,
							impulse_magnitude,
							current_thrust,
							current_Isp,
							current_power,
							math::norm(this->control[step].data(),3) * current_mass_flow_rate,
							this->number_of_active_engines[step],
							this->active_power[step]);

			phase_time_elapsed += this->time_step_sizes[step];

			//if we have stepped halfway through, insert the match point line
			if (step == options->num_timesteps / 2 - 1)
				write_summary_line(options,
							Universe,
							eventcount,
							this->phase_start_epoch + phase_time_elapsed,
							"match_point",
							"deep-space",
							0.0,
							-1,
							-1,
							-1,
							0,
							0,
							0,
							this->match_point_state.data(),
							empty_vector,
							empty_vector,
							0,
							0,
							0,
							0,
							0,
							0,
							0);
		}

		//*****************************************************************************
		//next, if there was an terminal coast, we must print it
		//we'll have to start by propagating to the halfway point of the initial coast step
		if (  (p < options->number_of_phases[j] - 1 ||  (options->journey_arrival_type[j] == 2 || options->journey_arrival_type[j] == 5) ) && options->forced_flyby_coast > 1.0e-6 )
		{
			double state_at_terminal_coast_midpoint[7];
			double terminal_coast_duration = options->forced_flyby_coast;
			double F, G, Ft, Gt;
			state_at_terminal_coast_midpoint[6] = state_at_end_of_phase[6];

			Kepler::KeplerLagrangeLaguerreConway(state_at_end_of_phase, state_at_terminal_coast_midpoint, Universe->mu, -terminal_coast_duration * 86400 / 2.0, &F, &G, &Ft, &Gt);
			
			write_summary_line(	options,
								Universe,
								eventcount,
								this->phase_start_epoch + phase_time_elapsed + 0.5 * terminal_coast_duration,
								"force-coast",
								"deep-space",
								terminal_coast_duration,
								-1,
								-1,
								-1,
								0.0,
								0.0,
								0.0,
								state_at_terminal_coast_midpoint,
								empty_vector,
								empty_vector,
								0.0,
								0.0,
								0.0,
								0.0,
								0.0,
								0,
								0.0);

			phase_time_elapsed += terminal_coast_duration;
		}

		//*****************************************************************************
		//finally, terminal phases have an arrival maneuver

		if (p == options->number_of_phases[j] - 1)
		{
			if (options->journey_arrival_type[j] == 0)
				event_type = "insertion";
			else if (options->journey_arrival_type[j] == 1)
				event_type = "rendezvous";
			else if (options->journey_arrival_type[j] == 2)
				event_type = "intercept";
			else if (options->journey_arrival_type[j] == 3 || options->journey_arrival_type[j] == 7)
				event_type = "LT_rndzvs";
			else if (options->journey_arrival_type[j] == 5 || options->journey_arrival_type[j] == 4)
				event_type = "match-vinf";
	
			double dV_arrival_mag;
			if (options->journey_arrival_type[j] == 2)
			{
				dV_arrival_mag = sqrt(this->C3_arrival);
			}
			else if (options->journey_arrival_type[j] == 4 || options->journey_arrival_type[j] == 3)
			{
				dV_arrival_mag = 0;
				this->dVarrival[0] = 0;
				this->dVarrival[1] = 0;
				this->dVarrival[2] = 0;
			}
			else
			{
				dV_arrival_mag = this->dV_arrival_magnitude;
			}

			write_summary_line(options,
							Universe,
							eventcount,
							this->phase_start_epoch + this->TOF,
							event_type,
							boundary2_name,
							0,
							-1,
							-1,
							-1,
							this->RA_arrival,
							this->DEC_arrival,
							this->C3_arrival,
							this->state_at_end_of_phase,
							this->dVarrival,
							empty_vector,
							dV_arrival_mag,
							-1,
							options->IspChem,
							-1,
							0,
							0,
							0);

			//if the phase ends in a capture spiral, print it
			if (options->journey_arrival_type[j] == 7)
			{
				this->write_summary_line(options,
									Universe,
									eventcount, 
									this->phase_start_epoch + this->TOF + this->spiral_capture_time,
									"end_spiral",
									this->Body2->name,
									this->spiral_capture_time,
									0.0,
									0.0,
									0.0,
									0.0,
									0.0,
									0.0,
									this->spiral_capture_state_after_spiral,
									empty_vector,
									empty_vector,
									this->spiral_capture_dv,
									this->spiral_capture_thrust,
									this->spiral_capture_Isp,
									this->spiral_capture_power,
									this->spiral_capture_mdot,
									this->spiral_capture_number_of_engines,
									this->spiral_capture_active_power);
			}
		}

		return 0;
	}

	//function to calculate the patch point derivatives
	int MGA_LT_phase::calculate_match_point_derivatives(	double* G,
															int* Gindex,
															const int& j, 
															const int& p,
															missionoptions* options, 
															EMTG::Astrodynamics::universe* Universe)
	{
		double dxdu, dydu, dzdu, dxdotdu, dydotdu, dzdotdu, dmdu, deltat, dtdu;

		//compute and store the forward derivatives of the match point constraints with respect to the control unit vector
		//and (if applicable) variable Isp
		//using the method developed by McConaghy in his dissertation, p38 and onward
		//modified by Ellison and Englander 8/28/2013 for unit-vector control
		for (int step = 1; step <= options->num_timesteps / 2; ++step)
		{
			for (int c = 0; c < 3; ++c)
			{
				//first we need to get the derivative of the next step's state vector with respect to the step where the control is applied
				dxdu = Forward_STM[step](0,c+3) * dVmax[step-1];
				dydu = Forward_STM[step](1,c+3) * dVmax[step-1];
				dzdu = Forward_STM[step](2,c+3) * dVmax[step-1];
				dxdotdu = Forward_STM[step](3,c+3) * dVmax[step-1];
				dydotdu = Forward_STM[step](4,c+3) * dVmax[step-1];
				dzdotdu = Forward_STM[step](5,c+3) * dVmax[step-1];

				double umag = math::norm(control[step-1].data(), 3);
				double r = math::norm(spacecraft_state[step-1].data(), 3);

				deltat = (time_step_sizes[step]*86400);

				dmdu = -(available_mass_flow_rate[step-1] * deltat * options->engine_duty_cycle) * ( (control[step-1][c] / (umag + 1.0e-10) ) );
				dtdu = 0.0; //there is no dependence of time on thrust control

				//loop over later steps
				for (int stepnext = step + 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			step,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[step][0][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][0][c]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[step][1][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][1][c]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[step][2][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][2][c]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[step][3][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][3][c]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[step][4][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][4][c]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[step][5][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][5][c]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[step][6][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][6][c]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);

			}//end loop over controls
		}

		//derivative with respect to Isp for forward propagation with VSI thruster
		if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
		{
			for (int step = 1; step <= options->num_timesteps / 2; ++step)
			{
				deltat = (time_step_sizes[step]*86400);

				double ux = control[step-1][0];
				double uy = control[step-1][1];
				double uz = control[step-1][2];
				double m = spacecraft_state[step-1][6];
				dxdu = options->engine_duty_cycle * deltat * dTdIsp[step-1]/1000/m * (Forward_STM[step](0,3)*ux + Forward_STM[step](0,4)*uy + Forward_STM[step](0,5)*uz);
				dydu = options->engine_duty_cycle * deltat * dTdIsp[step-1]/1000/m * (Forward_STM[step](1,3)*ux + Forward_STM[step](1,4)*uy + Forward_STM[step](1,5)*uz);
				dzdu = options->engine_duty_cycle * deltat * dTdIsp[step-1]/1000/m * (Forward_STM[step](2,3)*ux + Forward_STM[step](2,4)*uy + Forward_STM[step](2,5)*uz);
				dxdotdu = options->engine_duty_cycle * deltat * dTdIsp[step-1]/1000/m * (Forward_STM[step](3,3)*ux + Forward_STM[step](3,4)*uy + Forward_STM[step](3,5)*uz);
				dydotdu = options->engine_duty_cycle * deltat * dTdIsp[step-1]/1000/m * (Forward_STM[step](4,3)*ux + Forward_STM[step](4,4)*uy + Forward_STM[step](4,5)*uz);
				dzdotdu = options->engine_duty_cycle * deltat * dTdIsp[step-1]/1000/m * (Forward_STM[step](5,3)*ux + Forward_STM[step](5,4)*uy + Forward_STM[step](5,5)*uz);

				double umag = math::norm(control[step-1].data(), 3);

				dmdu = -deltat * options->engine_duty_cycle * (umag + 1.0e-10) * dmdotdIsp[step-1];
				dtdu = 0.0; //there is no dependence of time on Isp

				//loop over later steps
				for (int stepnext = step + 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			step,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[step][0][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][0][3]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[step][1][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][1][3]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[step][2][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][2][3]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[step][3][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][3][3]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[step][4][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][4][3]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[step][5][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][5][3]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[step][6][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[step][6][3]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
		}

		if (p == 0)
		{
			if (j == 0 && options->allow_initial_mass_to_vary && !(options->journey_departure_type[j] == 5) )
			{
				dxdu = 0.0;
				dydu = 0.0;
				dzdu = 0.0;
				dxdotdu = 0.0;
				dydotdu = 0.0;
				dzdotdu = 0.0;

				dmdu = unscaled_phase_initial_mass;
				dtdu = 0.0; //there is no dependence of time on mass

				//loop over later steps
				for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			0,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[0]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[0]]] * dxdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[1]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[1]]] * dydu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[2]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[2]]] * dzdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[3]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[3]]] * dxdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[4]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[4]]] * dydotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[5]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[5]]] * dzdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[6]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_mission_initial_mass_multiplier[6]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
			if (options->journey_variable_mass_increment[j] && !(options->journey_departure_type[j] == 5))
			{
				dxdu = 0.0;
				dydu = 0.0;
				dzdu = 0.0;
				dxdotdu = 0.0;
				dydotdu = 0.0;
				dzdotdu = 0.0;

				dmdu = options->journey_starting_mass_increment[j];
				dtdu = 0.0; //there is no dependence of time on mass

				//loop over later steps
				for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			0,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[0]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[0]]] * dxdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[1]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[1]]] * dydu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[2]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[2]]] * dzdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[3]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[3]]] * dxdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[4]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[4]]] * dydotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[5]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[5]]] * dzdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[6]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_journey_initial_mass_increment_multiplier[6]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
			if (options->journey_departure_type[j] == 0)// && options->LV_type > 0)
			{
				//first phase first step stuff
				double cRA = cos(RA_departure);
				double sRA = sin(RA_departure);
				double cDEC = cos(DEC_departure);
				double sDEC = sin(DEC_departure);
				double v_infinity = sqrt(C3_departure);
				dtdu = 0.0; //there is no dependence of time on the departure V_infinity vector
			
				//Step 1: derivative of match point constraints with respect to v_infinity magnitude
				dxdu = Forward_STM[0](0,3) * (cRA*cDEC) + Forward_STM[0](0,4) * (sRA*cDEC) + Forward_STM[0](0,5) * sDEC;
				dydu = Forward_STM[0](1,3) * (cRA*cDEC) + Forward_STM[0](1,4) * (sRA*cDEC) + Forward_STM[0](1,5) * sDEC;
				dzdu = Forward_STM[0](2,3) * (cRA*cDEC) + Forward_STM[0](2,4) * (sRA*cDEC) + Forward_STM[0](2,5) * sDEC;
				dxdotdu = Forward_STM[0](3,3) * (cRA*cDEC) + Forward_STM[0](3,4) * (sRA*cDEC) + Forward_STM[0](3,5) * sDEC;
				dydotdu = Forward_STM[0](4,3) * (cRA*cDEC) + Forward_STM[0](4,4) * (sRA*cDEC) + Forward_STM[0](4,5) * sDEC;
				dzdotdu = Forward_STM[0](5,3) * (cRA*cDEC) + Forward_STM[0](5,4) * (sRA*cDEC) + Forward_STM[0](5,5) * sDEC;

				dmdu = this->dmdvinf;

				//loop over later steps
				for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			0,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[0][0][0]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][0][0]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[0][1][0]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][1][0]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[0][2][0]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][2][0]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[0][3][0]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][3][0]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][4][0]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][4][0]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][5][0]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][5][0]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][6][0]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][6][0]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);

				//Step 2: derivative of match point constraints with respect to right ascension of launch asymptote
				dxdu = v_infinity * ( Forward_STM[0](0,3) * (-sRA*cDEC) + Forward_STM[0](0,4) * (cRA*cDEC) );
				dydu = v_infinity * ( Forward_STM[0](1,3) * (-sRA*cDEC) + Forward_STM[0](1,4) * (cRA*cDEC) );
				dzdu = v_infinity * ( Forward_STM[0](2,3) * (-sRA*cDEC) + Forward_STM[0](2,4) * (cRA*cDEC) );
				dxdotdu = v_infinity * ( Forward_STM[0](3,3) * (-sRA*cDEC) + Forward_STM[0](3,4) * (cRA*cDEC) );
				dydotdu = v_infinity * ( Forward_STM[0](4,3) * (-sRA*cDEC) + Forward_STM[0](4,4) * (cRA*cDEC) );
				dzdotdu = v_infinity * ( Forward_STM[0](5,3) * (-sRA*cDEC) + Forward_STM[0](5,4) * (cRA*cDEC) );

				dmdu = 0.0;

				//loop over later steps
				for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			0,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[0][0][1]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][0][1]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[0][1][1]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][1][1]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[0][2][1]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][2][1]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[0][3][1]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][3][1]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][4][1]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][4][1]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][5][1]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][5][1]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][6][1]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][6][1]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);

				//Step 3: derivative of match point constraints with respect to declination of launch asymptote
				dxdu = v_infinity * ( Forward_STM[0](0,3) * (-cRA*sDEC) + Forward_STM[0](0,4) * (-sRA*sDEC) + Forward_STM[0](0,5) * (cDEC) );
				dydu = v_infinity * ( Forward_STM[0](1,3) * (-cRA*sDEC) + Forward_STM[0](1,4) * (-sRA*sDEC) + Forward_STM[0](1,5) * (cDEC) );
				dzdu = v_infinity * ( Forward_STM[0](2,3) * (-cRA*sDEC) + Forward_STM[0](2,4) * (-sRA*sDEC) + Forward_STM[0](2,5) * (cDEC) );
				dxdotdu = v_infinity * ( Forward_STM[0](3,3) * (-cRA*sDEC) + Forward_STM[0](3,4) * (-sRA*sDEC) + Forward_STM[0](3,5) * (cDEC) );
				dydotdu = v_infinity * ( Forward_STM[0](4,3) * (-cRA*sDEC) + Forward_STM[0](4,4) * (-sRA*sDEC) + Forward_STM[0](4,5) * (cDEC) );
				dzdotdu = v_infinity * ( Forward_STM[0](5,3) * (-cRA*sDEC) + Forward_STM[0](5,4) * (-sRA*sDEC) + Forward_STM[0](5,5) * (cDEC) );

				dmdu = 0.0;

				//loop over later steps
				for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			0,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[0][0][2]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][0][2]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[0][1][2]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][1][2]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[0][2][2]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][2][2]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[0][3][2]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][3][2]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][4][2]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][4][2]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][5][2]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][5][2]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][6][2]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][6][2]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
		}
		else//for other phases other than the first
		{
			//derivative of match point constraints with respect to initial velocity increment
			for (int c = 0; c < 3; ++c)
			{
				//first we need to get the derivative of the next step's state vector with respect to the terminal velocity increment
				dxdu = Forward_STM[0](0,c+3);
				dydu = Forward_STM[0](1,c+3);
				dzdu = Forward_STM[0](2,c+3);
				dxdotdu = Forward_STM[0](3,c+3);
				dydotdu = Forward_STM[0](4,c+3);
				dzdotdu = Forward_STM[0](5,c+3);

				dmdu = 0.0;
				dtdu = 0.0; //there is no dependence of time on initial velocity

				//loop over later steps
				for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			0,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[0][0][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][0][c]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[0][1][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][1][c]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[0][2][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][2][c]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[0][3][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][3][c]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][4][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][4][c]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][5][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][5][c]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[0][6][c]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[0][6][c]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}//end loop over controls

			//derivative of match point constraints with respect to initial mass
			{
				dxdu = 0.0;
				dydu = 0.0;
				dzdu = 0.0;
				dxdotdu = 0.0;
				dydotdu = 0.0;
				dzdotdu = 0.0;

				dmdu = -1.0;
				dtdu = 0.0; //there is no dependence of time on initial mass

				//loop over later steps
				for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_forward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			0,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[0]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[0]]] * dxdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[1]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[1]]] * dydu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[2]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[2]]] * dzdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[3]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[3]]] * dxdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[4]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[4]]] * dydotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[5]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[5]]] * dzdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[6]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_initial_mass[6]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
		}
		
		//derivatives of the constraints with respect to the forward flight time variables
		//this in turn is dependent on the velocity and acceleration of the left-most body
		if (options->derivative_type > 2)
		{
			double dxdt = left_boundary_state_derivative[0];
			double dydt = left_boundary_state_derivative[1];
			double dzdt = left_boundary_state_derivative[2];
			double dxdotdt = left_boundary_state_derivative[3];
			double dydotdt = left_boundary_state_derivative[4];
			double dzdotdt = left_boundary_state_derivative[5];

			double next_step_distance_from_central_body = math::norm(this->spacecraft_state[0].data(), 3);
			double next_step_acceleration_coefficient = -Universe->mu / (next_step_distance_from_central_body*next_step_distance_from_central_body*next_step_distance_from_central_body);

			dtdu = 1.0; //time varies directly with time variables (obviously)

			dxdu = Forward_STM[0](0,0) * dxdt + Forward_STM[0](0,1) * dydt + Forward_STM[0](0,2) * dzdt
				+ Forward_STM[0](0,3) * dxdotdt + Forward_STM[0](0,4) * dydotdt + Forward_STM[0](0,5) * dzdotdt
				+ this->spacecraft_state[0][3] * dtdu;
			dydu = Forward_STM[0](1,0) * dxdt + Forward_STM[0](1,1) * dydt + Forward_STM[0](1,2) * dzdt
				+ Forward_STM[0](1,3) * dxdotdt + Forward_STM[0](1,4) * dydotdt + Forward_STM[0](1,5) * dzdotdt
				+ this->spacecraft_state[0][4] * dtdu;
			dzdu = Forward_STM[0](2,0) * dxdt + Forward_STM[0](2,1) * dydt + Forward_STM[0](2,2) * dzdt
				+ Forward_STM[0](2,3) * dxdotdt + Forward_STM[0](2,4) * dydotdt + Forward_STM[0](2,5) * dzdotdt
				+ this->spacecraft_state[0][5] * dtdu;
			dxdotdu = Forward_STM[0](3,0) * dxdt + Forward_STM[0](3,1) * dydt + Forward_STM[0](3,2) * dzdt
				+ Forward_STM[0](3,3) * dxdotdt + Forward_STM[0](3,4) * dydotdt + Forward_STM[0](3,5) * dzdotdt
				+ next_step_acceleration_coefficient * this->spacecraft_state[0][0] * dtdu;
			dydotdu = Forward_STM[0](4,0) * dxdt + Forward_STM[0](4,1) * dydt + Forward_STM[0](4,2) * dzdt
				+ Forward_STM[0](4,3) * dxdotdt + Forward_STM[0](4,4) * dydotdt + Forward_STM[0](4,5) * dzdotdt
				+ next_step_acceleration_coefficient * this->spacecraft_state[0][1] * dtdu;
			dzdotdu = Forward_STM[0](5,0) * dxdt + Forward_STM[0](5,1) * dydt + Forward_STM[0](5,2) * dzdt
				+ Forward_STM[0](5,3) * dxdotdt + Forward_STM[0](5,4) * dydotdt + Forward_STM[0](5,5) * dzdotdt
				+ next_step_acceleration_coefficient * this->spacecraft_state[0][2] * dtdu;

			dmdu = 0.0;

			//loop over later steps
			for (int stepnext = 1; stepnext <= options->num_timesteps / 2; ++stepnext)
			{
				calculate_match_point_forward_propagation_derivatives(	G,
																		Gindex,
																		j, 
																		p,
																		options, 
																		Universe,
																		0,
																		stepnext,
																		dxdu,
																		dydu,
																		dzdu,
																		dxdotdu,
																		dydotdu,
																		dzdotdu,
																		dmdu,
																		dtdu);
			} //end loop over later steps

			for (int timevar = 0; timevar < G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[0].size(); ++timevar)
			{
				//place the derivatives in the Jacobian
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[0][timevar]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[0][timevar]]] * dxdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[1][timevar]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[1][timevar]]] * dydu / Universe->LU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[2][timevar]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[2][timevar]]] * dzdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[3][timevar]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[3][timevar]]] * dxdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[4][timevar]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[4][timevar]]] * dydotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[5][timevar]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[5][timevar]]] * dzdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[6][timevar]] = -options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[6][timevar]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
		}
	
		//compute and store the backward derivatives of the match point constraints with respect to the control unit vector
		//and (if applicable) variable Isp
		for (int step = 0; step < options->num_timesteps / 2; ++step)
		{
			//translate into backward steps
			int backstep = options->num_timesteps - 1 - step;

			for (int c = 0; c < 3; ++c)
			{
				//first we need to get the derivative of the next step's state vector with respect to the step where the control is applied
				dxdu = -Backward_STM[step+1](0,c+3) * dVmax[backstep];
				dydu = -Backward_STM[step+1](1,c+3) * dVmax[backstep];
				dzdu = -Backward_STM[step+1](2,c+3) * dVmax[backstep];
				dxdotdu = -Backward_STM[step+1](3,c+3) * dVmax[backstep];
				dydotdu = -Backward_STM[step+1](4,c+3) * dVmax[backstep];
				dzdotdu = -Backward_STM[step+1](5,c+3) * dVmax[backstep];

				double umag = math::norm(control[backstep].data(), 3);
				double r = math::norm(spacecraft_state[backstep].data(), 3);

				deltat = (time_step_sizes[backstep]*86400);

				dmdu = (available_mass_flow_rate[backstep] * deltat * options->engine_duty_cycle) * ( (control[backstep][c] / (umag + 1.0e-10) ) );
				dtdu = 0.0; //there is no dependence of time on thrust control

				//loop over later steps
				for (int stepnext = step + 1; stepnext < options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_backward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			backstep,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[backstep+1][0][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][0][c]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[backstep+1][1][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][1][c]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[backstep+1][2][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][2][c]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[backstep+1][3][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][3][c]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[backstep+1][4][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][4][c]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[backstep+1][5][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][5][c]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[backstep+1][6][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][6][c]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}//end loop over controls
		}

		//derivative with respect to Isp for backward propagation with VSI thruster
		if (options->engine_type == 4 || options->engine_type == 12 || options->engine_type == 13)
		{
			for (int step = 0; step < options->num_timesteps / 2; ++step)
			{
				//translate into backward steps
				int backstep = options->num_timesteps - 1 - step;

				deltat = (time_step_sizes[backstep]*86400);

				double ux = control[backstep][0];
				double uy = control[backstep][1];
				double uz = control[backstep][2];
				double m = spacecraft_state[backstep][6];
				dxdu = options->engine_duty_cycle * deltat * dTdIsp[backstep]/1000/m * (Backward_STM[step+1](0,3)*ux + Backward_STM[step+1](0,4)*uy + Backward_STM[step+1](0,5)*uz);
				dydu = options->engine_duty_cycle * deltat * dTdIsp[backstep]/1000/m * (Backward_STM[step+1](1,3)*ux + Backward_STM[step+1](1,4)*uy + Backward_STM[step+1](1,5)*uz);
				dzdu = options->engine_duty_cycle * deltat * dTdIsp[backstep]/1000/m * (Backward_STM[step+1](2,3)*ux + Backward_STM[step+1](2,4)*uy + Backward_STM[step+1](2,5)*uz);
				dxdotdu = options->engine_duty_cycle * deltat * dTdIsp[backstep]/1000/m * (Backward_STM[step+1](3,3)*ux + Backward_STM[step+1](3,4)*uy + Backward_STM[step+1](3,5)*uz);
				dydotdu = options->engine_duty_cycle * deltat * dTdIsp[backstep]/1000/m * (Backward_STM[step+1](4,3)*ux + Backward_STM[step+1](4,4)*uy + Backward_STM[step+1](4,5)*uz);
				dzdotdu = options->engine_duty_cycle * deltat * dTdIsp[backstep]/1000/m * (Backward_STM[step+1](5,3)*ux + Backward_STM[step+1](5,4)*uy + Backward_STM[step+1](5,5)*uz);

				double umag = math::norm(control[backstep].data(), 3);

				dmdu = -deltat * options->engine_duty_cycle * (umag + 1.0e-10) * dmdotdIsp[backstep];
				dtdu = 0.0; //there is no dependence of time on Isp

				//loop over later steps
				for (int stepnext = step + 1; stepnext < options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_backward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			step,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[backstep+1][0][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][0][3]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[backstep+1][1][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][1][3]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[backstep+1][2][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][2][3]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[backstep+1][3][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][3][3]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[backstep+1][4][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][4][3]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[backstep+1][5][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][5][3]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[backstep+1][6][3]] = -options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[backstep+1][6][3]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
		}

		//derivative of match point constraints with respect to arrival mass
		//disable this for a phase ending in a capture spiral
		if (!(p == options->number_of_phases[j] && options->journey_arrival_type[j] == 7))
		{
			dxdu = 0.0;
			dydu = 0.0;
			dzdu = 0.0;
			dxdotdu = 0.0;
			dydotdu = 0.0;
			dzdotdu = 0.0;

			dmdu = 1.0;
			dtdu = 0.0; //there is no dependence of time on arrival mass

			//loop over later steps
			for (int stepnext = 0; stepnext < options->num_timesteps / 2; ++stepnext)
			{
				calculate_match_point_backward_propagation_derivatives(	G,
																		Gindex,
																		j, 
																		p,
																		options, 
																		Universe,
																		options->num_timesteps - 1,
																		stepnext,
																		dxdu,
																		dydu,
																		dzdu,
																		dxdotdu,
																		dydotdu,
																		dzdotdu,
																		dmdu,
																		dtdu);
			} //end loop over later steps

			//place the derivatives in the Jacobian
			G[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[0]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[0]]] * dxdu / Universe->LU;
			G[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[1]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[1]]] * dydu / Universe->LU;
			G[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[2]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[2]]] * dzdu / Universe->LU;
			G[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[3]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[3]]] * dxdotdu / Universe->LU * Universe->TU;
			G[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[4]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[4]]] * dydotdu / Universe->LU * Universe->TU;
			G[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[5]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[5]]] * dzdotdu / Universe->LU * Universe->TU;
			G[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[6]] = options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_constraints_with_respect_to_arrival_mass[6]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
		}

		//derivatives of the constraints with respect to the backward flight time variables
		//this in turn is dependent on the velocity and acceleration of the right-most body
		if (options->derivative_type > 2)
		{
			double dxdt = right_boundary_state_derivative[0];
			double dydt = right_boundary_state_derivative[1];
			double dzdt = right_boundary_state_derivative[2];
			double dxdotdt = right_boundary_state_derivative[3];
			double dydotdt = right_boundary_state_derivative[4];
			double dzdotdt = right_boundary_state_derivative[5];

			dtdu = -1.0; //time varies directly with time variables (obviously)

			double next_backstep_distance_from_central_body = math::norm(this->spacecraft_state[options->num_timesteps-1].data(), 3);
			double next_backstep_acceleration_coefficient = -Universe->mu / (next_backstep_distance_from_central_body*next_backstep_distance_from_central_body*next_backstep_distance_from_central_body);

			dxdu = Backward_STM[0](0,0) * dxdt + Backward_STM[0](0,1) * dydt + Backward_STM[0](0,2) * dzdt 
				+ Backward_STM[0](0,3) * dxdotdt + Backward_STM[0](0,4) * dydotdt + Backward_STM[0](0,5) * dzdotdt
				+ this->spacecraft_state[options->num_timesteps-1][3] * dtdu;
			dydu = Backward_STM[0](1,0) * dxdt + Backward_STM[0](1,1) * dydt + Backward_STM[0](1,2) * dzdt 
				+ Backward_STM[0](1,3) * dxdotdt + Backward_STM[0](1,4) * dydotdt + Backward_STM[0](1,5) * dzdotdt
				+ this->spacecraft_state[options->num_timesteps-1][4] * dtdu;
			dzdu = Backward_STM[0](2,0) * dxdt + Backward_STM[0](2,1) * dydt + Backward_STM[0](2,2) * dzdt 
				+ Backward_STM[0](2,3) * dxdotdt + Backward_STM[0](2,4) * dydotdt + Backward_STM[0](2,5) * dzdotdt
				+ this->spacecraft_state[options->num_timesteps-1][5] * dtdu;
			dxdotdu = Backward_STM[0](3,0) * dxdt + Backward_STM[0](3,1) * dydt + Backward_STM[0](3,2) * dzdt 
				+ Backward_STM[0](3,3) * dxdotdt + Backward_STM[0](3,4) * dydotdt + Backward_STM[0](3,5) * dzdotdt;
				+ next_backstep_acceleration_coefficient * this->spacecraft_state[options->num_timesteps-1][0] * dtdu;
			dydotdu = Backward_STM[0](4,0) * dxdt + Backward_STM[0](4,1) * dydt + Backward_STM[0](4,2) * dzdt 
				+ Backward_STM[0](4,3) * dxdotdt + Backward_STM[0](4,4) * dydotdt + Backward_STM[0](4,5) * dzdotdt
				+ next_backstep_acceleration_coefficient * this->spacecraft_state[options->num_timesteps-1][1] * dtdu;
			dzdotdu = Backward_STM[0](5,0) * dxdt + Backward_STM[0](5,1) * dydt + Backward_STM[0](5,2) * dzdt 
				+ Backward_STM[0](5,3) * dxdotdt + Backward_STM[0](5,4) * dydotdt + Backward_STM[0](5,5) * dzdotdt
				+ next_backstep_acceleration_coefficient * this->spacecraft_state[options->num_timesteps-1][2] * dtdu;

			dmdu = 0.0;
			

			//loop over later steps
			for (int stepnext = 0; stepnext < options->num_timesteps / 2; ++stepnext)
			{
				calculate_match_point_backward_propagation_derivatives(	G,
																		Gindex,
																		j, 
																		p,
																		options, 
																		Universe,
																		0,
																		stepnext,
																		dxdu,
																		dydu,
																		dzdu,
																		dxdotdu,
																		dydotdu,
																		dzdotdu,
																		dmdu,
																		dtdu);
			} //end loop over later steps

			for (int timevar = 0; timevar < G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[0].size(); ++timevar)
			{
				//place the derivatives in the Jacobian
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[0][timevar]] += options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[0][timevar]]] * dxdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[1][timevar]] += options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[1][timevar]]] * dydu / Universe->LU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[2][timevar]] += options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[2][timevar]]] * dzdu / Universe->LU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[3][timevar]] += options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[3][timevar]]] * dxdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[4][timevar]] += options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[4][timevar]]] * dydotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[5][timevar]] += options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[5][timevar]]] * dzdotdu / Universe->LU * Universe->TU;
				G[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[6][timevar]] += options->X_scale_ranges[options->jGvar[G_index_of_derivative_of_match_point_with_respect_to_flight_time_variables[6][timevar]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}
		}

		
		//derivative with respect to arrival velocity
		//only evaluated for phases that are not terminal intercepts
		if (!(p == options->number_of_phases[j] - 1 && ((options->journey_arrival_type[j] == 1) || options->journey_arrival_type[j] == 3) || options->journey_arrival_type[j] == 5 || options->journey_arrival_type[j] == 7))
		{
			for (int c = 0; c < 3; ++c)
			{
				//first we need to get the derivative of the next step's state vector with respect to the terminal velocity increment
				dxdu = Backward_STM[0](0,c+3);
				dydu = Backward_STM[0](1,c+3);
				dzdu = Backward_STM[0](2,c+3);
				dxdotdu = Backward_STM[0](3,c+3);
				dydotdu = Backward_STM[0](4,c+3);
				dzdotdu = Backward_STM[0](5,c+3);

				dmdu = 0.0;
				dtdu = 0.0; //there is no dependence of time on initial velocity

				//loop over later steps
				for (int stepnext = 0; stepnext < options->num_timesteps / 2; ++stepnext)
				{
					calculate_match_point_backward_propagation_derivatives(	G,
																			Gindex,
																			j, 
																			p,
																			options, 
																			Universe,
																			options->num_timesteps - 1,
																			stepnext,
																			dxdu,
																			dydu,
																			dzdu,
																			dxdotdu,
																			dydotdu,
																			dzdotdu,
																			dmdu,
																			dtdu);
				} //end loop over later steps

				//place the derivatives in the Jacobian
				G[match_point_constraint_G_indices[options->num_timesteps + 1][0][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[options->num_timesteps + 1][0][c]]] * dxdu / Universe->LU;
				G[match_point_constraint_G_indices[options->num_timesteps + 1][1][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[options->num_timesteps + 1][1][c]]] * dydu / Universe->LU;
				G[match_point_constraint_G_indices[options->num_timesteps + 1][2][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[options->num_timesteps + 1][2][c]]] * dzdu / Universe->LU;
				G[match_point_constraint_G_indices[options->num_timesteps + 1][3][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[options->num_timesteps + 1][3][c]]] * dxdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[options->num_timesteps + 1][4][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[options->num_timesteps + 1][4][c]]] * dydotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[options->num_timesteps + 1][5][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[options->num_timesteps + 1][5][c]]] * dzdotdu / Universe->LU * Universe->TU;
				G[match_point_constraint_G_indices[options->num_timesteps + 1][6][c]] = options->X_scale_ranges[options->jGvar[match_point_constraint_G_indices[options->num_timesteps + 1][6][c]]] * dmdu  / (options->maximum_mass + journey_initial_mass_increment_scale_factor * current_mass_increment);
			}//end loop over controls
		}

		return 0;
	}

	//function to calculate the derivative of a match point constraint with respect to a decision variable in the forward propagation
	int MGA_LT_phase::calculate_match_point_forward_propagation_derivatives(double* G,
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
																			double& dtdu)
	{
		double dxdu_next, dydu_next, dzdu_next, dxdotdu_next, dydotdu_next, dzdotdu_next, dmdu_next, deltat;
		double dxdt, dydt, dzdt, dxdotdt, dydotdt, dzdotdt, dmdt;

		double x = spacecraft_state[stepnext - 1][0];
		double y = spacecraft_state[stepnext - 1][1];
		double z = spacecraft_state[stepnext - 1][2];
		double r = sqrt(x*x + y*y + z*z);
		double r3 = r*r*r;
		double umag = (math::norm(control[stepnext-1].data(),3) + 1.0e-10);

		deltat = (time_step_sizes[stepnext-1]*86400);
				
		
		double drdu = (spacecraft_state[stepnext-1][0]*dxdu + spacecraft_state[stepnext-1][1]*dydu + spacecraft_state[stepnext-1][2]*dzdu) / r;
		double mold = spacecraft_state[stepnext-1][6];

		//evaluate the time derivatives only when dtdu is nonzero, i.e. if u is a time variable
		if (fabs(dtdu) > 1.0e-8)
		{
			dxdt = spacecraft_state[stepnext][3];
			dydt = spacecraft_state[stepnext][4];
			dzdt = spacecraft_state[stepnext][5];
			//TODO partial derivatives of acceleration vector with repect to time!

			double rstepnext = math::norm(this->spacecraft_state[stepnext].data(), 3);
			double r3next = rstepnext * rstepnext * rstepnext;
			dxdotdt = ForceVector[stepnext][0]/mold - Universe->mu*x/r3next + dagravdtvec[stepnext][0];
			dydotdt = ForceVector[stepnext][1]/mold - Universe->mu*y/r3next + dagravdtvec[stepnext][1];
			dzdotdt = ForceVector[stepnext][2]/mold - Universe->mu*z/r3next + dagravdtvec[stepnext][2];
			dmdt = umag * options->engine_duty_cycle * available_mass_flow_rate[stepnext - 1];
		}
		else
		{
			dxdt = 0.0;
			dydt = 0.0;
			dzdt = 0.0;
			dxdotdt = 0.0;
			dydotdt = 0.0;
			dzdotdt = 0.0;
			dmdt = 0.0;
		}

		double ddVmaxdu = (options->engine_duty_cycle * deltat) *
							(dTdP[stepnext-1]/1000*dPdr[stepnext-1]/Universe->LU*drdu / mold
							- available_thrust[stepnext-1]/1000.0 / (mold*mold) * dmdu);

		double dVxplusdu = (dxdotdu + ddVmaxdu * control[stepnext-1][0] + dagravdRvec[stepnext - 1][0]*dxdu);
		double dVyplusdu = (dydotdu + ddVmaxdu * control[stepnext-1][1] + dagravdRvec[stepnext - 1][1]*dydu);
		double dVzplusdu = (dzdotdu + ddVmaxdu * control[stepnext-1][2] + dagravdRvec[stepnext - 1][2]*dzdu);

		dxdu_next = Forward_STM[stepnext](0,0) * dxdu + Forward_STM[stepnext](0,1) * dydu + Forward_STM[stepnext](0,2) * dzdu
					+ Forward_STM[stepnext](0,3) * dVxplusdu + Forward_STM[stepnext](0,4) * dVyplusdu + Forward_STM[stepnext](0,5) * dVzplusdu
					+ dxdt*dtdu;
		dydu_next = Forward_STM[stepnext](1,0) * dxdu + Forward_STM[stepnext](1,1) * dydu + Forward_STM[stepnext](1,2) * dzdu + 
					+ Forward_STM[stepnext](1,3) * dVxplusdu + Forward_STM[stepnext](1,4) * dVyplusdu + Forward_STM[stepnext](1,5) * dVzplusdu
					+ dydt*dtdu;
		dzdu_next = Forward_STM[stepnext](2,0) * dxdu + Forward_STM[stepnext](2,1) * dydu + Forward_STM[stepnext](2,2) * dzdu + 
					+ Forward_STM[stepnext](2,3) * dVxplusdu + Forward_STM[stepnext](2,4) * dVyplusdu + Forward_STM[stepnext](2,5) * dVzplusdu
					+ dzdt * dtdu;
		dxdotdu_next = Forward_STM[stepnext](3,0) * dxdu + Forward_STM[stepnext](3,1) * dydu + Forward_STM[stepnext](3,2) * dzdu + 
					+ Forward_STM[stepnext](3,3) * dVxplusdu + Forward_STM[stepnext](3,4) * dVyplusdu + Forward_STM[stepnext](3,5) * dVzplusdu
					+ dxdotdt * dtdu;
		dydotdu_next = Forward_STM[stepnext](4,0) * dxdu + Forward_STM[stepnext](4,1) * dydu + Forward_STM[stepnext](4,2) * dzdu + 
					+ Forward_STM[stepnext](4,3) * dVxplusdu + Forward_STM[stepnext](4,4) * dVyplusdu + Forward_STM[stepnext](4,5) * dVzplusdu
					+ dydotdt * dtdu;
		dzdotdu_next = Forward_STM[stepnext](5,0) * dxdu + Forward_STM[stepnext](5,1) * dydu + Forward_STM[stepnext](5,2) * dzdu + 
					+ Forward_STM[stepnext](5,3) * dVxplusdu + Forward_STM[stepnext](5,4) * dVyplusdu + Forward_STM[stepnext](5,5) * dVzplusdu
					+ dzdotdt * dtdu;
		dmdu_next = dmdu - (deltat * options->engine_duty_cycle)
							* (umag * dmdotdP[stepnext-1] * dPdr[stepnext-1]/Universe->LU * drdu) - dmdt * dtdu;

		dxdu = dxdu_next;
		dydu = dydu_next;
		dzdu = dzdu_next;
		dxdotdu = dxdotdu_next;
		dydotdu = dydotdu_next;
		dzdotdu = dzdotdu_next;
		dmdu = dmdu_next;

		return 0;
	}

	//function to calculate the derivative of a match point constraint with respect to a decision variable in the backward propagation
	int MGA_LT_phase::calculate_match_point_backward_propagation_derivatives(	double* G,
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
																				double& dtdu)
	{
		double dxdu_next, dydu_next, dzdu_next, dxdotdu_next, dydotdu_next, dzdotdu_next, dmdu_next, deltat;
		double dxdt, dydt, dzdt, dxdotdt, dydotdt, dzdotdt, dmdt;

		//translate into backward steps
		int backstepnext = options->num_timesteps - 1 - stepnext;

		double x = spacecraft_state[backstepnext - 1][0];
		double y = spacecraft_state[backstepnext - 1][1];
		double z = spacecraft_state[backstepnext - 1][2];
		double r = sqrt(x*x + y*y + z*z);
		double r3 = r*r*r;
		double umag = (math::norm(control[backstepnext-1].data(),3) + 1.0e-10);

		deltat = (time_step_sizes[backstepnext-1]*86400);
				
		
		double drdu = (spacecraft_state[backstepnext-1][0]*dxdu + spacecraft_state[backstepnext-1][1]*dydu + spacecraft_state[backstepnext-1][2]*dzdu) / r;
		double mold = spacecraft_state[backstepnext-1][6];

		//evaluate the time derivatives only when dtdu is nonzero, i.e. if u is a time variable
		if (fabs(dtdu) > 1.0e-8)
		{
			dxdt = spacecraft_state[backstepnext][3];
			dydt = spacecraft_state[backstepnext][4];
			dzdt = spacecraft_state[backstepnext][5];
			double rbackstepnext = math::norm(this->spacecraft_state[backstepnext].data(), 3);
			double r3next = rbackstepnext * rbackstepnext * rbackstepnext;
			//TODO partial derivatives of acceleration vector with repect to time!
			dxdotdt = ForceVector[backstepnext][0]/mold - Universe->mu*x/r3next + dagravdtvec[backstepnext][0];
			dydotdt = ForceVector[backstepnext][1]/mold - Universe->mu*y/r3next + dagravdtvec[backstepnext][1];
			dzdotdt = ForceVector[backstepnext][2]/mold - Universe->mu*z/r3next + dagravdtvec[backstepnext][2];
			dmdt = umag * options->engine_duty_cycle * available_mass_flow_rate[backstepnext - 1];
		}
		else
		{
			dxdt = 0.0;
			dydt = 0.0;
			dzdt = 0.0;
			dxdotdt = 0.0;
			dydotdt = 0.0;
			dzdotdt = 0.0;
			dmdt = 0.0;
		}

		double ddVmaxdu = (options->engine_duty_cycle * deltat) * 
							( 1.0 / mold * dTdP[backstepnext-1]/1000*dPdr[backstepnext - 1]/Universe->LU*drdu
							+ available_thrust[backstepnext-1]/1000 * (-1.0 / (mold*mold) * dmdu));

		double dVxplusdu = (dxdotdu - ddVmaxdu * control[backstepnext - 1][0] + dagravdRvec[backstepnext - 1][0]*dxdu);
		double dVyplusdu = (dydotdu - ddVmaxdu * control[backstepnext - 1][1] + dagravdRvec[backstepnext - 1][1]*dydu);
		double dVzplusdu = (dzdotdu - ddVmaxdu * control[backstepnext - 1][2] + dagravdRvec[backstepnext - 1][2]*dzdu);

		dxdu_next = Backward_STM[stepnext+1](0,0) * dxdu + Backward_STM[stepnext+1](0,1) * dydu + Backward_STM[stepnext+1](0,2) * dzdu
					+ Backward_STM[stepnext+1](0,3) * dVxplusdu + Backward_STM[stepnext+1](0,4) * dVyplusdu + Backward_STM[stepnext+1](0,5) * dVzplusdu
					+ dxdt * dtdu;
		dydu_next = Backward_STM[stepnext+1](1,0) * dxdu + Backward_STM[stepnext+1](1,1) * dydu + Backward_STM[stepnext+1](1,2) * dzdu + 
					+ Backward_STM[stepnext+1](1,3) * dVxplusdu + Backward_STM[stepnext+1](1,4) * dVyplusdu + Backward_STM[stepnext+1](1,5) * dVzplusdu
					+ dydt * dtdu;
		dzdu_next = Backward_STM[stepnext+1](2,0) * dxdu + Backward_STM[stepnext+1](2,1) * dydu + Backward_STM[stepnext+1](2,2) * dzdu + 
					+ Backward_STM[stepnext+1](2,3) * dVxplusdu + Backward_STM[stepnext+1](2,4) * dVyplusdu + Backward_STM[stepnext+1](2,5) * dVzplusdu
					+ dzdt * dtdu;
		dxdotdu_next = Backward_STM[stepnext+1](3,0) * dxdu + Backward_STM[stepnext+1](3,1) * dydu + Backward_STM[stepnext+1](3,2) * dzdu + 
					+ Backward_STM[stepnext+1](3,3) * dVxplusdu + Backward_STM[stepnext+1](3,4) * dVyplusdu + Backward_STM[stepnext+1](3,5) * dVzplusdu
					+ dxdotdt * dtdu;
		dydotdu_next = Backward_STM[stepnext+1](4,0) * dxdu + Backward_STM[stepnext+1](4,1) * dydu + Backward_STM[stepnext+1](4,2) * dzdu + 
					+ Backward_STM[stepnext+1](4,3) * dVxplusdu + Backward_STM[stepnext+1](4,4) * dVyplusdu + Backward_STM[stepnext+1](4,5) * dVzplusdu
					+ dydotdt * dtdu;
		dzdotdu_next = Backward_STM[stepnext+1](5,0) * dxdu + Backward_STM[stepnext+1](5,1) * dydu + Backward_STM[stepnext+1](5,2) * dzdu + 
					+ Backward_STM[stepnext+1](5,3) * dVxplusdu + Backward_STM[stepnext+1](5,4) * dVyplusdu + Backward_STM[stepnext+1](5,5) * dVzplusdu
					+ dzdotdt * dtdu;
		dmdu_next = dmdu - (deltat * options->engine_duty_cycle)
							* (umag * dmdotdP[backstepnext-1] * dPdr[backstepnext-1]/Universe->LU  * drdu) - dmdt * dtdu;

		dxdu = dxdu_next;
		dydu = dydu_next;
		dzdu = dzdu_next;
		dxdotdu = dxdotdu_next;
		dydotdu = dydotdu_next;
		dzdotdu = dzdotdu_next;
		dmdu = dmdu_next;

		return 0;
	}

} /* namespace EMTG */

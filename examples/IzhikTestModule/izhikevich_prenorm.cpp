/*
 *  izhikevich.cpp
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// C++ includes:
#include <limits>

// Includes from libnestutil:
#include "numerics.h"

// Includes from nestkernel:
#include "event_delivery_manager_impl.h"
#include "exceptions.h"
#include "kernel_manager.h"
#include "universal_data_logger_impl.h"
#include "lookback_node.h"

#include "../models/stdp_connection.h"

// Includes from sli:
#include "dict.h"
#include "dictutils.h"
#include "doubledatum.h"
#include "integerdatum.h"

#include "izhikevich_prenorm.h"

/* ----------------------------------------------------------------
 * Recordables map
 * ---------------------------------------------------------------- */

nest::RecordablesMap< mynest::izhikevich_prenorm > mynest::izhikevich_prenorm::recordablesMap_;

namespace nest
{
// Override the create() method with one call to nest::RecordablesMap::insert_()
// for each quantity to be recorded.

template <>
void nest::RecordablesMap< mynest::izhikevich_prenorm >::create()
{
  // use standard nest::names whereever you can for consistency!
  insert_( nest::names::V_m, &mynest::izhikevich_prenorm::get_V_m_ );
  insert_( nest::names::U_m, &mynest::izhikevich_prenorm::get_U_m_ );
  insert_( "inc_weight_sum", &mynest::izhikevich_prenorm::get_inc_weight_sum);
}
}


/* ----------------------------------------------------------------
 * Default constructors defining default parameters and state
 * ---------------------------------------------------------------- */


mynest::izhikevich_prenorm::Parameters_::Parameters_()
    : a_( 0.02 )                                      // a
    , b_( 0.2 )                                       // b
    , c_( -65.0 )                                     // c without unit
    , d_( 8.0 )                                       // d
    , I_e_( 0.0 )                                     // pA
    , V_th_( 30.0 )                                   // mV
    , V_min_( -std::numeric_limits< double >::max() ) // mV
    , norm_period_(nest::Time(nest::Time::ms(100l)).get_steps())                               // steps
    , norm_value_(30.0)                              // pA
    , consistent_integration_( true )
{
}

mynest::izhikevich_prenorm::State_::State_()
    : v_( -65.0 ) // membrane potential
    , u_( 0.0 )   // membrane recovery variable
    , I_( 0.0 )   // input current
{
}

/* ----------------------------------------------------------------
 * Parameter and state extractions and manipulation functions
 * ---------------------------------------------------------------- */

void mynest::izhikevich_prenorm::Parameters_::get( DictionaryDatum& d ) const
{
  def< double >( d, nest::names::I_e, I_e_ );
  def< double >( d, nest::names::V_th, V_th_ ); // threshold value
  def< double >( d, nest::names::V_min, V_min_ );
  def< double >( d, nest::names::a, a_ );
  def< double >( d, nest::names::b, b_ );
  def< double >( d, nest::names::c, c_ );
  def< double >( d, nest::names::d, d_ );
  def< double >( d, "norm_period", nest::Time::delay_steps_to_ms(norm_period_) );
  def< double >( d, "norm_value", norm_value_ );

  def< bool >( d, nest::names::consistent_integration, consistent_integration_ );
}

void mynest::izhikevich_prenorm::Parameters_::set( const DictionaryDatum& d )
{

  updateValue< double >( d, nest::names::V_th, V_th_ );
  updateValue< double >( d, nest::names::V_min, V_min_ );
  updateValue< double >( d, nest::names::I_e, I_e_ );
  updateValue< double >( d, nest::names::a, a_ );
  updateValue< double >( d, nest::names::b, b_ );
  updateValue< double >( d, nest::names::c, c_ );
  updateValue< double >( d, nest::names::d, d_ );
  updateValue< bool >(
      d, nest::names::consistent_integration, consistent_integration_ );

  long norm_period_in_ms=0;
  updateValue< double >( d, "norm_period", norm_period_in_ms );
  norm_period_ = nest::Time(nest::Time::ms(norm_period_in_ms)).get_steps();

  if (norm_period_ < nest::kernel().connection_manager.get_min_delay()) {
    throw nest::BadParameter("norm_period must be at least equal to the smallest nest::delay in the network");
  }

  updateValue< double >( d, "norm_value", norm_value_);
  const double h = nest::Time::get_resolution().get_ms();
  if ( not consistent_integration_ && h != 1.0 )
  {
    LOG(
        nest::M_INFO, "Parameters_::set", "Use 1.0 ms as resolution for consistency." );
  }
}

void mynest::izhikevich_prenorm::State_::get( DictionaryDatum& d, const Parameters_& ) const
{
  def< double >( d, nest::names::U_m, u_ ); // Membrane potential recovery variable
  def< double >( d, nest::names::V_m, v_ ); // Membrane potential
}

void mynest::izhikevich_prenorm::State_::set( const DictionaryDatum& d, const Parameters_& )
{
  updateValue< double >( d, nest::names::U_m, u_ );
  updateValue< double >( d, nest::names::V_m, v_ );
}

mynest::izhikevich_prenorm::Buffers_::Buffers_( izhikevich_prenorm& n )
    : logger_( n )
{
}

mynest::izhikevich_prenorm::Buffers_::Buffers_( const Buffers_&, izhikevich_prenorm& n )
    : logger_( n )
{
}

/* ----------------------------------------------------------------
 * Default and copy constructor for node
 * ---------------------------------------------------------------- */

mynest::izhikevich_prenorm::izhikevich_prenorm()
    : nest::Archiving_Node()
    , P_()
    , S_()
    , B_( *this )
{
  recordablesMap_.create();
}

mynest::izhikevich_prenorm::izhikevich_prenorm( const izhikevich_prenorm& n )
    : nest::Archiving_Node( n )
    , P_( n.P_ )
    , S_( n.S_ )
    , B_( n.B_, *this )
{
}

/* ----------------------------------------------------------------
 * nest::Node initialization functions
 * ---------------------------------------------------------------- */

void mynest::izhikevich_prenorm::init_state_( const nest::Node& proto )
{
  const izhikevich_prenorm& pr = downcast< izhikevich_prenorm >( proto );
  S_ = pr.S_;
}

void mynest::izhikevich_prenorm::init_buffers_()
{
  B_.spikes_.clear();   // includes resize
  B_.currents_.clear(); // includes resize
  B_.logger_.reset();   // includes resize
  nest::Archiving_Node::clear_history();
}

void mynest::izhikevich_prenorm::calibrate()
{
  // Perform verification of incoming synapse pointers here
  for(auto &inc_conn_ptr : nest::LookBackNode<ConnectionT>::incoming_syn_ptr_set) {
    if (inc_conn_ptr->get_target(nest::kernel().vp_manager.get_thread_id())->get_gid() != this->get_gid()) {
      throw  nest::KernelException("InvalidIncPointers");
    }
  }
  B_.logger_.init();
}

/* ----------------------------------------------------------------
 * Update and spike handling functions
 */

void mynest::izhikevich_prenorm::update( nest::Time const& origin, const long from, const long to )
{
  assert(
      to >= 0 && ( nest::delay ) from < nest::kernel().connection_manager.get_min_delay() );
  assert( from < to );

  const double h = nest::Time::get_resolution().get_ms();
  double v_old, u_old;

  for ( long lag = from; lag < to; ++lag )
  {
    // neuron is never refractory
    // use standard forward Euler numerics in this case
    if ( P_.consistent_integration_ )
    {
      v_old = S_.v_;
      u_old = S_.u_;
      S_.v_ += h * ( 0.04 * v_old * v_old + 5.0 * v_old + 140.0 - u_old + S_.I_
                     + P_.I_e_ )
               + B_.spikes_.get_value( lag );
      S_.u_ += h * P_.a_ * ( P_.b_ * v_old - u_old );
    }
      // use numerics published in Izhikevich (2003) in this case (not
      // recommended)
    else
    {
      double I_syn = B_.spikes_.get_value( lag );
      S_.v_ += h * 0.5 * ( 0.04 * S_.v_ * S_.v_ + 5.0 * S_.v_ + 140.0 - S_.u_
                           + S_.I_ + P_.I_e_ + I_syn );
      S_.v_ += h * 0.5 * ( 0.04 * S_.v_ * S_.v_ + 5.0 * S_.v_ + 140.0 - S_.u_
                           + S_.I_ + P_.I_e_ + I_syn );
      S_.u_ += h * P_.a_ * ( P_.b_ * S_.v_ - S_.u_ );
    }

    // lower bound of membrane potential
    S_.v_ = ( S_.v_ < P_.V_min_ ? P_.V_min_ : S_.v_ );

    // threshold crossing
    if ( S_.v_ >= P_.V_th_ )
    {
      S_.v_ = P_.c_;
      S_.u_ = S_.u_ + P_.d_;

      // compute spike time
      set_spiketime( nest::Time::step( origin.get_steps() + lag + 1 ) );

      nest::SpikeEvent se;
      nest::kernel().event_delivery_manager.send( *this, se, lag );
    }

    // set new input current
    S_.I_ = B_.currents_.get_value( lag );

    // Here, we try to normalize incoming weights
    if ((origin.get_steps() + lag) % P_.norm_period_ == 0) {
//      std::cout << "Trying To Normalize at time " << origin.get_ms()
//                << ": N_incoming = " << nest::LookBackNode<ConnectionT>::incoming_syn_ptr_set.size()
//                << "inc_weight_sum = " << weight_sum << std::endl;
      double weight_sum = get_inc_weight_sum();
      for(auto &conn_ptr : nest::LookBackNode<ConnectionT>::incoming_syn_ptr_set) {
        conn_ptr->set_weight(conn_ptr->get_weight()*P_.norm_value_/weight_sum);
      }
    }

    // voltage logging
    B_.logger_.record_data( origin.get_steps() + lag );
  }
}

void mynest::izhikevich_prenorm::handle( nest::SpikeEvent& e )
{
  assert( e.get_delay() > 0 );
  B_.spikes_.add_value(
      e.get_rel_delivery_steps( nest::kernel().simulation_manager.get_slice_origin() ),
      e.get_weight() * e.get_multiplicity() );
}

void mynest::izhikevich_prenorm::handle( nest::CurrentEvent& e )
{
  assert( e.get_delay() > 0 );

  const double c = e.get_current();
  const double w = e.get_weight();
  B_.currents_.add_value(
      e.get_rel_delivery_steps( nest::kernel().simulation_manager.get_slice_origin() ),
      w * c );
}

void mynest::izhikevich_prenorm::handle( nest::DataLoggingRequest& e )
{
  B_.logger_.handle( e );
}

double mynest::izhikevich_prenorm::get_inc_weight_sum() const {

  double weight_sum = 0;
  for(auto &conn_ptr : nest::LookBackNode<ConnectionT>::incoming_syn_ptr_set) {
    weight_sum += conn_ptr->get_weight();
  }
  return weight_sum;
}

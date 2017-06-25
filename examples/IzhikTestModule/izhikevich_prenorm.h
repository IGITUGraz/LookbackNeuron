/*
 *  izhikevich.h
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

#ifndef IZHIKEVICH_H
#define IZHIKEVICH_H

// Includes from nestkernel:
#include "archiving_node.h"
#include "connection.h"
#include "event.h"
#include "nest_types.h"
#include "ring_buffer.h"
#include "universal_data_logger.h"

#include "../models/stdp_connection.h"
#include "target_identifier.h"

#include "lookback_node.h"

namespace mynest
{

typedef nest::STDPConnection<nest::TargetIdentifierPtrRport> ConnectionT;

/* BeginDocumentation
   Name: izhikevich_prenorm - Izhikevich neuron model with incoming synaptic
                              normalization

   Description:
   Implementation of the simple spiking neuron model introduced by Izhikevich
   [1]. The dynamics are given by:
       dv/dt = 0.04*v^2 + 5*v + 140 - u + I
          du/dt = a*(b*v - u)

       if v >= V_th:
         v is set to c
         u is incremented by d

       v jumps on each spike arrival by the weight of the spike.

   As published in [1], the numerics differs from the standard forward Euler
   technique in two ways:
   1) the new value of u is calculated based on the new value of v, rather than
   the previous value
   2) the variable v is updated using a time step half the size of that used to
   update variable u.

   This model offers both forms of integration, they can be selected using the
   boolean parameter consistent_integration. To reproduce some results published
   on the basis of this model, it is necessary to use the published form of the
   dynamics. In this case, consistent_integration must be set to false. For all
   other purposes, it is recommended to use the standard technique for forward
   Euler integration. In this case, consistent_integration must be set to true
   (default).


   Parameters:
   The following parameters can be set in the status dictionary.

   V_m        double - Membrane potential in mV
   U_m        double - Membrane potential recovery variable
   V_th       double - Spike threshold in mV.
   I_e        double - Constant input current in pA. (R=1)
   V_min      double - Absolute lower value for the membrane potential.
   a          double - describes time scale of recovery variable
   b          double - sensitivity of recovery variable
   c          double - after-spike reset value of V_m
   d          double - after-spike reset value of U_m
   consistent_integration  bool - use standard integration technique


   References:
   [1] Izhikevich, Simple Model of Spiking Neurons,
   IEEE Transactions on Neural Networks (2003) 14:1569-1572

   Sends: nest::SpikeEvent

   Receives: nest::SpikeEvent, nest::CurrentEvent, nest::DataLoggingRequest
   FirstVersion: 2009
   Author: Hanuschkin, Morrison, Kunkel
   SeeAlso: iaf_psc_delta, mat2_psc_exp
*/
class izhikevich_prenorm : public nest::Archiving_Node, public nest::LookBackNode<ConnectionT>
{

public:
  izhikevich_prenorm();
  izhikevich_prenorm( const izhikevich_prenorm& );

  /**
   * Import sets of overloaded virtual functions.
   * @see Technical Issues / Virtual Functions: Overriding, Overloading, and
   * Hiding
   */
  using nest::Node::handle;
  using nest::Node::handles_test_event;

  void handle( nest::DataLoggingRequest& );
  void handle( nest::SpikeEvent& );
  void handle( nest::CurrentEvent& );

  nest::port handles_test_event( nest::DataLoggingRequest&, nest::rport );
  nest::port handles_test_event( nest::SpikeEvent&, nest::rport );
  nest::port handles_test_event( nest::CurrentEvent&, nest::rport );

  nest::port send_test_event( nest::Node&, nest::rport, nest::synindex, bool );

  void get_status( DictionaryDatum& ) const;
  void set_status( const DictionaryDatum& );

protected:

  void update( nest::Time const&, const long, const long );

  void init_state_( const nest::Node& proto );
  void init_buffers_();
  void calibrate();

private:
  friend class nest::RecordablesMap< izhikevich_prenorm >;
  friend class nest::UniversalDataLogger< izhikevich_prenorm >;

  // ----------------------------------------------------------------

  /**
   * Independent parameters of the model.
   */
  struct Parameters_
  {
    double a_;
    double b_;
    double c_;
    double d_;

    /** External DC current */
    double I_e_;

    /** Threshold */
    double V_th_;

    /** Lower bound */
    double V_min_;

    /** Normalization parameters in steps*/
    long norm_period_; // Normalization period in steps
    double norm_value_; // value of sum of weights

    /** Use standard integration numerics **/
    bool consistent_integration_;

    Parameters_(); //!< Sets default parameter values

    void get( DictionaryDatum& ) const; //!< Store current values in dictionary
    void set( const DictionaryDatum& ); //!< Set values from dicitonary
  };

  // ----------------------------------------------------------------

  /**
   * State variables of the model.
   */
  struct State_
  {
    double v_; // membrane potential
    double u_; // membrane recovery variable
    double I_; // input current
    double inc_weight_sum_; //sum of incoming synapse weights

    /** Accumulate spikes arriving during refractory period, discounted for
        decay until end of refractory period.
    */

    State_(); //!< Default initialization

    void get( DictionaryDatum&, const Parameters_& ) const;
    void set( const DictionaryDatum&, const Parameters_& );
  };

  // ----------------------------------------------------------------

  /**
   * Buffers of the model.
   */
  struct Buffers_
  {
    /**
     * Buffer for recording
     */
    Buffers_( izhikevich_prenorm& );
    Buffers_( const Buffers_&, izhikevich_prenorm& );
    nest::UniversalDataLogger< izhikevich_prenorm > logger_;

    /** buffers and sums up incoming spikes/currents */
    nest::RingBuffer spikes_;
    nest::RingBuffer currents_;
  };

  // ----------------------------------------------------------------

  /**
   * Internal variables of the model.
   */
  struct Variables_
  {
  };

  // Access functions for nest::UniversalDataLogger -----------------------

  //! Read out the membrane potential
  double
  get_V_m_() const
  {
    return S_.v_;
  }
  //! Read out the recovery variable
  double
  get_U_m_() const
  {
    return S_.u_;
  }
  //! Read out sum of incoming weights
  double get_inc_weight_sum() const;

  // ----------------------------------------------------------------

  Parameters_ P_;
  State_ S_;
  Variables_ V_;
  Buffers_ B_;

  //! Mapping of recordables nest::names to access functions
  static nest::RecordablesMap< izhikevich_prenorm > recordablesMap_;
  /** @} */
};

inline nest::port
izhikevich_prenorm::send_test_event( nest::Node& target, nest::rport receptor_type, nest::synindex, bool )
{
  nest::SpikeEvent e;
  e.set_sender( *this );

  return target.handles_test_event( e, receptor_type );
}

inline nest::port
izhikevich_prenorm::handles_test_event( nest::SpikeEvent&, nest::rport receptor_type )
{
  if ( receptor_type != 0 )
    throw nest::UnknownReceptorType( receptor_type, get_name() );
  return 0;
}

inline nest::port
izhikevich_prenorm::handles_test_event( nest::CurrentEvent&, nest::rport receptor_type )
{
  if ( receptor_type != 0 )
    throw nest::UnknownReceptorType( receptor_type, get_name() );
  return 0;
}

inline nest::port
izhikevich_prenorm::handles_test_event( nest::DataLoggingRequest& dlr, nest::rport receptor_type )
{
  if ( receptor_type != 0 )
    throw nest::UnknownReceptorType( receptor_type, get_name() );
  return B_.logger_.connect_logging_device( dlr, recordablesMap_ );
}

inline void
izhikevich_prenorm::get_status( DictionaryDatum& d ) const
{
  P_.get( d );
  S_.get( d, P_ );
  nest::Archiving_Node::get_status( d );
  ( *d )[ nest::names::recordables ] = recordablesMap_.get_list();
}

inline void
izhikevich_prenorm::set_status( const DictionaryDatum& d )
{
  Parameters_ ptmp = P_; // temporary copy in case of errors
  ptmp.set( d );         // throws if BadProperty
  State_ stmp = S_;      // temporary copy in case of errors
  stmp.set( d, ptmp );   // throws if BadProperty

  // We now know that (ptmp, stmp) are consistent. We do not
  // write them back to (P_, S_) before we are also sure that
  // the properties to be set in the parent class are internally
  // consistent.
  nest::Archiving_Node::set_status( d );

  // if we get here, temporaries contain consistent set of properties
  P_ = ptmp;
  S_ = stmp;
}

} // namespace nest

#endif /* #ifndef IZHIKEVICH_H */

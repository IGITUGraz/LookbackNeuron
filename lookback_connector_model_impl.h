#ifndef LOOKBACK_CONNECTOR_MODEL_IMPL
#define LOOKBACK_CONNECTOR_MODEL_IMPL

#include "lookback_connector_model.h"
#include "connector_model_impl.h"

#include <vector>

namespace nest
{
///////////////////////////////////////////////////////////////////////////
// LOOK BACK CONNECTOR MODEL //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template <typename ConnectionT>
std::vector<ConnectionT *> LookBackConnectorModel<ConnectionT>::get_conn_ptrs(ConnectorBase *conn_base_in, index syn_id) {
  // This function makes the following assumptions:
  //
  // 1.  The conn_base_in pointer is the pointer that is passed into `this->add_connection()`
  //     or the one returned from `this->GenericConnectorModel<ConnectionT>::add_connection()`
  //
  // 2.  The above basically imples that this is NOT a validated pointer and the LSB's
  //     still contain the information regarding has_primary and has_secondary (see the
  //     functions validate_pointer(), has_primary(), and has_secondary() in connector_model.h
  //
  // 3.  Also, since the conn_base_in can be NULL as a result of there being no connection from
  //     the given source neuron, this case is taken care of.
  //
  // The following points regarding the structure of a ConnectorBase have been kept in
  // mind while designing
  //
  // 1.  ConnectorBase has a different structure depending on whether it is homogeneous or
  //     heterogeneous. In each case, the appropriate functions handle it.
  //
  // 2.  In all cases. all assumptions of a called function that are stated are met.

  // Extract information from 2 LSBs
  conn_base_in = validate_pointer(conn_base_in);

  if (conn_base_in) {
    // in case there is a connector_base
    if (conn_base_in->homogeneous_model()) {
      // This is the case where the ConnectorBase is a homogenous connector
      if (is_matching_syn_id(conn_base_in, syn_id)){
        return get_conn_ptrs_hom(conn_base_in);
      }
      else {
        return std::vector<ConnectionT *>();
      }
    }
    else {
      // This is the case where the ConnectorBase is a heterogenous connector

      // Note that the synid passed to this function represents the type of
      // the synapse inserted in add_connection, which is expected to refer to
      // the type of LookBackConnectorModel<ConnectionT>

      return get_conn_ptrs_het(conn_base_in, syn_id);
    }
  }
  else {
    // in case there is no connection of the source node
    return std::vector<ConnectionT *>();
  }
}

template <typename  ConnectionT>
void LookBackConnectorModel<ConnectionT>::update_conn_ptrs(const std::vector<ConnectionT *> &old_conn_ptrs,
                                              const std::vector<ConnectionT *> &new_conn_ptrs) {

  // Assumptions:
  //
  // Regarding Connection Pointers:
  //
  // 1. Each pointer in `new_conn_ptrs` is a valid pointer pointing to a synapse of
  //    model LookBackConnectorModel<ConnectionT>. (satisfied by get_conn_ptr)
  //
  // 2. All pointers in `new_conn_ptrs` point to connections of the above type in a
  //    single connector. (satisfied by get_conn_ptr)
  //
  // 3. The pointers in `new_conn_ptrs` having satisfied the above two assumptions
  //    will basically be pointing to the elements of the vector used to store the
  //    connections in the connector. It is necessary that should be stored in the
  //    same order as the elements in the vector. In other words this will imply
  //    that the values of the pointers will be sequential. (satisfied by get_conn_ptr)
  //
  // 4. This function is designed to update pointers after the addition of a SINGLE
  //    connection. Furthermore we assume that there is NO DELETION OF SYNAPSES.
  //    Combined, this translates to the assumption that
  //
  //      length(new_conn_ptrs) = length(old_conn_ptrs)+1
  //
  //
  // 5. In continuation with 3. and 4. we assume in addition, that there is a one-
  //    to-one correspondence between old_conn_ptrs[1..end] and new_conn_ptrs[1..end-1].
  //    This also means that the new synapse to be inserted is at the back of
  //    new_conn_ptrs.
  //
  // 6. It is also assumed that the target neuron of the new synapse has already
  //    been validated to have inherited from LookBackNode<ConnectionT>.
  //

  assert (old_conn_ptrs.size() + 1 == new_conn_ptrs.size());

  // First, we check if the two vectors are identical i.e. there were no
  // reallocations. This is done by checking if old_conn_ptrs is empty or they
  // have identical first elements. This is sufficient because of 3. 4. & 5.
  if (!(old_conn_ptrs.empty() || old_conn_ptrs[0] == new_conn_ptrs[0])) {

    // in case they are not, we need to replace the relevant synapses across all
    // the neurons to whom these synapses point. As far as thread safety goes, there
    // is no need to worry as by definition of connection_manager.connections_,
    // all the connections on a single connector (assum 2.) point only to neurons
    // that are accessed on the same thread as the current one.

    for (auto old_iter=old_conn_ptrs.begin(), new_iter=new_conn_ptrs.begin();
         old_iter != old_conn_ptrs.end();
         ++old_iter, ++new_iter) {

      // It is assumed that this will work. assump 1. implies that all pointers here
      // are connected using LookBackConnectorModel. This means that their target is
      // already validated to have inherited from LookBackNode<ConnectionT>.
      auto target_node = dynamic_cast< LookBackNode<ConnectionT>* >(
          // Get the Target (Node*) of the connection that is pointed to by the (element
          // that is pointed to by new_iter)
          (*new_iter)->get_target(kernel().vp_manager.get_thread_id())
      );

      target_node->replace_inc_synapse(*old_iter, *new_iter);
    }
  }

  // Now, having assumed that the last element of new_conn_ptrs is the new connection, we
  // will perform the relevant addition to the set (note assump 6. for reinterpret_cast)

  auto val_target_node = dynamic_cast< LookBackNode<ConnectionT>* >(
      // Get the Target of the connection that is pointed to by the last
      // element of new_conn_ptrs
      new_conn_ptrs.back()->get_target(kernel().vp_manager.get_thread_id())
  );
  val_target_node->add_inc_synapse(new_conn_ptrs.back());

}


template <typename ConnectionT>
LookBackNode<ConnectionT>* LookBackConnectorModel<ConnectionT>::get_validated_neuron(Node *node_in) {

  auto val_target_node = dynamic_cast<LookBackNode<ConnectionT>*>(node_in);

  if (!val_target_node) {
    std::ostringstream msg;
    msg << "The node being connected to by a LookBackConnectorModel is not itself derived "
        << "from the appropriate LookBackNode";
    throw IllegalConnection(msg.str());
  }

  return val_target_node;
}


template <typename ConnectionT>
ConnectorBase* LookBackConnectorModel<ConnectionT>::add_connection(Node &src, Node &tgt, ConnectorBase *conn, synindex syn_id,
                                                      DictionaryDatum &d, double weight, double delay) {
  // Assumptions:
  //
  // 1. Adding a synapse to a connector will only result in the re-allocation of the
  //    hom-connector to which the synapse is allocated.
  //
  // Ensured:
  //
  // 1. A synapse is connected if and only if the target neuron inherits from LookBackNode<ConnectionT>.
  // 2. At each step, all tarrget neurons that have ever been connected to via
  //    add_connection, possesses valid pointers to the incoming synapses

  // First we perform target neuron validation
  get_validated_neuron(&tgt);

  // Then we read out the vector of old pointers to the connections
  std::vector<ConnectionT*> old_conn_ptrs = get_conn_ptrs(conn, syn_id);

  // Then we add synapse by calling super function from GenericConnectorModel
  ConnectorBase* new_conn_base_ptr = GenericConnectorModel<ConnectionT>::add_connection(src, tgt, conn, syn_id, d, weight, delay);

  // Then we read out the vector of new pointers to the connections
  // This includes te newly added synapse
  std::vector<ConnectionT*> new_conn_ptrs = get_conn_ptrs(new_conn_base_ptr, syn_id);

  // Now we update the connection pointers on all the relevant target neurons
  update_conn_ptrs(old_conn_ptrs, new_conn_ptrs);

  return new_conn_base_ptr;
}


template <typename ConnectionT>
ConnectorBase* LookBackConnectorModel<ConnectionT>::add_connection(Node &src, Node &tgt, ConnectorBase *conn, synindex syn_id,
                                                      double weight, double delay) {

  // First we perform target neuron validation
  get_validated_neuron(&tgt);

  // Then we read out the vector of old pointers to the connections
  std::vector<ConnectionT*> old_conn_ptrs = get_conn_ptrs(conn, syn_id);

  // Then we add synapse by calling super function from GenericConnectorModel
  ConnectorBase* new_conn_base_ptr = GenericConnectorModel<ConnectionT>::add_connection(src, tgt, conn, syn_id, weight, delay);

  // Then we read out the vector of new pointers to the connections
  // This includes te newly added synapse
  std::vector<ConnectionT*> new_conn_ptrs = get_conn_ptrs(new_conn_base_ptr, syn_id);

  // Now we update the connection pointers on all the relevant target neurons
  update_conn_ptrs(old_conn_ptrs, new_conn_ptrs);

  return new_conn_base_ptr;
}

template <typename ConnectionT>
bool LookBackConnectorModel<ConnectionT>::is_matching_syn_id(ConnectorBase *conn_base_in, index syn_id) {

  // Assumptions:
  //
  // 1. conn_base_in is a validated pointer (i.e. with 2 LSB's 0)

  return conn_base_in->homogeneous_model() && conn_base_in->get_syn_id() == syn_id;
}

template <typename ConnectionT>
std::vector<ConnectionT *> LookBackConnectorModel<ConnectionT>::get_conn_ptrs_hom(ConnectorBase *conn_base_in) {

  // Assumptions:
  //
  // 1. conn_base_in is a validated pointer (i.e. with 2 LSB's 0)
  // 2. conn_base_in represents a homogeneous connector
  // 3. conn_base_in contains synapses of type ConnectionT
  //

  std::vector<ConnectionT *> conn_ptrs_out;

  // Cast connector to vector_like<ConnectionT>. Guaranteed by assumptions 1, 2
  auto hom_conn_base = static_cast<vector_like<ConnectionT>* >(conn_base_in);

  for(size_t i=0; i< hom_conn_base->size(); ++i) {
    conn_ptrs_out.push_back( &(hom_conn_base->at(i)) );
  }

  return conn_ptrs_out;
}

template <typename ConnectionT>
std::vector<ConnectionT *> LookBackConnectorModel<ConnectionT>::get_conn_ptrs_het(ConnectorBase *conn_base_in, index syn_id) {

  // Assumptions:
  //
  // 1. conn_base_in is a validated pointer (i.e. with 2 LSB's 0)
  // 2. conn_base_in represents a heterogeneous connector

  // Casting conn_base_in to HetConnector. Safe thanksto assumption 1, 2
  auto het_conn = static_cast<HetConnector*>(conn_base_in);

  // Finding the Homogenous connector with matching type
  vector_like<ConnectionT>* matching_hom_conn = NULL;
  for(size_t i=0; i < het_conn->size(); ++i) {
    if (is_matching_syn_id(het_conn->at(i), syn_id)) {
      matching_hom_conn = static_cast< vector_like<ConnectionT>* >(het_conn->at(i));
      break;
    }
  }

  // If there is a homogenous connector of the right type, then return pointers from it
  // else return empty vector
  if(matching_hom_conn) {
    return get_conn_ptrs_hom(matching_hom_conn);
  }
  else {
    return std::vector<ConnectionT *>();
  }
}

template <typename ConnectionT>
ConnectorModel *LookBackConnectorModel<ConnectionT>::clone(std::string name) const {
  return new LookBackConnectorModel<ConnectionT>( *this, name );
}

}

#endif
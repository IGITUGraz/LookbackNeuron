#ifndef LOOKBACK_CONNECTOR_MODEL_H
#define LOOKBACK_CONNECTOR_MODEL_H

#include "connector_model.h"
#include "lookback_node.h"

#include <vector>

namespace nest
{

template <typename ConnectionT>
class LookBackConnectorModel : public GenericConnectorModel<ConnectionT> {
public:
  LookBackConnectorModel( const std::string name,
                          bool is_primary,
                          bool has_delay,
                          bool requires_symmetric)
      : GenericConnectorModel<ConnectionT>( name, is_primary, has_delay, requires_symmetric=requires_symmetric)
  {
  }

  LookBackConnectorModel( const LookBackConnectorModel& cm,
                          const std::string name )
      : GenericConnectorModel<ConnectionT>( cm, name )
  {
  }

private:
  inline bool is_matching_syn_id(ConnectorBase *conn_base_in, index syn_id);
  inline std::vector<ConnectionT *> get_conn_ptrs_hom(ConnectorBase * conn_base_in);
  inline std::vector<ConnectionT *> get_conn_ptrs_het(ConnectorBase * conn_base_in, index syn_id);
  inline std::vector<ConnectionT *> get_conn_ptrs(ConnectorBase * conn_base_in, index synid);
  inline void update_conn_ptrs(const std::vector<ConnectionT*> &old_conn_ptrs, const std::vector<ConnectionT*> &new_conn_ptrs);
  LookBackNode<ConnectionT>* get_validated_neuron(Node *node_in);

public:
  ConnectorBase* add_connection( Node& src,
                                 Node& tgt,
                                 ConnectorBase* conn,
                                 synindex syn_id,
                                 double delay,
                                 double weight);
  ConnectorBase* add_connection( Node& src,
                                 Node& tgt,
                                 ConnectorBase* conn,
                                 synindex syn_id,
                                 DictionaryDatum& d,
                                 double delay,
                                 double weight );

  ConnectorModel* clone(std::string name) const;

};

}

#endif
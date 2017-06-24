#ifndef LOOKBACK_EXCEPTIONS_H
#define LOOKBACK_EXCEPTIONS_H

#include <sstream>

#include "exceptions.h"

namespace nest
{

class InvalidSynapseReplacement : public KernelException
{
  size_t TargetGID_;
public:
  InvalidSynapseReplacement(const size_t &TargetGID)
      : KernelException( "InvalidSynapseReplacement" )
      , TargetGID_( TargetGID )
  {
  }

  ~InvalidSynapseReplacement() throw()
  {
  }
  std::string message();
};

}
#endif
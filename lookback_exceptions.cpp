#ifndef LOOKBACK_EXCEPTIONS_CPP
#define LOOKBACK_EXCEPTIONS_CPP

#include "lookback_exceptions.h"

namespace nest
{

std::string InvalidSynapseReplacement::message() {
  std::ostringstream msg;
  msg << "A pointer that does not exist in the neuron " << TargetGID_ << "Is being replaced.";
  return msg.str();
}

}

#endif
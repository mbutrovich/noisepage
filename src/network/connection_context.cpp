#include "network/connection_context.h"

#include "network/network_io_utils.h"
#include "network/network_io_wrapper.h"

namespace noisepage::network {

size_t ConnectionContext::GetNetworkWrapperBytesRead() const { return network_io_wrapper_->GetBytesRead(); }

size_t ConnectionContext::GetNetworkWrapperBytesWritten() const { return network_io_wrapper_->GetBytesWritten(); }

}  // namespace noisepage::network

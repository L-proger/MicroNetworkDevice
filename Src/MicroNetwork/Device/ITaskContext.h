#pragma once

#include <MicroNetwork.Common.h>


namespace MicroNetwork::Device {
    class ITaskContext;
}

namespace LFramework {
template<>
struct InterfaceAbi<MicroNetwork::Device::ITaskContext> : public InterfaceAbi<MicroNetwork::Common::IDataReceiver> {
    using Base = InterfaceAbi<IUnknown>;
	static constexpr InterfaceID ID() { return { 0xb2ea937e, 0xf025, 0x4218, { 0x8c, 0x27, 0xd9, 0x8a, 0x67, 0x8f, 0x34, 0xd6 } };  }
    virtual Result readPackets() = 0;
    virtual Result isExitRequested(bool& result) = 0;
private:
    ~InterfaceAbi() = delete;
};

template<class TImplementer>
struct InterfaceRemap<MicroNetwork::Device::ITaskContext, TImplementer> : public InterfaceRemap<MicroNetwork::Common::IDataReceiver, TImplementer> {
public:
    virtual Result readPackets() { return this->implementer()->readPackets(); }
    virtual Result isExitRequested(bool& result) { return this->implementer()->isExitRequested(result); }
};
}

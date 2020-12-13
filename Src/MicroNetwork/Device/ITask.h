#pragma once

#include <MicroNetwork/Device/ITaskContext.h>


namespace MicroNetwork::Device {
    class ITask;
}

namespace LFramework {
template<>
struct InterfaceAbi<MicroNetwork::Device::ITask> : public InterfaceAbi<MicroNetwork::Common::IDataReceiver> {
    using Base = InterfaceAbi<IUnknown>;
    static constexpr InterfaceID ID() { return { 0xf874922b, 0x1de0, 0x41f8, { 0x90, 0x91, 0x10, 0x66, 0x6, 0x70, 0x8c, 0xed } };  }
    virtual Result run(LFramework::ComPtr<MicroNetwork::Device::ITaskContext> context) = 0;
};

template<class TImplementer>
struct InterfaceRemap<MicroNetwork::Device::ITask, TImplementer> : public InterfaceRemap<IUnknown, TImplementer> {
public:
    virtual Result run(LFramework::ComPtr<MicroNetwork::Device::ITaskContext> context)  { return this->_implementer->run(context); }
};
}



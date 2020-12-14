#pragma once

#include <MicroNetwork/Device/ITask.h>
#include <MicroNetwork/Common/Packet.h>

namespace MicroNetwork::Device {

    class Task : public LFramework::ComImplement<Task, LFramework::ComObject, ITask> {
	public:
		virtual ~Task() = default;
		virtual LFramework::Result packet(MicroNetwork::Common::PacketHeader header, const void* data) = 0;
		virtual LFramework::Result run(LFramework::ComPtr<ITaskContext> context) = 0;
	};

}

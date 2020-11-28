#pragma once

#include <MicroNetwork/Device/ITaskContext.h>
#include <MicroNetwork/Common/IDataReceiver.h>

namespace MicroNetwork::Device {

    class Task : public Common::IDataReceiver {
	public:
		virtual ~Task() = default;
		virtual void run(ITaskContext* context) = 0;
	};

}

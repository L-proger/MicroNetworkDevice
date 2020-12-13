#pragma once

#include <MicroNetwork/Device/ITask.h>

namespace MicroNetwork::Device {

    class Task : public LFramework::ComImplement<Task, LFramework::ComObject, ITask> {
	public:
		virtual ~Task() = default;
		virtual LFramework::Result run(LFramework::ComPtr<ITaskContext> context) = 0;
	};

}

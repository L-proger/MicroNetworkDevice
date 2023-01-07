#pragma once

#include <LFramework/Guid.h>
#include <MicroNetwork.Device.h>

namespace MicroNetwork::Device {

	class TaskManager {
	public:
		virtual ~TaskManager() = default;
		virtual std::size_t getTasksCount() = 0;
		virtual bool getTaskId(std::size_t id, LFramework::Guid& result) = 0;
		virtual LFramework::ComPtr<ITask> createTask() = 0;
	};

}

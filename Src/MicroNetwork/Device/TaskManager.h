#pragma once

#include <MicroNetwork/Device/Task.h>
#include <LFramework/Guid.h>

namespace MicroNetwork::Device {

	class TaskManager {
	public:
		virtual ~TaskManager() = default;
		virtual std::size_t getTasksCount() = 0;
		virtual bool getTaskId(std::size_t id, LFramework::Guid& result) = 0;
		virtual Task* createTask() = 0;
		virtual void deleteTask(Task* task) = 0;
	};

}

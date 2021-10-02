#pragma once

#include <MicroNetwork.Device.h>
#include <MicroNetwork/Common/Packet.h>
#include <MicroNetwork/Common/DataStream.h>
#include <LFramework/Debug.h>
#include <LFramework/Assert.h>
#include <MicroNetwork/Device/TaskManager.h>
#include <LFramework/Containers/ByteFifo.h>
#include <LFramework/Threading/CriticalSection.h>
#include <LFramework/Threading/Semaphore.h>
#include <LFramework/Threading/Thread.h>

#include "TaskManager.h"
#include <atomic>

namespace MicroNetwork::Device {

class Node : public Common::DataStream {
public:
    class TaskContext : public LFramework::ComImplement<TaskContext, LFramework::ComObject, ITaskContext> {
	public:

        
		TaskContext(Node* node):_node(node){
            static_assert(LFramework::HasInterfaceWrapper<MicroNetwork::Device::ITask>::value, "NO WRAPPER");
		}
        bool receivePacketFromNetwork(const Common::PacketHeader& header, const void* data){
            LFramework::Threading::CriticalSection lock;
			if(_rxBuffer.sizeAvailable() < MicroNetwork::Common::packetFullSize(header)){
				return false;
			}
			_rxBuffer.write(&header, sizeof(header));
			_rxBuffer.write(data, header.size);
			return true;
		}

        LFramework::Result packet(Common::PacketHeader header, const void* data) {
			if(_node->writePacket(header, data)){
				return LFramework::Result::Ok;
			}
			return LFramework::Result::OutOfMemory;
		}

        LFramework::Result readPackets() {
			while(true){
                LFramework::Threading::CriticalSection lock;
				if(!readPacket()){
					return LFramework::Result::OutOfMemory;
				}else{
					_currentTask->packet(_packet.header, _packet.payload.data());
				}
			}
			return LFramework::Result::Ok;
		}

		void processTask(LFramework::ComPtr<ITask> task) {
			{
                LFramework::Threading::CriticalSection lock;
				_rxBuffer.clear();
				_exitRequested = false;
				_currentTask = task;
			}
			auto t = this->queryInterface<ITaskContext>();

			bool b;
			t->isExitRequested(b);

			_currentTask->run(t);
		}

		void requestExit() {
			{
                LFramework::Threading::CriticalSection lock;
				_exitRequested = true;
				/*auto task = _currentTask.load();
				if(task != nullptr){
					//task->
				}*/
			}
		}

		LFramework::Result isExitRequested(bool& result) {
			result = _exitRequested;
			return LFramework::Result::Ok;
		}
	private:
		bool readPacket() {
			if(!_rxBuffer.peek(&_packet.header, sizeof(_packet.header))){
				return false;
			}
			if(_rxBuffer.size() < MicroNetwork::Common::packetFullSize(_packet.header)) {
				return false;
			}
			_rxBuffer.read(&_packet, MicroNetwork::Common::packetFullSize(_packet.header));
			return true;
		}

		LFramework::ComPtr<ITask> _currentTask;
		bool _exitRequested = false;
		Node* _node;
        Common::MaxPacket _packet;
		ByteFifo<1024> _rxBuffer;
	};



	Node(TaskManager* taskManager) : _taskManager(taskManager){
		_taskContext = new TaskContext(this);
		_taskContext->addRef();
	}

    bool start() override {
        return true;
    }
    void process() {
        while(true){
        	//handle bind
            auto v = LFramework::Threading::CriticalSection::lock();
        	_taskTxEnabled = true;
        	if(_bindRequested){
        		lfDebug() << "Bind request detected";
        		_startRequested = false;
				_taskTxEnabled = true;

				//Send bind
                std::size_t tasksCount = _taskManager->getTasksCount();
                _txPacket.header.id = Common::PacketId::Bind;
                _txPacket.header.size = sizeof(tasksCount);
                memcpy(_txPacket.payload.data(), &tasksCount, sizeof(tasksCount));
        		_bindRequested = false;
                LFramework::Threading::CriticalSection::unlock(v);
        		writePacketBlocking();
        		lfDebug() << "Bind response sent";

        		//send tasks list
        		for(std::size_t i = 0; i < tasksCount; ++i){
        			LFramework::Guid taskId;
        			if(_taskManager->getTaskId(i, taskId)){
        				_txPacket.header.id = Common::PacketId::TaskDescription;
						_txPacket.header.size = sizeof(taskId);
						memcpy(_txPacket.payload.data(), &taskId, sizeof(taskId));
						writePacketBlocking();
						lfDebug() << "Task description sent";
        			}
        		}
        	}else{
                LFramework::Threading::CriticalSection::unlock(v);
        	}

        	if(_startRequested && !_bindRequested){
        		_startRequested = false;
        		processTask();
        	}else{
                LFramework::Threading::ThisThread::sleepForMs(1);
        	}

        }
    }

    bool readPacket(Common::MaxPacket& packet) {
    	LFramework::Threading::CriticalSection lock;
		if(_remote == nullptr){
			return false;
		}
		if(!_remote->peek(&packet.header, sizeof(packet.header))){
			return false;
		}

		auto packetFullSize = sizeof(packet.header) + packet.header.size;

		if(_remote->bytesAvailable() < packetFullSize){
			return false;
		}

		lfAssert(_remote->read(&packet, packetFullSize) == packetFullSize);
		return true;

    }

    bool writePacketBlocking() {
    	while(_taskTxEnabled){
    		if(writePacket(_txPacket.header, _txPacket.payload.data())){
    			return true;
    		}else{
    			LFramework::Threading::ThisThread::sleepForMs(1);
    		}
    	}
    	return false;
    }

    bool writePacket(Common::PacketHeader header, const void* data) {
    	auto totalSize = sizeof(header) + header.size;
    	LFramework::Threading::CriticalSection lock;

    	if(!_taskTxEnabled){
    		return false;
    	}

    	if(freeSpace() >= totalSize){
    		write(&header, sizeof(header));
    		write(data, header.size);
    		return true;
    	}
    	return false;
    }
protected:
    void processTask() {
    	//lfDebug() << "Node: Creating task";
    	auto task = _taskManager->createTask();
    	//lfDebug() << "Sending task start";
        _txPacket.header.id = Common::PacketId::TaskStart;
        _txPacket.header.size = 0;
    	writePacketBlocking();
    	//lfDebug() << "Node: Running task";
    	_taskContext->processTask(task);
    	//lfDebug() << "Node: Deleting task";
    	task = nullptr;
        _txPacket.header.id = Common::PacketId::TaskStop;
        _txPacket.header.size = 0;
    	writePacketBlocking();
    	//lfDebug() << "Node: TaskStop sent";
    }

    void onRemoteDisconnect() override {

    }

    void onRemoteDataAvailable() override {

    	while(true){
    		if(_remote->peek(&_rxPacket.header, sizeof(_rxPacket.header)) != sizeof(_rxPacket.header)){
    			break;
    		}

            const auto packetFullSize = MicroNetwork::Common::packetFullSize(_rxPacket.header);
            
    		if(_remote->bytesAvailable() < packetFullSize) {
    			break;
    		}

    		//lfDebug() << "Received packet!";
            if(_rxPacket.header.id == Common::PacketId::TaskStop){
				_remote->discard(packetFullSize);
				//lfDebug() << "Stop task requested";
				_taskContext->requestExit();

            }else if(_rxPacket.header.id == Common::PacketId::TaskStart){
            	_remote->peek(&_rxPacket, packetFullSize);
				_remote->discard(packetFullSize);
				_rxPacket.getData(_taskId);
				//lfDebug() << "Start task requested";
				_startRequested = true;
			}else{
				_remote->peek(&_rxPacket, packetFullSize);
				if(_taskContext->receivePacketFromNetwork(_rxPacket.header, _rxPacket.payload.data())){
					_remote->discard(packetFullSize);
				}
			}
    	}
    }

    void onReadBytes() override {

    }
    void onRemoteReset() override {
    	lfDebug() << "Reset node";
        LFramework::Threading::CriticalSection lock;
    	_taskContext->requestExit();
    	_taskTxEnabled = false;
    	_bindRequested = true;
    	clear();
    }
private:
    TaskContext* _taskContext = nullptr;
    Common::MaxPacket _rxPacket;
    Common::MaxPacket _txPacket;
    LFramework::Guid _taskId;
    TaskManager* _taskManager = nullptr;
    std::atomic<bool> _startRequested = false;
    std::atomic<bool> _bindRequested = false;
    std::atomic<bool> _taskTxEnabled = false;
};

}

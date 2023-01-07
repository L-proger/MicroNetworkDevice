#pragma once

#include <LFramework/USB/Device/UsbDBulkInterface.h>
#include <MicroNetwork/Common/DataStream.h>
#include <LFramework/Threading/CriticalSection.h>
#include <atomic>

namespace MicroNetwork::Device {

class UsbTransmitter : public Common::DataStream {
public:
	static constexpr std::size_t MaxPacketSize = 64;

    UsbTransmitter(LFramework::USB::UsbDBulkInterface* usbInterface) : _usbInterface(usbInterface) {
		_rxTransfer.buffer = _rxBuffer;
		_rxTransfer.size = MaxPacketSize;
		_rxTransfer.callback = &onReceivedPacket_;
		_rxTransfer.context = this;

		_txTransfer.buffer = _txBuffer;
		_txTransfer.callback = &onTramsmittedPacket_;
		_txTransfer.context = this;
	}

	bool start() override {
        _rxEndpoint = reinterpret_cast<LFramework::USB::UsbDEndpoint*>(_usbInterface->getEndpoint(false, 0));
        _txEndpoint = reinterpret_cast<LFramework::USB::UsbDEndpoint*>(_usbInterface->getEndpoint(true, 0));
		return (_rxEndpoint != nullptr) && (_txEndpoint != nullptr) && startRxTransfer();
	}

protected:
	void onRemoteDisconnect() override {

	}

	void onRemoteDataAvailable() override {
		LFramework::Threading::CriticalSection lock;
		continueUsbTx();
	}
	void onReadBytes() override {
		LFramework::Threading::CriticalSection lock;
		if(freeSpace() >= MaxPacketSize){
			startRxTransfer();
		}
	}
	void onRemoteReset() override {
		for(;;);
	}
private:

    void onReceivedPacket(LFramework::USB::UsbTransfer * transfer, bool isOk) {
		if(transfer->actualSize == 0) {
			reset();
			if(!_isTransmitting) {
				_synchronized = sendSyncPacket();
			} else {
				_synchronized = false;
			}
		} else {
			if(_synchronized) {
				if(write(reinterpret_cast<const uint8_t*>(transfer->buffer), transfer->actualSize) != transfer->actualSize){
					for(;;);
				}
			}
		}
		startRxTransfer();
	}

	bool sendSyncPacket() {
		_txTransfer.size = 0;
		_isTransmitting = _txEndpoint->transferAsync(&_txTransfer);
		return _isTransmitting;
	}

	void continueUsbTx() {
		LFramework::Threading::CriticalSection lock;
		if(_isTransmitting){
			return;
		}

		auto txSize = _remote->read(_txBuffer, sizeof(_txBuffer));
		if(txSize != 0) {
			_txTransfer.size = txSize;
			//lfDebug() << "USB begin tx: " << txSize;
			_isTransmitting = _txEndpoint->transferAsync(&_txTransfer);
			if(!_isTransmitting){
				lfDebug() << "Assert: failed to start TX transfer";
				for(;;);
			}
		}
	}


    void onTramsmittedPacket(LFramework::USB::UsbTransfer* transfer, bool isOk) {
		//lfDebug() << "USB tx done: " << transfer->size;
        LFramework::Threading::CriticalSection lock;
        _isTransmitting = false;
		if(!_synchronized) {
			_synchronized = sendSyncPacket();
		} else {
			continueUsbTx();
		}

	}
	bool startRxTransfer() {
		bool result = _rxEndpoint->transferAsync(&_rxTransfer);
		//lfDebug() << "startRxTransfer: " << result;
		return result;
	}


	static void onReceivedPacket_(LFramework::USB::UsbTransfer * transfer, bool isOk) {
		auto _this = reinterpret_cast<UsbTransmitter *>(transfer->context);
		_this->onReceivedPacket(transfer, isOk);
	}

	static void onTramsmittedPacket_(LFramework::USB::UsbTransfer * transfer, bool isOk) {
		auto _this = reinterpret_cast<UsbTransmitter *>(transfer->context);
		_this->onTramsmittedPacket(transfer, isOk);
	}

	bool _synchronized = false;
	bool _isTransmitting = false;

	uint8_t _rxBuffer[MaxPacketSize];
	uint8_t _txBuffer[MaxPacketSize];

    LFramework::USB::UsbTransfer _rxTransfer;
    LFramework::USB::UsbTransfer _txTransfer;

    LFramework::USB::UsbDEndpoint* _rxEndpoint = nullptr;
    LFramework::USB::UsbDEndpoint* _txEndpoint = nullptr;

    LFramework::USB::UsbDBulkInterface* _usbInterface = nullptr;
};

}


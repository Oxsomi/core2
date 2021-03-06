#include "system/system.hpp"
#include "system/log.hpp"
#include "system/viewport_manager.hpp"

namespace oic {

	System::System(LocalFileSystem *files_, Allocator *allocator_, ViewportManager *viewportManager_, Log *nativeLog):
		files_(files_), allocator_(allocator_), viewportManager_(viewportManager_), log_(nativeLog), nativeLog(nativeLog) {

		if (!system)
			system = this;
	}

	System::~System() {

		if (log_ != nativeLog)
			delete log_;

		if(system == this)
			system = nullptr;
		
	}

	void System::setCustomLogCallback(Log *log) {

		if (system->log_ != system->nativeLog)
			delete system->log_;

		system->log_ = log ? log : system->nativeLog;
	}

	void System::wait(ns time) {
		system->sleep(time);
	}


}
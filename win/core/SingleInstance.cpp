#include "SingleInstance.h"

#include <windows.h>


namespace core {

struct SingleInstance::Impl {
    HANDLE mutex = NULL;
    bool acquired = false;
};

SingleInstance::SingleInstance(const std::string& name) : impl(std::make_unique<Impl>()) {
    const std::string mutexName = "Local\\" + name;

    impl->mutex = CreateMutexA(NULL, TRUE, mutexName.c_str());
    if (impl->mutex == NULL) {
        return;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(impl->mutex);
        impl->mutex = NULL;
        return;
    }

    impl->acquired = true;
}

SingleInstance::~SingleInstance() {
    if (impl->mutex != NULL) {
        if (impl->acquired) {
            ReleaseMutex(impl->mutex);
        }
        CloseHandle(impl->mutex);
        impl->mutex = NULL;
    }
    impl->acquired = false;
}

bool SingleInstance::acquired() const {
    return impl->acquired;
}

}

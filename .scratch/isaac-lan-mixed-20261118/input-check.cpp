// 区分系统 DirectInput 初始化与游戏启动问题，分别记录 ANSI/Unicode 接口及 COM 初始化结果。
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <cstdio>

int main() {
    const auto module = GetModuleHandleW(nullptr);
    auto check = [module](const char* label, REFIID iid) {
        IUnknown* input = nullptr;
        const HRESULT status = DirectInput8Create(module, DIRECTINPUT_VERSION, iid,
                                                   reinterpret_cast<void**>(&input), nullptr);
        std::printf("%s result=0x%08lx object=%d\n", label, status, input != nullptr);
        if (input) input->Release();
    };
    check("default-A", IID_IDirectInput8A);
    check("default-W", IID_IDirectInput8W);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::printf("CoInitializeEx result=0x%08lx\n", com);
    check("COM-A", IID_IDirectInput8A);
    check("COM-W", IID_IDirectInput8W);
    if (SUCCEEDED(com)) CoUninitialize();
}

#include "world/binary/read_session.h"
#include <cstring>
#include <windows.h>

namespace cs::world {
namespace {
bool Readable(DWORD protection) noexcept {
  if(protection&PAGE_GUARD)return false;
  switch(protection&0xffu){
    case PAGE_READONLY: case PAGE_READWRITE: case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READ: case PAGE_EXECUTE_READWRITE: case PAGE_EXECUTE_WRITECOPY: return true;
    default: return false;
  }
}
}
int NativeReadExceptionFilter(unsigned code) noexcept {
  return code==EXCEPTION_ACCESS_VIOLATION || code==EXCEPTION_IN_PAGE_ERROR ||
         code==0x80000001u ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}
// 独立 SEH 叶只接收裸缓冲区，避免 MSVC 对带 C++ 析构的函数限制。
unsigned CopyNativeReadBytesFault(void *out,const void *in,std::size_t size) noexcept {
  __try {
    std::memcpy(out,in,size);
    return 0;
  } __except (NativeReadExceptionFilter(GetExceptionCode())) {
    return GetExceptionCode();
  }
}
bool CopyNativeReadBytes(void *out,const void *in,std::size_t size) noexcept {
  return CopyNativeReadBytesFault(out,in,size)==0;
}
bool QueryNativeReadRegion(std::uint64_t address,NativeReadRegion &out) noexcept {
  MEMORY_BASIC_INFORMATION info{};
  if(address>=kNativeAddressLimit ||
     !VirtualQuery(reinterpret_cast<void *>(std::uintptr_t(address)),&info,sizeof(info))||
     info.State!=MEM_COMMIT||!Readable(info.Protect))return false;
  const auto first=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
  const auto last=std::uint64_t(first)+info.RegionSize;
  if(address<first||address>=last||last>kNativeAddressLimit)return false;
  out={std::uint32_t(first),last};
  return true;
}
}
namespace cs::world::binary {
bool CaptureReadSession::Begin(bool (*current)(void *) noexcept,void *owner) noexcept {
  if(active_||serial_==UINT64_MAX)return Reject();
  ++serial_;
  revoked_=failed_=false;
  active_=true;
  current_=current;owner_=owner;
  return true;
}
void CaptureReadSession::Revoke() noexcept {
  failed_=revoked_=true;
  InvalidateFrameReadRegions(FrameRegionReset::Revoke);
}
// 外部写入使整个读段失效。局部复制失败只有在水位未变化时才能恢复。
bool CaptureReadSession::ExternalCurrent() noexcept {
  if(!active_||revoked_)return false;
  if(current_&&!current_(owner_)){RevokeForResourceChange();return false;}
  return true;
}
bool CaptureReadSession::ReviewRoot() noexcept {
  if(!active_||failed_)return false;
  if(!deferred_serial_)return true;
  ++counters_.root_resource_checks;
  if(deferred_serial_==serial_&&ExternalCurrent())return true;
  RevokeForResourceChange();
  ++counters_.root_resource_revocations;
  return false;
}
bool CaptureReadSession::ContinueAfterObjectFailure() noexcept {
  if(!ExternalCurrent())return false;
  failed_=false;
  return true;
}
bool CaptureReadSession::Probe(std::uint64_t address,std::span<std::byte> output) noexcept {
  if(!Good())return false;
  ++counters_.copies;
  if(!CheckedNativeCopy(address,output.data(),output.size(),FrameRegionReader::Capture,&counters_)||
     (!deferred_serial_&&!Good()))return false;
  counters_.bytes+=output.size();
  return true;
}
bool CaptureReadSession::Copy(std::uint64_t address,void *output,std::size_t size) noexcept {
  if(!Good())return false;
  if(!size)return true;
  if(!output||!address||address>=kNativeAddressLimit||size>kNativeAddressLimit-address){
    InvalidateFrameReadRegions(FrameRegionReset::Revoke);
    return Reject();
  }
  ++counters_.copies;
  if(!CheckedNativeCopy(address,output,size,FrameRegionReader::Capture,&counters_))return Reject();
  counters_.bytes+=size;
  return deferred_serial_||Good()||Reject();
}
bool CaptureReadSession::StringBlock(std::uint64_t address,std::span<std::byte> output,std::size_t &size) noexcept {
  size=0;
  if(!Good())return Reject();
  if(!address||address>=kNativeAddressLimit||output.empty()){
    InvalidateFrameReadRegions(FrameRegionReset::Revoke);
    return Reject();
  }
  ++counters_.string_blocks;
  ++counters_.copies;
  if(!CheckedNativeStringBlock(address,output,size,FrameRegionReader::Capture,&counters_))return Reject();
  counters_.bytes+=size;
  return deferred_serial_||Good()||Reject();
}
}

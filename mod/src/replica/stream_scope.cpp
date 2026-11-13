#include "replica/stream_model.h"

namespace cs::replica::stream {
// 将已核定的 Host 世界身份绑定到当前连接。
Scope Connected(const WorldScope &scope,std::uint64_t connection) {
  if(!connection||!scope.session||!scope.run||!scope.floor||!scope.dimension||!scope.room||!scope.scene)
    throw ContractError("stream_scope");
  return {scope.session,connection,scope.run,scope.floor,scope.dimension,scope.room,scope.scene};
}
}

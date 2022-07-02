#ifndef CHARON_SERVER_CHARON_H
#define CHARON_SERVER_CHARON_H

#include <google/protobuf/service.h>
#include "charon/pb/charon.pb.h"
#include "charon/pb/raft.pb.h"

namespace charon {

class Charon : public CharonService {

 public:
  Charon() = default;

  ~Charon() = default;

  void DiscoverServer(::google::protobuf::RpcController* controller,
                       const ::DiscoverRequest* request,
                       ::DiscoverResponse* response,
                       ::google::protobuf::Closure* done);
  
  void RegisterServer(::google::protobuf::RpcController* controller,
                       const ::RegisterRequest* request,
                       ::RegisterResponse* response,
                       ::google::protobuf::Closure* done);
};


class Raft: public RaftService {

 public:
  Raft() = default;

  ~Raft() = default;

  void AskVote(::google::protobuf::RpcController* controller,
                       const ::AskVoteRequest* request,
                       ::AskVoteResponse* response,
                       ::google::protobuf::Closure* done);
  
  void AppendLogEntries(::google::protobuf::RpcController* controller,
                       const ::AppendLogEntriesRequest* request,
                       ::AppendLogEntriesResponse* response,
                       ::google::protobuf::Closure* done);
};


}


#endif
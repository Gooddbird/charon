#include "charon/pb/raft.pb.h"
#include "charon/raft/raft_node.h"
#include "charon/server/interface/ask_vote.h"
#include "tinyrpc/comm/log.h"

namespace charon {

AskVoteImpl::AskVoteImpl(const AskVoteRequest* request, AskVoteResponse* response)
  : m_request(request), m_response(response) {
    AppDebugLog << "AsvVote get request[" << request->ShortDebugString() << "]";
}

AskVoteImpl::~AskVoteImpl() {

}

void AskVoteImpl::run() {

  checkInput();

  execute();
}

void AskVoteImpl::checkInput() {


}

void AskVoteImpl::execute() {

  RaftNode::GetRaftNode()->handleAskVote(*m_request, *m_response);
}


}
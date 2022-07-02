#include "charon/pb/raft.pb.h"
#include "charon/server/interface/ask_vote.h"

namespace charon {

AskVoteImpl::AskVoteImpl(const AskVoteRequest* request, AskVoteResponse* response) {


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


}


}
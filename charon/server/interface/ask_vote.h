#ifndef CHARON_SERVER_INTERFACE_ASK_VOTE_H
#define CHARON_SERVER_INTERFACE_ASK_VOTE_H 

#include "charon/pb/raft.pb.h"

namespace charon {

class AskVoteImpl {
 public:
  AskVoteImpl(const AskVoteRequest* request, AskVoteResponse* response);

  ~AskVoteImpl();

  void run();

  void checkInput();

  void execute();

 private:
  const AskVoteRequest* m_request {NULL};
  AskVoteResponse* m_response {NULL};

};


}


#endif
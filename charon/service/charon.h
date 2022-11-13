/*************************************************************
 * 
 *  #####   ###   #     #    #		 #     #####    #####     ####
 *    #			 #    #	#	  #			#		#			 #  #	  	#		#		 #
 *    #			 #    #		# #				#				 ###			#####    #		
 *    #			###   #		  #				#				 #   #		#					####
 *
 * charon.h
 * Generated by tinyrpc framework tinyrpc_generator.py
 * Create Time: 2022-11-13 17:39:07
 * This file will be overwrite every time
*************************************************************/


#ifndef CHARON_SERVICE_CHARON_H 
#define CHARON_SERVICE_CHARON_H

#include <google/protobuf/service.h>
#include "charon/pb/charon.pb.h"

namespace charon {

class CharonServiceImpl : public CharonService {

 public:

  CharonServiceImpl() = default;

  ~CharonServiceImpl() = default;

  // override from CharonService
  virtual void DiscoverServer(::google::protobuf::RpcController* controller,
                       const ::DiscoverRequest* request,
                       ::DiscoverResponse* response,
                       ::google::protobuf::Closure* done);

  // override from CharonService
  virtual void RegisterServer(::google::protobuf::RpcController* controller,
                       const ::RegisterRequest* request,
                       ::RegisterResponse* response,
                       ::google::protobuf::Closure* done);

  // override from CharonService
  virtual void AskVote(::google::protobuf::RpcController* controller,
                       const ::AskVoteRequest* request,
                       ::AskVoteResponse* response,
                       ::google::protobuf::Closure* done);

  // override from CharonService
  virtual void AppendLogEntries(::google::protobuf::RpcController* controller,
                       const ::AppendLogEntriesRequest* request,
                       ::AppendLogEntriesResponse* response,
                       ::google::protobuf::Closure* done);

  

};

}


#endif
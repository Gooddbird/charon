/*************************************************************
 * 
 *  #####   ###   #     #    #		 #     #####    #####     ####
 *    #			 #    #	#	  #			#		#			 #  #	  	#		#		 #
 *    #			 #    #		# #				#				 ###			#####    #		
 *    #			###   #		  #				#				 #   #		#					####
 *
 * register_server.h
 * Generated by tinyrpc framework tinyrpc_generator.py
 * Create Time: 2022-11-13 14:00:23
 * This file will not be overwrite althrough protobuf file changed !!!
 * Just write this file while not exist
*************************************************************/

#ifndef CHARON_INTERFACE_REGISTER_SERVER_H
#define CHARON_INTERFACE_REGISTER_SERVER_H 

#include "charon/pb/charon.pb.h"


namespace charon {

/*
 * Rpc Interface Class
 * Alloc one object every time RPC call begin, and destroy this object while RPC call end
*/

class RegisterServerInterface {
 public:

  RegisterServerInterface(const ::RegisterRequest& request, ::RegisterResponse& response);

  ~RegisterServerInterface();

  void run();

 private:
  const ::RegisterRequest& m_request;      // request object fron client

  ::RegisterResponse& m_response;           // response object that reply to client

};


}


#endif
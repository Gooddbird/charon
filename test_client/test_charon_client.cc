#include <google/protobuf/service.h>
#include <iostream>
#include "tinyrpc/net/tinypb/tinypb_rpc_channel.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_async_channel.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_controller.h"
#include "tinyrpc/net/tinypb/tinypb_rpc_closure.h"
#include "tinyrpc/net/net_address.h"
#include "tinyrpc/comm/string_util.h"
#include "charon/pb/charon.pb.h"


tinyrpc::IPAddress::ptr g_addr = nullptr;

void printfLine() {
  int i = 10;
  std::string s = "**********";
  while(i--) {
    s += "**********";
  }
  std::cout << s << std::endl;
}

void printInput() {
  std::cout << "<<< " << std::flush;
}

void printOutput(const std::string& info) {
  std::cout << info << "\n" <<  std::endl;
}

void printError(const std::string& reason) {
  printOutput("Error! " + reason);
}

void printSucc(const std::string& info) {
  printOutput("Succ! " + info);
}

RAFT_SERVER_LSTATE stringToLstate(const std::string& state_res) {

  std::string state = state_res;
  std::transform(state.begin(), state.end(), state.begin(), toupper);

  if (state == "ACTIVE") {
    return EN_RAFT_LSTATE_ACTIVE;
  } else if (state == "DELETED") {
    return EN_RAFT_LSTATE_DELETED;
  } else {
    return EN_RAFT_LSTATE_UNDEFINE;
  }
}

std::string lstateToString(RAFT_SERVER_LSTATE state) {
  if (state == EN_RAFT_LSTATE_ACTIVE) {
    return "Active";
  } else if (state == EN_RAFT_LSTATE_DELETED) {
    return "Deleted";
  } else {
    return "Undefine error state";
  }
}

std::string serverNodeToString(const ServerNode& node) {
  std::stringstream ss;
  ss << "{\"node_id\": " << node.id()
    << ", \"node_name\": \"" << node.name() << "\""
    << ", \"node_addr\": \"" << node.addr() << "\"" 
    << ", \"node_state\": \"" << lstateToString(node.lstate()) << "\"" 
    << "}";

  return ss.str();
}



std::pair<int, std::string> testOperateRaftServerNode(const OperateRaftServerNodeRequest& requeset, OperateRaftServerNodeResponse& response) {

  tinyrpc::TinyPbRpcChannel channel(g_addr);
  CharonService_Stub stub(&channel);

  tinyrpc::TinyPbRpcController rpc_controller;
  rpc_controller.SetTimeout(5000);

  // std::cout << "Send to tinyrpc server " << addr->toString() << ", requeset body: " << requeset.ShortDebugString() << std::endl;
  stub.OperateRaftServerNode(&rpc_controller, &requeset, &response, NULL);

  if (rpc_controller.ErrorCode() != 0) {
    // std::cout << "Failed to call tinyrpc server, error code: " << rpc_controller.ErrorCode() << ", error info: " << rpc_controller.ErrorText() << std::endl; 
    return std::make_pair(rpc_controller.ErrorCode(), rpc_controller.ErrorText());
  }

  // std::cout << "Success get response frrom tinyrpc server " << addr->toString() << ", response body: " << response.ShortDebugString() << std::endl;

  return std::make_pair(0, "OK");
}


std::pair<int, std::string> testQueryAllRaftServerNode(const QueryAllRaftServerNodeRequest& request, QueryAllRaftServerNodeResponse& response) {

  tinyrpc::TinyPbRpcChannel channel(g_addr);
  CharonService_Stub stub(&channel);

  tinyrpc::TinyPbRpcController rpc_controller;
  rpc_controller.SetTimeout(5000);

  // std::cout << "Send to tinyrpc server " << addr->toString() << ", requeset body: " << rpc_req.ShortDebugString() << std::endl;
  stub.QueryAllRaftServerNode(&rpc_controller, &request, &response, NULL);

  if (rpc_controller.ErrorCode() != 0) {
    // std::cout << "Failed to call tinyrpc server, error code: " << rpc_controller.ErrorCode() << ", error info: " << rpc_controller.ErrorText() << std::endl; 
    return std::make_pair(rpc_controller.ErrorCode(), rpc_controller.ErrorText());
  }

  // std::cout << "Success get response frrom tinyrpc server " << addr->toString() << ", response body: " << rpc_res.ShortDebugString() << std::endl;
  return std::make_pair(0, "OK");
}




void dealAddRaftServerNode(std::vector<std::string>& vec) {
  if (g_addr == nullptr) {
    printError("AddRaftServerNode error, please Connect a server node at first");
    return;
  }
  if (vec.size() < 3 || vec.size() > 4) {
    printError("AddRaftServerNode args error, usage: AddRaftServerNode name addr [lstate]");
    return;
  }

  OperateRaftServerNodeRequest requeset;
  OperateRaftServerNodeResponse response;
  ServerNode* node = requeset.mutable_node();
  node->set_name(vec[1]);
  node->set_addr(vec[2]);
  if (vec.size() == 4) {
    RAFT_SERVER_LSTATE state = stringToLstate(vec[2]);
    if (state == EN_RAFT_LSTATE_UNDEFINE) {
      printError("AddRaftServerNode args error, usage: AddRaftServerNode name addr [lstate]");
      return;
    }
    node->set_lstate(state);
  } else {
    node->set_lstate(EN_RAFT_LSTATE_ACTIVE);
  }
  requeset.set_option(EN_RAFT_SERVER_OPERATION_ADD);

  auto res = testOperateRaftServerNode(requeset, response);
  if (res.first != 0) {
    printError("AddRaftServerNode error, framework exception, errinfo: [" + std::to_string(res.first) + ", " + res.second + "]");
    return;
  }
  if (response.ret_code() != 0) {
    printError("AddRaftServerNode error, business exception, errinfo: [" + std::to_string(response.ret_code()) + ", " + response.res_info()+ "]");
    return;
  }
  printSucc("AddRaftServerNode succ, node info: " + serverNodeToString(response.node()));

}

void dealUpdateRaftServerNode(std::vector<std::string>& vec) {

}


void dealQueryAllRaftServerNode(std::vector<std::string>& vec) {
  if (g_addr == nullptr) {
    printError("QueryAllRaftServerNode error, please Connect a server node at first");
    return;
  }
  if (vec.size() > 1) {
    printError("QueryAllRaftServerNode args error, usage: QueryAllRaftServerNode");
    return;
  }

  QueryAllRaftServerNodeRequest requeset;
  QueryAllRaftServerNodeResponse response;
  requeset.set_client_ip("127.0.0.1");

  auto res = testQueryAllRaftServerNode(requeset, response);
  if (res.first != 0) {
    printError("QueryAllRaftServerNode error, framework exception, errinfo: [" + std::to_string(res.first) + ", " + res.second + "]");
    return;
  }
  if (response.ret_code() != 0) {
    printError("QueryAllRaftServerNode error, business exception, errinfo: [" + std::to_string(response.ret_code()) + ", " + response.res_info()+ "]");
    return;
  }
  int node_count = response.node_list_size();
  std::stringstream ss;
  ss << "node_count: " << node_count; 
  for (int i = 0; i < node_count; ++i) {
    ss << "\nNo." << (i+1) << " " << serverNodeToString(response.node_list().at(i));
  }

  printSucc("QueryAllRaftServerNode succ, " + ss.str());

}

void dealConnect(std::vector<std::string>& vec) {
  if (vec.size() != 2) {
    printError("Connect args error, usage: Connect x.x.x.x:yy");
    return;
  }
  if(!tinyrpc::IPAddress::CheckValidIPAddr(vec[1])) {
    printError("Invalid network addr " + vec[1]);
  }

  g_addr = std::make_shared<tinyrpc::IPAddress>(vec[1]);

  printSucc("Success Connect addr " + g_addr->toString());

}

void parseCMD(const std::string& cmd) {
  std::vector<std::string> vec;
  tinyrpc::StringUtil::SplitStrToVector(cmd, " ", vec);
  if (vec.empty()) {
    return;
  }

  std::string old_option = vec[0]; 
  std::string option = old_option;
  std::transform(old_option.begin(), old_option.end(), option.begin(), toupper);

  if (option == "ADDRAFTSERVERNODE") {
    dealAddRaftServerNode(vec);
  } else if (option == "UPDATERAFTSERVERNODE") {

  } else if (option == "DELETETERAFTSERVERNODE") {

  } else if (option == "QUERYRAFTSERVERNODE") {

  } else if (option == "QUERYALLRAFTSERVERNODE") {
    dealQueryAllRaftServerNode(vec);
  } else if (option == "CONNECT") {
    dealConnect(vec);
  } else if (option == "EXIT") {
    printOutput("Now to exit charon client!");
    printfLine();
    exit(0);
  } else {
    printError("Unknown option: " + old_option);
    // for (int i = 0; i < old_option.length(); ++i) {
    //   printf("%d ", (int)(old_option[i]));
    // }
    return;
  }
  
}




int main(int argc, char* argv[]) {

  printfLine();
  std::cout << "Welcome to charon!\n";
  while(1) {
    printInput();
    std::string cmd;
    char c = 0;
    while(true) {
      c = std::cin.get();
      if (c == 10) {
        break;
      }
      cmd += c;
    }
    // std::cin>>cmd;
    
    parseCMD(cmd);
  }

  return 0;
}
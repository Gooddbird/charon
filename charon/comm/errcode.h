#ifndef CHARON_COMM_ERRCODE_H
#define CHARON_COMM_ERRCODE_H

namespace charon {

const int ERROR_KEY_EMPTY = 100;
const int ERROR_SERVER_ADDR_EMPTY = 101;
const int ERROR_SERVER_ADDR_INVALID = 102;
const int ERROR_REGISTER_SERVER_ADDR_EMPTY = 103;
const int ERROR_REGISTER_SERVER_TAG_INVALID = 104;

const int ERROR_INVALID_THREAD_HASH = 105;


const int ERR_PARAM_INPUT = 70000001;               // invalid param input 
const int ERR_EMPTY_RAFTNODES = 70000002;           // not exist raft server nodes
const int ERR_RPC_EXCEPTION = 70000003;             // call raft server node rpc exception


// raft error
const int ERR_TERM_MORE_THAN_LEADER = 80000001;        // current term more than leader' term
const int ERR_NOT_MATCH_PREINDEX = 80000002;           // can't find a log match prev_log_index & prev_log_term
const int ERR_LOG_MORE_THAN_CANDIDATE = 80000003;      // current log is more new than candidate
const int ERR_TERM_MORE_THAN_CANDICATE = 80000004;     // current term more than leader' term
const int ERR_ALREADY_VOTED = 80000005;               // current term node has already voted 



}


#endif
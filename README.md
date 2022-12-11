# charon
charon 是一款使用 TinyRPC 框架开发的一款分布式 K-V 数据库，可以用作分布式服务的服务注册、服务发现中心。


# TODO
1. MGSNO 无法透传给下游的问题？ 解决
2. sleep 后无法唤醒? 解决


# TODO: 2022-12-12
1. TimerEvent 的 resetTime 不起作用？看代码是定时器那块实现有点问题
2. charon2 在执行 raft 算法时候 coredump 了

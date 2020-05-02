A service oriented broker that implements the Majordomo protocol
----------------------------------------------------------------

This broker connects a set of clients to a set of workers who register particular "services". 
Client requests are then sent to workers according to their availability, and replies sent
back to the original clients.

This implementation was developed using zproject and zproto projects.

The protocol, Majordomo Protocol 0.2, is specified in
[18/MDP](http://rfc.zeromq.org/spec:18)

For the older implementation, based on the Guide, see
[majordomo/libmdp](libmdp).

Reference implementations of older wire protocols:

* [MDP/0.1](http://rfc.zeromq.org/spec:7), see the git tag
  [wire-protocol-0.1](https://github.com/zeromq/majordomo/tree/wire-protocol-0.1).
* [MDP/0.2](http://rfc.zeromq.org/spec:18), see the git tag
  [wire-protocol-0.2](https://github.com/zeromq/majordomo/tree/wire-protocol-0.2).


Architecture
------------

* Definitions of messages and codecs are in XML files in the directory src
* Due to the peculiar nature of the Majordomo protocol, there is one type of server,
  know as broker, and two types of clients: client and worker. mdp_broker.xml defines
  the broker's state machine; mdp_client.xml and mdp_worker.xml define the client's
  and worker's state machine respectively.
* project.xml was adapted from zproject. generate.sh was copied directly from zproject.
  It generates autogen.sh (which is used in the first step of configuring the project).
 

Contribution process
--------------------

* C4 process is at http://rfc.zeromq.org/spec:16.
* All commits are be backed by issues.
* All commits are made as pull requests from forked work repository.


Building and installing
-----------------------

* ./autogen.sh
* ./configure
* make
* make install

See also
--------

* [GSL, iMatix code generator](https://github.com/imatix/gsl) - tool for generating code
  from XML-based models
* [zeromq/zproto](https://github.com/zeromq/zproto) - A protocol framework for ZeroMQ
* [zeromq/zproject](https://github.com/zeromq/zproject) - CLASS Project Generator
* [zeromq/czmq](https://github.com/zeromq/czmq) - High-level C binding for ZeroMQ

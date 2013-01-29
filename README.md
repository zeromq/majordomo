A Free-as-in-movement

State of the art implementation of the Majordomo protocol. The current
implementation on the `master` branch is work in progress.

Reference implementations of older wire protocols:

* [MDP/0.1](http://rfc.zeromq.org/spec:7), see the git tag
  [wire-protocol-0.1](https://github.com/zeromq/majordomo/tree/wire-protocol-0.1).
* [MDP/0.2](http://rfc.zeromq.org/spec:18), see the git tag
  [wire-protocol-0.2](https://github.com/zeromq/majordomo/tree/wire-protocol-0.2).

Goals:

* Forked from Guide examples, to live alone and die free.
* Multiple language bindings (be the first to write one!).

Architecture:

* Broker and core C library is in /libmdp directory.
* Language bindings are under /bindings directory.

Contribution process:

* C4 process is at http://rfc.zeromq.org/spec:16.
* All commits are be backed by issues.
* All commits are made as pull requests from forked work repository.

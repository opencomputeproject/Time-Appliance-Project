## Linearizable Clock Test

This is a vanilla program that tests the true clocks. It is by no means mature, yet. 
It only depends on the C++11 features and no extra packages are needed(e.g. thrift, etc.)
The OSS socket library from https://cs.baylor.edu/~donahoo/practical/CSockets/practical/ are copied and modified for our needs.

Note: the original socket library does not support IPv6. Currently, efforts are only made to support IPv6 for UDP sockets.


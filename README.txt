=================================================
  CSCI 4273
  Network Systems
  Programming Assignment 4
  Peer-to-Peer File Server
  Written by Ben Cavins (cavinsb@colorado.edu)
=================================================

=================================================
  Files
=================================================

client_PFS.c
server_PFS.c
protocol.h
protocol.c
list.h
list.c
Makefile
README.txt

=================================================
  Examples
=================================================

--- Build ---
make

--- Run ---
./server_PFS <port number>
./client_PFS <client name> <server ip> <server port>

=================================================
  General Info
=================================================

This is an implementation of a simple peer-to-peer file sharing service.
Clients register with a central server which keeps records of registered
clients and a master list of all files in the network. Clients use this master
list to connect with and request files from each other. 

=================================================
  Known Issues
=================================================

  - Server only removes client if the client sends a remove command. The 
    server should not trust the client to do this.
  - Clients can unregister any other clients. This is less than ideal.
  - Clients occationally hang when exiting.
  - Master file list only updates when a client registers with the server.
  - Master file list never removes or updates any of its entries.

=================================================
  Borrowed Code
=================================================

list.c and list.h were taken from https://github.com/zhemao/libds
These are distributed under the MIT license, reproduced below:

Copyright (c) 2011 Zhehao Mao

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

lock_client::lock_client(std::string dst)
{
	sockaddr_in dstsock;
	make_sockaddr(dst.c_str(), &dstsock);
	//	cl = new rpcc(dstsock);

	cl = new rsm_client(dst);

	srand(666);
	id = rand();

//	if (rcl->bind() < 0) {
//		printf("lock_client: call bind\n");
//	}
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
	int r;
	cl->call(lock_protocol::stat, id, lid, r);
	//assert (ret == lock_protocol::OK);
	return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
	int r;
	while(cl->call(lock_protocol::acquire, id, lid, r) == lock_protocol::RETRY)
		usleep(50000);

	//assert (ret == lock_protocol::OK);
	return r;
}


lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
	int r;

	cl->call(lock_protocol::release, id, lid, r);
	//assert (ret == lock_protocol::OK);
	return r;
}


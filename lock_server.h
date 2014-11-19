// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <pthread.h>
#include <map>
#include "rsm.h"
#include "rsm_state_transfer.h"

class lock_server: public rsm_state_transfer {

private:
	class rsm *rsm;

protected:
	int nacquire;
	enum lock_state {
		FREE, LOCKED
	};

	typedef int l_state;

	std::map<lock_protocol::lockid_t, l_state> l_state_map;
	std::map<lock_protocol::lockid_t, l_state>::iterator iter;

public:
	lock_server(class rsm *rsm=0);
	~lock_server();

	lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
	lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);	
	lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);

	std::string marshal_state();
	void unmarshal_state(std::string state);
};

#endif 








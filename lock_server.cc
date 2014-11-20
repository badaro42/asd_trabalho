// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <rpc/slock.h>
#include <slock.h>

pthread_mutex_t global_lock;
pthread_cond_t map_condition;

lock_server::lock_server(class rsm *_rsm): rsm(_rsm), nacquire (0)
{
	pthread_mutex_init(&global_lock, NULL);
	pthread_cond_init(&map_condition, NULL);
	rsm->set_state_transfer(this);
}

lock_server::~lock_server()
{
	pthread_mutex_destroy(&global_lock);
	pthread_cond_destroy(&map_condition);
}


lock_protocol::status lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
	lock_protocol::status ret = lock_protocol::OK;
	printf("stat request from clt %d\n", clt);

	r = nacquire;
	return ret;
}


lock_protocol::status lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
	lock_protocol::status ret = lock_protocol::OK;

	ScopedLock sl (&global_lock);
	lock_state stat = FREE;

	//o elemento ja existe. se estiver ocupado, fica na fila de espera
	if(l_state_map.count(lid) > 0) {	

		//		while(!(l_state_map[lid] == stat))
		//			pthread_cond_wait(&map_condition, &global_lock);
		if(l_state_map[lid] != stat)
			return lock_protocol::RETRY;
	}

	//o elemento nao existe
	lock_state status = LOCKED;
	l_state_map[lid] = status;

	return ret;
}


lock_protocol::status lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{	
	lock_protocol::status ret = lock_protocol::OK;

	ScopedLock sl (&global_lock);

	//o id do lock é valido e este esta LOCKED
	if((l_state_map.count(lid) > 0) && (l_state_map[lid] == LOCKED)) {

		l_state_map[lid] = FREE;
		//		pthread_cond_broadcast(&map_condition);

	}
	//o id do lock nao existe, retornamos erro que diz que a entidade nao existe
	else if(l_state_map.count(lid) == 0) {
		ret = lock_protocol::NOENT;
	}

	return ret;
}

//FEITO lab6 - falta testar
std::string
lock_server::marshal_state() {

	// lock any needed mutexes
	pthread_mutex_lock(&global_lock);

	marshall rep;
	rep << l_state_map.size();
//	std::map<std::string, l_state>::iterator iter_lock;

	for (iter = l_state_map.begin(); iter != l_state_map.end(); iter++) {
		lock_protocol::lockid_t lid = iter->first;
		l_state stat;
		stat = l_state_map[lid];
		rep << lid;
		rep << stat;
	}
	// unlock any mutexes
	pthread_mutex_unlock(&global_lock);

	return rep.str();
}

//FEITO lab6 - falta testar
void
lock_server::unmarshal_state(std::string state) {

	// lock any needed mutexes
	pthread_mutex_lock(&global_lock);

	unmarshall rep(state);
	unsigned int locks_size;
	rep >> locks_size;
	for (unsigned int i = 0; i < locks_size; i++) {
		lock_protocol::lockid_t lid;
		rep >> lid;
		l_state stat;
		rep >> stat;
		l_state_map[lid] = stat;
	}
	// unlock any mutexes
	pthread_mutex_unlock(&global_lock);
}

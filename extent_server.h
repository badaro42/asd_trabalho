// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"

class extent_server {

public:
	extent_server();
	~extent_server();

protected:
	struct extent_cell {
		extent_protocol::attr extent_attr;
		std::string data;
	};

	std::map<extent_protocol::extentid_t, extent_cell *> ext_storage;
	pthread_mutex_t storage_mutex;

public:
	int put(extent_protocol::extentid_t id, std::string, int &);
	int get(extent_protocol::extentid_t id, std::string &);
	int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
	int remove(extent_protocol::extentid_t id, int &);
};

#endif 








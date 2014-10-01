// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
	ec = new extent_client(extent_dst);

}

yfs_client::inum
yfs_client::n2i(std::string n)
{
	std::istringstream ist(n);
	yfs_client::inum finum;
	ist >> finum;
	return finum;
}

std::string
yfs_client::filename(inum inum)
{
	std::ostringstream ost;
	ost << inum;
	return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
	if(inum & 0x80000000)
		return true;
	return false;
}

bool
yfs_client::isdir(inum inum)
{
	return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
	int r = OK;

	printf("getfile %016llx\n", inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}

	fin.atime = a.atime;
	fin.mtime = a.mtime;
	fin.ctime = a.ctime;
	fin.size = a.size;
	printf("getfile %016llx -> sz %llu\n", inum, fin.size);

	release:
	return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
	int r = OK;

	printf("getdir %016llx\n", inum);
	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	din.atime = a.atime;
	din.mtime = a.mtime;
	din.ctime = a.ctime;

	release:
	return r;
}

int
yfs_client::put(inum inum, std::string buf) {
	extent_protocol::status r;
	printf("put %016llx\n", inum);
	r = ec->put(inum, buf);

	if (r != extent_protocol::OK)
		return IOERR;

	else
		return OK;
}

int
yfs_client::get(inum inum, std::string& buf) {
	extent_protocol::status r;
	printf("get %016llx\n", inum);
	r = ec->get(inum, buf);

	if (r != extent_protocol::OK)
		return IOERR;

	else
		return OK;
}

int
yfs_client::remove(inum inum) {
	extent_protocol::status r;
	printf("remove %016llx\n", inum);
	r = ec->remove(inum);

	if (r != extent_protocol::OK)
		return IOERR;

	else
		return OK;
}

//int
//yfs_client::setattr(inum inum, fileinfo &fin)
//{
//	//	int ret = OK;
//	//	std::string buffer;
//
//	//if(ec->put(inum, buffer))
//}

//se o inum passado como parametro for um ficheiro, retorna logo zero
yfs_client::inum
yfs_client::ilookup(inum di, std::string name)
{
	if(isfile(di))
		return 0;

	std::string buffer;
	get(di, buffer);

	std::vector<std::string> entries = yfs_client::split(buffer,"\n",true,false);
	for(unsigned int i = 0; i < entries.size(); i++){
		std::string entry = entries[i];
		std::vector<std::string> info = yfs_client::split(entry," ", true,false);
		if(info.size() != 2)return 0;
		inum temp_inum = n2i(info[0]);
		std::string temp_name = info[1];
		if(isfile(temp_inum)){
			if(temp_name == name) return temp_inum;
			else continue;
		}
	}
	return 0;

	//TODO completar esta merda

}

//TODO alterar isto depois quando tudo estiver feito
std::vector<std::string> yfs_client::split(const std::string& s, const std::string& match,
		bool removeEmpty,bool fullMatch){
	std::vector<std::string> result;
	std::string::size_type start = 0, skip = 1;
	find_t pfind = &std::string::find_first_of;

	if (fullMatch){
		skip = match.length();
		pfind = &std::string::find;
	}

	while (start != std::string::npos){
		std::string::size_type end = (s.*pfind)(match, start);
		if (skip == 0) end = std::string::npos;

		std::string token = s.substr(start, end - start);

		if (!(removeEmpty && token.empty())){
			result.push_back(token);
		}
		if ((start = end) != std::string::npos) start += skip;
	}

	return result;
}



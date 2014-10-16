// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
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
	lc = new lock_client(lock_dst);
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

	//printf("GETFILE - AINDA NAO TENHO LOCK!\n");
	//acquire_lock(inum);
	//printf("GETFILE - JÁ TENHO LOCK!\n");

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
	//release_lock(inum);
	return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
	int r = OK;
	printf("getdir %016llx\n", inum);

	//printf("GETDIR - AINDA NAO TENHO LOCK!\n");
	//acquire_lock(inum);
	//printf("GETDIR - JÁ TENHO LOCK!\n");

	extent_protocol::attr a;
	if (ec->getattr(inum, a) != extent_protocol::OK) {
		r = IOERR;
		goto release;
	}
	din.atime = a.atime;
	din.mtime = a.mtime;
	din.ctime = a.ctime;

	release:
	//release_lock(inum);
	return r;
}

//SEM LOCKS
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

//metodo auxiliar que recebe o caracter onde queremos partir a string, parte a mesma e devolve-a num vector
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

//metodo principal que trata de chamar o auxiliar para partir a string
std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}


//se o inum passado como parametro for um ficheiro, retorna logo zero
yfs_client::inum
yfs_client::ilookup(inum di, std::string name)
{
	//verificamos se o inum do parametro é ou nao uma directoria
	if(isfile(di))
		return 0;

	std::string buffer;
	get(di, buffer); //obtem o conteudo da directoria e escreve no buffer

	//cada posiçao deste vector contém uma entrada com a info dum ficheiro
	std::vector<std::string> entries = split(buffer, '\n');
	for(unsigned int i = 0; i < entries.size(); i++){

		std::string entry = entries[i];
		std::vector<std::string> info = split(entry, ' ');

		//cada entrada tem que ter duas posiçoes: inum e nome
		if(info.size() != 2)
			return 0;

		inum temp_inum = n2i(info[0]);
		std::string temp_name = info[1];

		//vamos ver se o ficheiro existe
		if(temp_name == name) //se existir verificamos se o nome do ficheiro é igual ao do parametro
			return temp_inum;
		else continue;
	}
	return 0;
}

//liberta o lock do extent ext_id
void
yfs_client::release_lock(inum ext_id)
{
	printf("RELEASE_LOCK - estou a libertar o lock do extent %016llx\n", ext_id);
	lc->release(ext_id);
}

//adquire o lock do extent ext_id
void
yfs_client::acquire_lock(inum ext_id)
{
	printf("ACQUIRE_LOCK - estou a pedir o lock do extent %016llx\n", ext_id);
	lc->acquire(ext_id);
}

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
	//verificamos se o inum do parametro é ou nao uma directoria
	if(isfile(di))
		return 0;

	std::string buffer;
	get(di, buffer); //obtem o conteudo da directoria e escreve no buffer

	//cada posiçao deste vector contém uma entrada com a info dum ficheiro
	std::vector<std::string> entries = yfs_client::split(buffer,"\n");
	for(unsigned int i = 0; i < entries.size(); i++){

		std::string entry = entries[i];
		std::vector<std::string> info = yfs_client::split(entry, " ");

		//cada entrada tem que ter duas posiçoes: inum e nome
		if(info.size() != 2)
			return 0;

		inum temp_inum = n2i(info[0]);
		std::string temp_name = info[1];

		//vamos ver se o ficheiro existe
		if(isfile(temp_inum)){
			if(temp_name == name) //se existir verificamos se o nome do ficheiro é igual ao do parametro
				return temp_inum;
			else continue;
		}
	}
	return 0;

	//TODO completar esta merda

}

//TODO alterar isto depois quando tudo estiver feito
std::vector<std::string> yfs_client::split(const std::string& s, const std::string& match){

	std::vector<std::string> result;
	std::string::size_type start = 0; //size_type é mais apropriado para o tamanho da string que o int
	find_t pfind = &std::string::find_first_of;

	while (start != std::string::npos){
		std::string::size_type end = (s.*pfind)(match, start);
		std::string token = s.substr(start, end - start);

		if (!(token.empty())){
			result.push_back(token);
		}
		if ((start = end) != std::string::npos)
			start += 1;
	}

	return result;
}

//std::vector<std::string> yfs_client::split(const std::string& s, const std::string& match){
//
//	std::vector<std::string> result;
//	std::stringstream ss(s);
//	std::string line;
//
//	//queremos partir a string de entrada por linha
//	if(match.compare("\n")) {
//		while (getline(ss, line)) {
//			result.push_back(line);
//			printf("***** %s *****\n", line.c_str());
//		}
//	}
//	else { //aqui partimos a string por espaços
//
//		printf("espaços espaços\n");
//		printf("%s\n", s.c_str());
//
//		unsigned int pos = s.find(match);
//		unsigned int initialPos = 0;
//
//		while(pos != std::string::npos) {
//			result.push_back(s.substr(initialPos, pos - initialPos + 1));
//			initialPos = pos + 1;
//
//			pos = s.find(match, initialPos);
//		}
//
//		result.push_back(s.substr(initialPos, std::min(pos, s.size()) - initialPos + 1));
//	}
//
//	return result;
//}



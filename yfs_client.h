#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>


class yfs_client {
	extent_client *ec;
public:

	typedef unsigned long long inum;
	enum xxstatus { OK, RPCERR, NOENT, IOERR, FBIG };
	typedef int status;

	struct fileinfo {
		unsigned long long size;
		unsigned long atime;
		unsigned long mtime;
		unsigned long ctime;
	};
	struct dirinfo {
		unsigned long atime;
		unsigned long mtime;
		unsigned long ctime;
	};
	struct dirent {
		std::string name;
		yfs_client::inum inum;
	};

private:
	static std::string filename(inum);
	static inum n2i(std::string);
public:

	yfs_client(std::string, std::string);

	bool isfile(inum);
	bool isdir(inum);
	inum ilookup(inum di, std::string name);

	int getfile(inum, fileinfo &);
	int getdir(inum, dirinfo &);

	int put(inum, std::string buf);
	//	int createfile(inum, const char *, inum &);
	//	int createdir(inum, const char *, inum &);
	//	int createroot(inum, const char *);
	//	int lookup(inum, const char *, inum &);
	//	int readdir(inum, std::vector<dirent> &);
	//	int read(inum, off_t off, size_t size, std::string &);
	//	int write(inum, const char *, off_t off, size_t size);
};

#endif 

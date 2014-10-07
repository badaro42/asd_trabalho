/*
 * receive request from fuse and call methods of yfs_client
 *
 * started life as low-level example in the fuse distribution.  we
 * have to use low-level interface in order to get i-numbers.  the
 * high-level interface only gives us complete paths.
 */

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include "yfs_client.h"

int myid;
yfs_client *yfs;

int id() { 
	return myid;
}

//codigo dado, nao mexemos
yfs_client::status
getattr(yfs_client::inum inum, struct stat &st)
{
	yfs_client::status ret;

	bzero(&st, sizeof(st));

	st.st_ino = inum;
	printf("getattr %016llx %d\n", inum, yfs->isfile(inum));
	if(yfs->isfile(inum)){
		yfs_client::fileinfo info;
		ret = yfs->getfile(inum, info);
		if(ret != yfs_client::OK)
			return ret;
		st.st_mode = S_IFREG | 0666;
		st.st_nlink = 1;
		st.st_atime = info.atime;
		st.st_mtime = info.mtime;
		st.st_ctime = info.ctime;
		st.st_size = info.size;
		printf("   getattr -> %llu\n", info.size);
	} else {
		yfs_client::dirinfo info;
		ret = yfs->getdir(inum, info);
		if(ret != yfs_client::OK)
			return ret;
		st.st_mode = S_IFDIR | 0777;
		st.st_nlink = 2;
		st.st_atime = info.atime;
		st.st_mtime = info.mtime;
		st.st_ctime = info.ctime;
		printf("   getattr -> %lu %lu %lu\n", info.atime, info.mtime, info.ctime);
	}
	return yfs_client::OK;
}

//codigo dado, nao mexemos
void
fuseserver_getattr(fuse_req_t req, fuse_ino_t ino,
		struct fuse_file_info *fi)
{
	struct stat st;
	yfs_client::inum inum = ino; // req->in.h.nodeid;
	yfs_client::status ret;

	ret = getattr(inum, st);
	if(ret != yfs_client::OK){
		fuse_reply_err(req, ENOENT);
		return;
	}
	fuse_reply_attr(req, &st, 0);
}

//3A FASE
void
fuseserver_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi)
{
	printf("fuseserver_setattr 0x%x\n", to_set);
	if (FUSE_SET_ATTR_SIZE & to_set) {
		printf("   fuseserver_setattr set size to %zu\n", attr->st_size);
		// You fill this in

		yfs_client::inum local_inum = ino;
		yfs_client::fileinfo finfo;
		std::string t_buf;
		std::string file_data;
		struct stat st;

		//TODO verificar que o extent existe - feito
		if(yfs->get(local_inum, t_buf) != yfs_client::OK)
			fuse_reply_err(req, ENOENT);

		//TODO obter os atributos actuais do extent pretendido - feito
		if((yfs->getfile(local_inum, finfo) != yfs_client::OK) || (getattr(local_inum, st) != yfs_client::OK))
			fuse_reply_err(req, ENOENT);

		//o novo tamanho é igual a zero: esvaziamos o buffer (em vez de truncar o seu conteudo a zeros)
		if(attr->st_size == 0) {
			t_buf.clear();
			//TODO - o que é que é suposto devolver???
		}

		//o novo tamanho é menor que o tamanho actual do extent
		//truncamos a informação do ficheiro até ao tamanho pretendido
		else if(attr->st_size < finfo.size) {
			t_buf.substr(0, attr->st_size);
		}

		//o novo tamanho é maior que o actual: acrescentam-se zeros à informaçao actual
		else {
			//o novo tamanho do buffer
			unsigned int new_length = finfo.size + (attr->st_size - finfo.size);
			char temp_buf[new_length];

			//primeiro colocamos tudo a zero. o tamanho actual + o que acrescentamos
			//depois escrevemos a informação que ja estava no ficheiro no novo buffer, o resto fica a 0
			memset(temp_buf, 0, new_length);
			memcpy(&temp_buf, t_buf.c_str(), strlen(t_buf.c_str()) + 1);

			file_data = std::string(temp_buf);
		}

		//agr temos que actualizar a informação do ficheiro
		if(yfs->put(local_inum, file_data) == yfs_client::OK) {
			finfo.size = attr->st_size;
			st.st_size = attr->st_size;
			fuse_reply_attr(req, &st, 0);
		}

		else {
			fuse_reply_err(req, EIO);
		}


		//#if 0
		//		fuse_reply_attr(req, &st, 0);
		//#else
		//		fuse_reply_err(req, ENOSYS);
		//#endif
	} else {
		fuse_reply_err(req, EIO);
	}
}





//void
//fuseserver_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi)
//{
//	//	yfs_client::inum temp_inum = ino;
//	//	yfs_client::fileinfo finfo;
//	//	yfs_client::status c_ret;
//
//	printf("fuseserver_setattr 0x%x\n", to_set);
//	//if ((yfs->isfile(temp_inum)) && (FUSE_SET_ATTR_SIZE & to_set)) {
//	if (FUSE_SET_ATTR_SIZE & to_set) {
//		printf("   fuseserver_setattr set size to %zu\n", attr->st_size);
//		//struct stat st;
//
//		//finfo.size = attr->st_size;
//		//c_ret = yfs->setattr(temp_inum, ));
//
//
//		// You fill this in
//
//	}
//}

//3A FASE - feito, falta testar
//off - a posiçao onde queremos começar a ler
//size - o numero de bytes que queremos ler
void
fuseserver_read(fuse_req_t req, fuse_ino_t ino, size_t size,
		off_t off, struct fuse_file_info *fi)
{
	// You fill this in

	yfs_client::inum local_inum = ino;
	std::string buffer;

	//da erro pois nao é possivel ler ficheiros inexistentes
	if(yfs->get(local_inum, buffer) != yfs_client::OK)
		fuse_reply_err(req, ENOENT);

	//o ficheiro existe, temos que tratar agr da quantidade de informaçao que queremos
	else {
		//o offset ou o tamanho sao menores que zero: erro de I/O
		if((off < 0) || (size < 0))
			fuse_reply_err(req, EIO);

		//segundo a o header do fuse, caso o offset seja maior que o tamanho do buffer
		//actual do ficheiro, deve devolver zeros
		else if(off >= buffer.size())
			fuse_reply_buf(req, NULL, size);

		//a quantidade de informaçao que queremos ler esta dentro dos limites do ficheiro
		else if(off+size <= buffer.size()) {
			std::string to_reply = buffer.substr(off, size);
			fuse_reply_buf(req, to_reply.c_str(), size);
			//fuse_reply_buf(req, to_reply.c_str(), (int)to_reply.size); //TODO ou apenas size??
		}

		//offset dentro dos limites de ficheiro, mas a soma do offset com o size sai
		//dos limites, ler apenas a parte dentro do ficheiro
		else {
			std::string to_reply = buffer.substr(off, buffer.size()-off);
			fuse_reply_buf(req, to_reply.c_str(), buffer.size()-off);
			//fuse_reply_buf(req, to_reply.c_str(), (int)to_reply.size); //TODO ou apenas size??
		}
	}

	//#if 0
	//	fuse_reply_buf(req, buf, size);
	//#else
	//	fuse_reply_err(req, ENOSYS);
	//#endif
}

//3A FASE - feito, falta testar
//off - a posiçao onde queremos começar a escrever
//size - o numero de bytes que queremos ler
void
fuseserver_write(fuse_req_t req, fuse_ino_t ino,
		const char *buf, size_t size, off_t off,
		struct fuse_file_info *fi)
{
	// You fill this in

	yfs_client::inum local_inum = ino;
	std::string info;

	//da erro pois nao é possivel ler ficheiros inexistentes
	if(yfs->get(local_inum, info) != yfs_client::OK)
		fuse_reply_err(req, ENOENT);

	else {
		//todas as condiçoes que despoletam erro de I/O. se alguma delas se verificar, retorna erro
		if((size < 0) || (off < 0) || (off > info.size()))
			fuse_reply_err(req, EIO);

		else {
			std::stringstream ss;

			//introduzimos o inicio do conteudo (desde a posição zero até off) no stream
			//voltamos a verificar esta condicao pois se for zero nao fazemos nada.
			if(off > 0)
				ss << info.substr(0, off);

			//introduzir o conteudo novo na stream (mas apenas os bytes pretendidos, ou seja, de 0 a size)
			std::string temp_buf = buf;
			ss << temp_buf.substr(0,size);

			//calculamos o numero de bytes que se encontram no fim do ficheiro
			int remaining_length = off+size;

			//se a soma do offset com o size for menor que o tamanho do ficheiro, temos que voltar a escrever
			//os bytes remanescentes
			if(remaining_length < info.size()) {
				ss << info.substr(remaining_length, info.size()-remaining_length); //TODO ou usamos std::string::npos (para ir ate ao fim da string)??????
			}

			//escreve o novo conteudo no ficheiro
			if(yfs->put(local_inum, ss.str()) != yfs_client::OK)
				fuse_reply_err(req, EIO);

			fuse_reply_write(req, size);
		}
	}

	//#if 0
	//	fuse_reply_write(req, bytes_written);
	//#else
	//	fuse_reply_err(req, ENOSYS);
	//#endif
}

//3A FASE - feito, falta testar
void
fuseserver_open(fuse_req_t req, fuse_ino_t ino,
		struct fuse_file_info *fi)
{
	// You fill this in

	std::string buf;
	//fazemos o get do extent pretendido. se existir, devolve OK e abrimos o extent
	if(yfs->get(ino,buf) == yfs_client::OK)
		fuse_reply_open(req, fi);
	else
		fuse_reply_err(req, EIO);

	//#if 0
	//	fuse_reply_open(req, fi);
	//#else
	//	fuse_reply_err(req, ENOSYS);
	//#endif
}

//FUNCIONA!! (feito na FASE 2)
//não trata o caso de gerar inums que ja possam existir.
//implementar a geraçao de novo inum sempre que isso se verifique
yfs_client::status
fuseserver_createhelper(fuse_ino_t parent, const char *name,
		mode_t mode, struct fuse_entry_param *e)
{
	fuse_ino_t new_inum = rand() | 0x80000000;
	std::string buff;
	std::stringstream ss;
	std::string parent_content;

	//cria um novo ficheiro, com o conteudo vazio.
	//no nosso caso, nao retorna nenhum erro, pois, no caso de o ficheiro ja existir,
	//substitui o conteudo do ficheiro
	if(yfs->put(new_inum, buff) == yfs_client::IOERR)
		return yfs_client::IOERR;

	//vai buscar os conteudos do pai, para colocar no novo ficheiro
	if(yfs->get(parent, parent_content) == yfs_client::IOERR)
		return yfs_client::IOERR;

	ss << parent_content;
	ss << new_inum << " " << name << "\n";

	//actualiza a informacao do pai do novo ficheiro, que agr tem a info
	//do ficheiro que acabamos de criar
	if(yfs->put(parent, ss.str()) == yfs_client::IOERR)
		return yfs_client::IOERR;

	struct stat new_st;
	getattr(new_inum,new_st);
	e->ino = new_inum;
	e->generation = 42;
	e->attr = new_st;
	e->attr_timeout = 0.0;
	e->entry_timeout = 0.0;

	// You fill this in - ACHO QUE JA TA BOM, FALTA TESTAR - TESTADO

	return yfs_client::OK;
}

//codigo dado, nao mexemos
void
fuseserver_create(fuse_req_t req, fuse_ino_t parent, const char *name,
		mode_t mode, struct fuse_file_info *fi)
{
	struct fuse_entry_param e;
	if( fuseserver_createhelper( parent, name, mode, &e ) == yfs_client::OK ) {
		fuse_reply_create(req, &e, fi);
	} else {
		fuse_reply_err(req, ENOENT);
	}
}

//codigo dado, nao mexemos
void fuseserver_mknod( fuse_req_t req, fuse_ino_t parent,
		const char *name, mode_t mode, dev_t rdev ) {
	struct fuse_entry_param e;
	if( fuseserver_createhelper( parent, name, mode, &e ) == yfs_client::OK ) {
		fuse_reply_entry(req, &e);
	} else {
		fuse_reply_err(req, ENOENT);
	}
}

//(feito na FASE 2)
std::vector<std::string>
&fuse_server_split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

//(feito na FASE 2)
std::vector<std::string>
fuse_server_split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	fuse_server_split(s, delim, elems);
	return elems;
}

//FUNCIONA (feito na FASE 2)
void
fuseserver_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;
	bool found = false;

	e.attr_timeout = 0.0;
	e.entry_timeout = 0.0;

	// You fill this in:
	// Look up the file named `name' in the directory referred to by
	// `parent' in YFS. If the file was found, initialize e.ino and
	// e.attr appropriately.

	yfs_client::inum parent_inum = yfs->ilookup(parent, name);

	//apenas tratamos o caso em que o found passa a true,
	//pois ele é inicializado a false
	if(parent_inum != 0)
		found = true;

	if (found) {
		struct stat new_st;
		getattr(parent_inum, new_st);

		e.ino = parent_inum;
		e.attr = new_st;

		fuse_reply_entry(req, &e);
	}
	else
		fuse_reply_err(req, ENOENT);
}


struct dirbuf {
	char *p;
	size_t size;
};

void dirbuf_add(struct dirbuf *b, const char *name, fuse_ino_t ino)
{
	struct stat stbuf;
	size_t oldsize = b->size;
	b->size += fuse_dirent_size(strlen(name));
	b->p = (char *) realloc(b->p, b->size);
	memset(&stbuf, 0, sizeof(stbuf));
	stbuf.st_ino = ino;
	fuse_add_dirent(b->p + oldsize, name, &stbuf, b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
		off_t off, size_t maxsize)
{
	if (off < bufsize)
		return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
	else
		return fuse_reply_buf(req, NULL, 0);
}

//FUNCIONA!!
void
fuseserver_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
		off_t off, struct fuse_file_info *fi)
{
	yfs_client::inum inum = ino; // req->in.h.nodeid;
	struct dirbuf b;
	yfs_client::dirent e;

	printf("fuseserver_readdir\n");

	if(!yfs->isdir(inum)){
		fuse_reply_err(req, ENOTDIR);
		return;
	}

	memset(&b, 0, sizeof(b));

	//fill in the b data structure using dirbuf_add
	std::string parent_buffer;
	if(yfs->get(inum, parent_buffer) == yfs_client::IOERR) {
		fuse_reply_err(req, ENOENT);
	}
	else {
		std::vector<std::string> dir_entries = fuse_server_split(parent_buffer, '\n');

		unsigned int i; //para nao dar warning aquando da comparação com o size do vector (que é unsigned)
		for(i = 0; i < dir_entries.size(); i++) {
			std::string entry = dir_entries[i];
			std::vector<std::string> info = fuse_server_split(entry, ' ');

			//deve dar erro caso a info esteja incompleta
			if(info.size() != 2){
				fuse_reply_err(req,ENOENT);
			}

			inum = yfs_client::n2i(info[0]);
			std::string n_name = info[1];
			dirbuf_add(&b,n_name.c_str(),inum);
		}

		reply_buf_limited(req, b.p, b.size, off, size);
		free(b.p);
	}
}

void
fuseserver_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
		mode_t mode)
{
	struct fuse_entry_param e;

	// You fill this in
#if 0
	fuse_reply_entry(req, &e);
#else
	fuse_reply_err(req, ENOSYS);
#endif
}

void
fuseserver_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{

	// You fill this in
	// Success:	fuse_reply_err(req, 0);
	// Not found:	fuse_reply_err(req, ENOENT);
	fuse_reply_err(req, ENOSYS);
}

void
fuseserver_statfs(fuse_req_t req)
{
	struct statvfs buf;

	printf("statfs\n");

	memset(&buf, 0, sizeof(buf));

	buf.f_namemax = 255;
	buf.f_bsize = 512;

	fuse_reply_statfs(req, &buf);
}

struct fuse_lowlevel_ops fuseserver_oper;

int
main(int argc, char *argv[])
{
	char *mountpoint = 0;
	int err = -1;
	int fd;

	setvbuf(stdout, NULL, _IONBF, 0);

	if(argc != 4){
		fprintf(stderr, "Usage: yfs_client <mountpoint> <port-extent-server> <port-lock-server>\n");
		exit(1);
	}
	mountpoint = argv[1];

	srandom(getpid());

	myid = random();

	yfs = new yfs_client(argv[2], argv[3]);

	fuseserver_oper.getattr    = fuseserver_getattr;
	fuseserver_oper.statfs     = fuseserver_statfs;
	fuseserver_oper.readdir    = fuseserver_readdir;
	fuseserver_oper.lookup     = fuseserver_lookup;
	fuseserver_oper.create     = fuseserver_create;
	fuseserver_oper.mknod      = fuseserver_mknod;
	fuseserver_oper.open       = fuseserver_open;
	fuseserver_oper.read       = fuseserver_read;
	fuseserver_oper.write      = fuseserver_write;
	fuseserver_oper.setattr    = fuseserver_setattr;
	fuseserver_oper.unlink     = fuseserver_unlink;
	fuseserver_oper.mkdir      = fuseserver_mkdir;

	const char *fuse_argv[20];
	int fuse_argc = 0;
	fuse_argv[fuse_argc++] = argv[0];
#ifdef __APPLE__
	fuse_argv[fuse_argc++] = "-o";
	fuse_argv[fuse_argc++] = "nolocalcaches"; // no dir entry caching
	fuse_argv[fuse_argc++] = "-o";
	fuse_argv[fuse_argc++] = "daemon_timeout=86400";
#endif

	// everyone can play, why not?
	//fuse_argv[fuse_argc++] = "-o";
	//fuse_argv[fuse_argc++] = "allow_other";

	fuse_argv[fuse_argc++] = mountpoint;
	fuse_argv[fuse_argc++] = "-d";

	fuse_args args = FUSE_ARGS_INIT( fuse_argc, (char **) fuse_argv );
	int foreground;
	int res = fuse_parse_cmdline( &args, &mountpoint, 0 /*multithreaded*/,
			&foreground );
	if( res == -1 ) {
		fprintf(stderr, "fuse_parse_cmdline failed\n");
		return 0;
	}

	args.allocated = 0;

	fd = fuse_mount(mountpoint, &args);
	if(fd == -1){
		fprintf(stderr, "fuse_mount failed\n");
		exit(1);
	}

	struct fuse_session *se;

	se = fuse_lowlevel_new(&args, &fuseserver_oper, sizeof(fuseserver_oper),
			NULL);
	if(se == 0){
		fprintf(stderr, "fuse_lowlevel_new failed\n");
		exit(1);
	}

	struct fuse_chan *ch = fuse_kern_chan_new(fd);
	if (ch == NULL) {
		fprintf(stderr, "fuse_kern_chan_new failed\n");
		exit(1);
	}

	fuse_session_add_chan(se, ch);
	// err = fuse_session_loop_mt(se);   // FK: wheelfs does this; why?
	err = fuse_session_loop(se);

	fuse_session_destroy(se);
	close(fd);
	fuse_unmount(mountpoint);

	return err ? 1 : 0;
}

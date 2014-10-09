// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
	pthread_mutex_init(&storage_mutex, NULL);
	std::string buf;
	int r;
	put(0x1, buf, r);
}

extent_server::~extent_server() {
	pthread_mutex_destroy(&storage_mutex);
}

//cria um novo extent no servidor
int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &r)
{
	r = extent_protocol::OK;

	ScopedLock sl (&storage_mutex);
	extent_cell *ext_c;

	//procurar na BD se o extent já existe
	if(ext_storage.count(id) > 0) //já existe, procura-se entao a entrada correspondente na BD
		ext_c = ext_storage[id];
	else //ainda nao existe, criamos um novo objecto
		ext_c = new extent_cell();

	//em qualquer dos casos, alteramos todos os times referentes a um extent
	ext_c->extent_attr.atime = time(NULL);
	ext_c->extent_attr.mtime = time(NULL);
	ext_c->extent_attr.ctime = time(NULL);

	ext_c->data = buf;
	ext_c->extent_attr.size = buf.size();

	ext_storage[id] = ext_c;

	return extent_protocol::OK;
}

//procura o extent no servidor e, caso exista, devolve o seu conteudo e actualiza o atime
//se não existir, retorna um erro
int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
	ScopedLock sl (&storage_mutex);
	extent_cell *ext_c;

	//procurar na BD se o extent existe
	if(ext_storage.count(id) > 0) {//já existe, procura-se entao a entrada correspondente na BD
		ext_c = ext_storage[id];

		//copiamos o buffer para o parametro de entrada, actualizamos o atime e
		//gravamos a estrutura actualizada no mapa de estruturas
		buf = ext_c->data;
		ext_c->extent_attr.atime = time(NULL);
		ext_storage[id] = ext_c;

		return extent_protocol::OK;
	}
	else //o extent nao existe, retorna erro
		return extent_protocol::NOENT;

}

//
int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
	// You replace this with a real implementation. We send a phony response
	// for now because it's difficult to get FUSE to do anything (including
	// unmount) if getattr fails.

	ScopedLock sl (&storage_mutex);
	extent_cell *ext_c;

	//procurar na BD se o extent existe
	if(ext_storage.count(id) > 0) {
		ext_c = ext_storage[id];

		a.size = ext_c->extent_attr.size;
		a.atime = ext_c->extent_attr.atime;
		a.mtime = ext_c->extent_attr.mtime;
		a.ctime = ext_c->extent_attr.ctime;
	}
	else {
		a.size = 0;
		a.atime = 0;
		a.mtime = 0;
		a.ctime = 0;
	}
	return extent_protocol::OK;
}

//remove uma entrada do mapa de extents.
//se a chave dada nao existir o metodo nao faz nada
int extent_server::remove(extent_protocol::extentid_t id, int &)
{
	ScopedLock sl (&storage_mutex);

	//apagar o extent da base de dados, bem como a estrutura que lhe está associada
	//usamos o delete() para remover a memoria previamente alocada para a struct,
	//so depois removemos a entrada do mapa de extents
	delete(ext_storage[id]);
	ext_storage.erase(id);

	return extent_protocol::OK;
}


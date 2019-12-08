#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SECTOR_SIZE	512 //definição tamanho setor 
#define CLUSTER_SIZE	2*SECTOR_SIZE //tamanho do cluster = 2*tam setor
#define ENTER_BY_CLUSTER CLUSTER_SIZE /sizeof(dir_entry_t) //entra pelo cluster
#define CLUSTER_NUM	4096 //numero de cluster
#define FAT_NAME	"fat.part" //nome da fat, definido na doc

struct _dir_entry_t
{
	unsigned char filename[18];
	unsigned char attributes;
	unsigned char reserved[7];
	unsigned short first_block;
	unsigned int size;
};

typedef struct _dir_entry_t  dir_entry_t; 

union _data_cluster
{
	dir_entry_t dir[SECTOR_SIZE / sizeof(dir_entry_t)];
	unsigned char data[SECTOR_SIZE];
};

typedef union _data_cluster data_cluster;


/*Function declaration*/
void append(char* path, char* content);

void create(char* path);

void mkdir (char* path);

void read  (char* path);

void unlink(char* path);

int  encontrarEspacoLivre(dir_entry_t* dir);

int init(void);

int carregaFat(void);

void salvaFat(void);

data_cluster* carregaFatCluster(int block);

data_cluster* escreveCluster(int block, data_cluster* cluster);

void apagaCluster(int block);

data_cluster* encontraPrincipal(data_cluster* current_cluster, char* path, int* addr);

data_cluster* encontra(data_cluster* current_cluster, char* path, int* addr);

char* pegaNome(char* path);

int encontraEspacoVazio(dir_entry_t* dir);

void copiaString(char* copia, char* copiado);

void limpaString(char* s);

int procuraBlocoLivre(void);

void ls(char* path);

int temFilho(char* path);

void write(char* path, char* content);

void shell(void);

void comandoInicializar();

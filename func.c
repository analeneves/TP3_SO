#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SECTOR_SIZE	512 //definição tamanho setor 
#define CLUSTER_SIZE	2*SECTOR_SIZE //tamanho do cluster = 2*tam setor
#define ENTER_BY_CLUSTER CLUSTER_SIZE /sizeof(dir_entry_t) //entra pelo cluster
#define CLUSTER_NUM	4096 //numero de cluster
#define FAT_NAME	"fat.part" //nome da fat, definido na doc


clock_t tempo;
//tempo = clock();

/*DECLARAÇÃO DE DADOS*/
struct _dir_entry_t{
    unsigned char filename[18];
	unsigned char attributes;
	unsigned char reserved[7];
	unsigned short first_block;
	unsigned int size;
};

typedef struct _dir_entry_t  dir_entry_t; 
union _data_cluster
{
	dir_entry_t dir[CLUSTER_SIZE / sizeof(dir_entry_t)];
	unsigned char data[CLUSTER_SIZE];
};

typedef union _data_cluster data_cluster;
unsigned short fat[CLUSTER_NUM];
unsigned char boot_block[CLUSTER_SIZE];
dir_entry_t root_dir[32];
data_cluster clusters[4086];

/*DECLARAÇÃO DE FUNÇÕES DO SHELL
QUE SÃO NECESSÁRIAS PARA FUNCIONAMENTO
DO PROGRAMA, DEFINIDAS NA DOCUMENTAÇÃO*/
void create(char* caminho);
void mkdir (char* caminho);
void read  (char* caminho);
void append(char* caminho, char* content);
void unlink(char* caminho);
int  encontraEspacoVazio(dir_entry_t* dir);

/*FUNÇÃO INIT QUE INICIA O SISTEMA DE ARQUIVOS
"FORMATANDO" O SISTEMA DE ARQUIVOS VIRTUAL*/
int init(void){
	FILE* ptr_file;
	int i;
	ptr_file = fopen(FAT_NAME,"wb");
	for (i = 0; i < CLUSTER_SIZE; ++i)
		boot_block[i] = 0xbb;

	fwrite(&boot_block, sizeof(boot_block), 1,ptr_file);

	fat[0] = 0xfffd;
	for (i = 1; i < 9; ++i)
		fat[i] = 0xfffe;

	fat[9] = 0xffff;
	for (i = 10; i < CLUSTER_NUM; ++i)
		fat[i] = 0x0000;

	fwrite(&fat, sizeof(fat), 1, ptr_file);
	fwrite(&root_dir, sizeof(root_dir), 1,ptr_file);

	for (i = 0; i < 4086; ++i)
		fwrite(&clusters, sizeof(data_cluster), 1, ptr_file);

	fclose(ptr_file);
	return 1;
}

/*FUNÇÃO PARA SALVAR A FAT NO ARQUIVO fat.part*/
void salvaFat(void){
	int i;
	FILE* arqFAT;
	arqFAT = fopen(FAT_NAME, "r+b");
	fseek(arqFAT, sizeof(boot_block), 0);
	fwrite(fat, sizeof(fat), 1, arqFAT);
	fclose(arqFAT);
}

/*FUNÇÃO PARA CARREGAR UMA FAT JÁ EXISTENTE*/
int carregaFat(void){
	int i;
	FILE* arqFAT;
	arqFAT = fopen(FAT_NAME, "rb");
	//Caso não exista o arquivo fat.part
	if(arqFAT == NULL){
		printf("FAT não encontrada! Utilize o comando init para inicializá-la!\n");
		return 0;
	}else{
	//Caso exista carrega a fat
	fseek(arqFAT, sizeof(boot_block), 0);
	fread(fat, sizeof(fat), 1, arqFAT);
	fclose(arqFAT);
		return 1;
	}
}

/*FUNÇÃO PARA CARREGAR UM CLUSTER NA FAT*/
data_cluster* carregaFatCluster(int bloco){
	data_cluster* cluster;
	cluster = calloc(1, sizeof(data_cluster));
	FILE* arqFAT;
	arqFAT = fopen(FAT_NAME, "rb");
	fseek(arqFAT, bloco*sizeof(data_cluster), 0);
	fread(cluster, sizeof(data_cluster), 1, arqFAT);
	fclose(arqFAT);
	return cluster;
}

/*FUNÇÃO PARA ESCREVER NO CLUSTER DA FAT*/
data_cluster* escreveCluster(int bloco, data_cluster* cluster){
	FILE* arqFAT;
	arqFAT = fopen(FAT_NAME, "r+b");
	fseek(arqFAT, bloco*sizeof(data_cluster), 0);
	fwrite(cluster, sizeof(data_cluster), 1, arqFAT);
	fclose(arqFAT);
}

/*FUNÇÃO PARA APAGAR O CLUSTER*/
void apagaCluster(int bloco){
	data_cluster* cluster;
	cluster = calloc(1, sizeof(data_cluster));
	escreveCluster(bloco, cluster);
}

/*FUNÇÃO PARA ENCONTRAR O DIRETÓRIO ATUAL*/
data_cluster* encontraPrincipal(data_cluster* clusterAtual, char* caminho, int* endereco){

	char caminhoAux[strlen(caminho)];
	strcpy(caminhoAux, caminho);
	char* nomeDiretorio = strtok(caminhoAux, "/");
	char* resto = strtok(NULL, "\0");

	dir_entry_t* current_dir = clusterAtual->dir;

	int i=0;
	while (i < 32) {
		dir_entry_t filho = current_dir[i];
		if (strcmp(filho.filename, nomeDiretorio) == 0 && resto){
			data_cluster* cluster = carregaFatCluster(filho.first_block);
			*endereco = filho.first_block;
			return encontraPrincipal(cluster, resto, endereco);
		}
		else if (strcmp(filho.filename, nomeDiretorio) == 0 && resto){
			return NULL;
		}
		i++;
	}

	if (!resto)
		return clusterAtual;

	return NULL;
}

/*FUNÇÃO PARA APONTAR OS DIRETÓRIOS QUE ESTÃO NO CAMINHO PASSADO*/
data_cluster* encontra(data_cluster* clusterAtual, char* caminho, int* endereco){
	if (!caminho || strcmp(caminho, "/") == 0)
		return clusterAtual;

	char caminhoAux[strlen(caminho)];
	strcpy(caminhoAux, caminho);
	char* nomeDiretorio = strtok(caminhoAux, "/");
	char* resto = strtok(NULL, "\0");

	dir_entry_t* current_dir = clusterAtual->dir;

	int i=0;
	while (i < 32) {
		dir_entry_t filho = current_dir[i];
		if (strcmp(filho.filename, nomeDiretorio) == 0){
			data_cluster* cluster = carregaFatCluster(filho.first_block);
			*endereco = filho.first_block;
			return encontra(cluster, resto, endereco);
		}
		i++;
	}
	return NULL;
}

/*RETORNA O NOME DO CAMINHO PASSADO */
char* pegaNome(char* caminho){

	char nomeAuxiliar[strlen(caminho)];
	strcpy(nomeAuxiliar, caminho);

	char* nome = strtok(nomeAuxiliar, "/");
	char* resto = strtok(NULL, "\0");
	if (resto != NULL)
		return pegaNome(resto);

	return (char*) nome;
}

/*FUNÇÃO PARA ENCONTRAR UM ESPAÇO VAZIO NO DIRETORIO*/
int encontraEspacoVazio(dir_entry_t* dir){
	int i;
	for (i = 0; i < 32; ++i){
		if (!dir->attributes)
			return i;
		dir++;
	}
	return -1;
}

/*FUNÇÃO PARA FAZER COPIA DA STRING*/
void copiaString(char* copia, char* copiado){
	int tam = strlen(copiado);
	int i;
	for (i = 0; i < tam; ++i) {
		copia[i] = copiado[i];
	}
}

/*FUNÇÃO PARA PROCURAR UM BLOCO LIVRE 
O QUAL RETORNARÁ (-1)*/
int procuraBlocoLivre(void){
	carregaFat();
	int i;
	for (i = 10; i < 4096; ++i)
		if(!fat[i]){
			fat[i] = -1;
			salvaFat();
			return i;
		}
	return 0;
}

/*FUNÇÃO QUE UTILIZA O CALLOC PARA LIMPAR A STRING
UTILIZANDO NO WRITE PARA SOBRESCREVER*/
void limpaString(char* s)
{
	s = (unsigned char*)calloc(CLUSTER_SIZE,sizeof(unsigned char));
}
void ls(char* caminho)
{
	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(9);
	data_cluster* cluster = encontra(clusterDoRoot, caminho, &enderecoRoot);
	int i;
	if (cluster){
		for (i = 0; i < 32; ++i){
			if (cluster->dir[i].attributes == 1 || cluster->dir[i].attributes == 2)
				printf("Arquivos:%s\n", cluster->dir[i].filename);
		}
	}
	else
		printf("Caminho não encontrado! Tente novamente!\n");
}

/*FUNÇÃO PARA CRIAR UM DIRETORIO*/
void mkdir(char* caminho)
{
	if(caminho == "/")
		return;

	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(9);
	data_cluster* clusterPrincipal = encontraPrincipal(clusterDoRoot, caminho, &enderecoRoot);

	if (clusterPrincipal){
		int posicaoLivre = encontraEspacoVazio(clusterPrincipal->dir);
		int blocoFat = procuraBlocoLivre();
		if (blocoFat && posicaoLivre != -1) {
			char* nomeDiretorio = pegaNome(caminho);
			copiaString(clusterPrincipal->dir[posicaoLivre].filename, nomeDiretorio);
			clusterPrincipal->dir[posicaoLivre].attributes = 1;
			clusterPrincipal->dir[posicaoLivre].first_block = blocoFat;
			escreveCluster(enderecoRoot, clusterPrincipal);
		}
	}
	else
		printf("Caminho não encontrado! Tente novamente!\n");
}

/*FUNÇÃO QUE RETORNA (1) CASO TENHA ALGUM DIRETORIO
OU ARQUIVO DENTRO DO MESMO */
int temFilho(char* caminho){
	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(9);
	data_cluster* cluster = encontra(clusterDoRoot, caminho, &enderecoRoot);
	int i;
	if (cluster){
		for (i = 0; i < 32; ++i){
			if (cluster->dir[i].attributes == 1 || cluster->dir[i].attributes == 2)
				return 1;
		}
	}
	else
		return 0;
}

/*FUNÇÃO PARA CRIAR UM ARQUIVO*/
void create(char* caminho)
{
	if(caminho == "/")
		return;

	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(9);
	data_cluster* clusterPrincipal = encontraPrincipal(clusterDoRoot, caminho, &enderecoRoot);

	if (clusterPrincipal){
		int posicaoLivre = encontraEspacoVazio(clusterPrincipal->dir);
		int blocoFat = procuraBlocoLivre();
		if (blocoFat && posicaoLivre != -1) {
			char* nomeDiretorio = pegaNome(caminho);
			copiaString(clusterPrincipal->dir[posicaoLivre].filename, nomeDiretorio);
			clusterPrincipal->dir[posicaoLivre].attributes = 2;
			clusterPrincipal->dir[posicaoLivre].first_block = blocoFat;
			escreveCluster(enderecoRoot, clusterPrincipal);
		}
	}
	else
			printf("Caminho não encontrado! Tente novamente!\n");
}

/*FUNÇÃO PARA REMOVER UM ARQUIVO OU DIRETORIO*/
void unlink(char* caminho)
{
	carregaFat();
	int i;
	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(enderecoRoot);
	encontraPrincipal(clusterDoRoot, caminho, &enderecoRoot);
	data_cluster* cluster = carregaFatCluster(enderecoRoot);
	if(temFilho(caminho) == 1){
	printf("Não é possível excluir! O diretório precisa estar vazio! Tente novamente\n");
	}else{
		if (cluster != NULL) {
			char* name = pegaNome(caminho);
			for (i = 0; i < 32; ++i) {
				if (strcmp(cluster->dir[i].filename, name)==0) {
					cluster->dir[i].attributes = -1;
					
					int first = cluster->dir[i].first_block;
					fat[first] = 0x0000;
					salvaFat();
					apagaCluster(enderecoRoot);
				
				}
			}
		}else{
			printf("Arquivo não encontrado, tente novamente!\n");
		}
	}

}

/*FUNÇÃO PARA SOBRESCREVER EM UM ARQUIVO EXISTENTE*/
void write(char* caminho, char* content)
{	
	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(9);
	data_cluster* cluster = encontra(clusterDoRoot, caminho, &enderecoRoot);
	if (cluster){
		limpaString(cluster->data);
		copiaString(cluster->data, content);
		escreveCluster(enderecoRoot, cluster);
	}
	else
		printf("Caminho não encontrado, tente novamente!\n");

}

/*FUNÇÃO PARA ADICIONAR UM TEXTO EM UM ARQUIVO 
JÁ EXISTENTE*/
void append(char* caminho, char* content)
{
	carregaFat();
	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(9);
	data_cluster* cluster = encontra(clusterDoRoot, caminho, &enderecoRoot);
	if (cluster){
		char* data = cluster->data;
		strcat(data, content);
		copiaString(cluster->data, data);
		escreveCluster(enderecoRoot, cluster);
	}

	else
		printf("Arquivo não encontrado, tente novamente!\n");

}

/*FUNÇÃO PARA LER DADOS DE UM ARQUIVO EXISTENTE*/
void read(char* caminho)
{
	carregaFat();
	int enderecoRoot = 9;
	data_cluster* clusterDoRoot = carregaFatCluster(9);
	data_cluster* cluster = encontra(clusterDoRoot, caminho, &enderecoRoot);
	if (cluster){
		printf("%s\n", cluster->data);
		printf("Tempo:%f\n",(clock() - tempo) / (double)CLOCKS_PER_SEC);

	}else{
		printf("Arquivo não encontrado, tente novamente!\n");
}
}

/*FUNÇÃO SHELL, QUE UNE TODAS EM UMA*/
void shell(void)
{
	char nome[4096] = { 0 };
    char nomeAux[4096] = { 0 };
	char copiaDoNome[4096] = { 0 };
	const char aux[2] = "/";
	char aux2[4096] = { 0 };

	char *comandoDigitado;
	int i;

	printf("Digite o comando que você deseja: load, init, ls, mkdir, create, unlink, write, append ou read\n");
	fgets(nome,4096,stdin);

	strcpy(copiaDoNome,nome);

	comandoDigitado = strtok(nome,aux);

	if ( strcmp(comandoDigitado, "init\n") == 0)
	{
		init();
	}
	else if ( strcmp(comandoDigitado, "load\n") == 0)
	{
		carregaFat();
	}
	else if ( strcmp(comandoDigitado, "ls ") == 0 && copiaDoNome[3] == '/') 
	{
		for(i = 3; i < strlen(copiaDoNome)-1; ++i)
		{
			aux2[i-3] = copiaDoNome[i];
		}
		ls(aux2);
	}
	else if ( strcmp(comandoDigitado, "mkdir ") == 0 && copiaDoNome[6] == '/')
	{
		for(i = 6; i < strlen(copiaDoNome)-1; ++i)
		{
			aux2[i-6] = copiaDoNome[i];
		}
		mkdir(aux2);
	}
	else if ( strcmp(comandoDigitado, "create ") == 0 && copiaDoNome[7] == '/')
	{
		for(i = 7; i < strlen(copiaDoNome)-1; ++i)
		{
			aux2[i-7] = copiaDoNome[i];
		}
		create(aux2);
	}
		else if ( strcmp(comandoDigitado, "unlink ") == 0 && copiaDoNome[7] == '/')
	{
		for(i = 7; i < strlen(copiaDoNome)-1; ++i)
		{
			aux2[i-7] = copiaDoNome[i];
		}
		unlink(aux2);
		printf("Tempo:%f\n",(clock() - tempo) / (double)CLOCKS_PER_SEC);
	}
	else if ( strcmp(comandoDigitado, "write ") == 0 && copiaDoNome[6] == '/')
	{
		for(i = 6; i < strlen(copiaDoNome)-1; ++i)
		{
			aux2[i-6] = copiaDoNome[i];
		}
		printf("Digite o texto que será escrito no arquivo:\n");
		fgets(nomeAux,4096,stdin);
		write(aux2,nomeAux);
	}
	else if ( strcmp(comandoDigitado, "append ") == 0 && copiaDoNome[7] == '/')
	{
		for(i = 7; i < strlen(copiaDoNome)-1; ++i)
		{
			aux2[i-7] = copiaDoNome[i];
		}
		printf("Digite o texto que será anexado no arquivo:\n");
		fgets(nomeAux,4096,stdin);
		append(aux2,nomeAux);
		printf("Tempo:%f\n",(clock() - tempo) / (double)CLOCKS_PER_SEC);
	}
	else if ( strcmp(comandoDigitado, "read ") == 0 && copiaDoNome[5] == '/')
	{
		for(i = 5; i < strlen(copiaDoNome)-1; ++i)
		{
			aux2[i-5] = copiaDoNome[i];
		}
		read(aux2);
	}

	else printf("Erro, tente novamente! Verifique se escreveu o init e load corretamente.\n");
}



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "func.h"

int main(){
void comandoInicializar();
	int aux = 0;
	while(aux != 1){
		char nome[4096] = { 0 };
		printf("Digite o comando para começar (init ou load):  ");
		fgets(nome,4096,stdin);
		if ( strcmp(nome, "init\n") == 0){
			aux = init();
		}
		else if ( strcmp(nome, "load\n") == 0){
			aux = carregaFat();
		}
	} while (1){
        shell();
    }
     return 0;
}
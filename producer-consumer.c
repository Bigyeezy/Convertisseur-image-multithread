/* 
    Exemple de Structure de producteur consurmer
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define STACK_MAX 10

typedef struct stack_t {
    int data[STACK_MAX];
    int count;
    int max;
    pthread_mutex_t lock;
    pthread_cond_t can_consume;
    pthread_cond_t can_produce;
} Stack;

static Stack stack;

void stack_init() {
    pthread_cond_init(&stack.can_produce, NULL);
    pthread_cond_init(&stack.can_consume, NULL);
    pthread_mutex_init(&stack.lock, NULL);
    stack.max = STACK_MAX;
    stack.count = 0;
    srand(time(NULL));
}

void* producer(void* arg);
void* producer(void* arg) {

	while(true) {
		
	}

	return NULL;
}

void* consumer(void* arg);
void* consumer(void* arg) {

	int total = 0;

	while(true) {
		
	}
	
	return NULL;
}

int main(int argc, char** argv) {

    // L'enseble de nos threads
    pthread_t threads_id[2];

    // Initialisation de la stack utilisé par les producteurs et consomateurs
    stack_init();

    // Initialisation de l'attribut du thread pour qu'il soit détachable ( donc pas joignable )
    pthread_attr_t attr;
    phtread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Création des poducteurs ( dont on attend pas qu'ils se termine )
    for(int i = 0; i < 1; i++) {
		pthread_create(&threads_id[i], &attr, producer, NULL);
	}

    // création du consomateur
    pthread_create(&threads_id[1], NULL, consumer, NULL);

    pthread_join(threads_id[1], NULL);
    pthread_attr_destroy(&attr); // libération de la mémoire pour les attributs du trhead

    return 0;
}
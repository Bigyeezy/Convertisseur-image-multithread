/*
	//gcc edge-detect.c bitmap.c -O2 -ftree-vectorize -fopt-info -mavx2 -fopt-info-vec-all
	//UTILISER UNIQUEMENT DES BMP 24bits
*/

#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"
#include <stdint.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>

#define DIM 3
#define LENGHT DIM
#define OFFSET DIM /2
#define STACK_MAX 10
#define LENGTH_STACK_OF_FILES 50

const float KERNEL[DIM][DIM] = {{-1, -1,-1},
							   {-1,8,-1},
							   {-1,-1,-1}};

typedef struct stack_t {
    Image data[STACK_MAX];
	char* filesToProcess[LENGTH_STACK_OF_FILES];
    int count;
	int fileToProcessCount;
    int max;
	int maxFileToProcess;
    pthread_mutex_t lock;
    pthread_cond_t can_consume;
    pthread_cond_t can_produce;
} Stack;

typedef struct Color_t {
	float Red;
	float Green;
	float Blue;
} Color_e;

static Stack stack;

void stack_init() {
    pthread_cond_init(&stack.can_produce, NULL);
    pthread_cond_init(&stack.can_consume, NULL);
    pthread_mutex_init(&stack.lock, NULL);
    stack.max = STACK_MAX;
	stack.maxFileToProcess = LENGTH_STACK_OF_FILES;
    stack.count = 0;
	stack.fileToProcessCount = 0;
    srand(time(NULL));
}

void apply_effect(Image* original, Image* new_i);
void apply_effect(Image* original, Image* new_i) {

	int w = original->bmp_header.width;
	int h = original->bmp_header.height;

	*new_i = new_image(w, h, original->bmp_header.bit_per_pixel, original->bmp_header.color_planes);

	for (int y = OFFSET; y < h - OFFSET; y++) {
		for (int x = OFFSET; x < w - OFFSET; x++) {
			Color_e c = { .Red = 0, .Green = 0, .Blue = 0};

			for(int a = 0; a < LENGHT; a++){
				for(int b = 0; b < LENGHT; b++){
					int xn = x + a - OFFSET;
					int yn = y + b - OFFSET;

					Pixel* p = &original->pixel_data[yn][xn];

					c.Red += ((float) p->r) * KERNEL[a][b];
					c.Green += ((float) p->g) * KERNEL[a][b];
					c.Blue += ((float) p->b) * KERNEL[a][b];
				}
			}

			Pixel* dest = &new_i->pixel_data[y][x];
			dest->r = (uint8_t)  (c.Red <= 0 ? 0 : c.Red >= 255 ? 255 : c.Red);
			dest->g = (uint8_t) (c.Green <= 0 ? 0 : c.Green >= 255 ? 255 : c.Green);
			dest->b = (uint8_t) (c.Blue <= 0 ? 0 : c.Blue >= 255 ? 255 : c.Blue);
		}
	}
}

void* producer(void* arg);
void* producer(void* arg) {

	while (1)
	{
		
		if (stack.fileToProcessCount <= 0) {
			printf("[PRODUCER] Plus de fichier a traiter.\n\n");
			break;
		} else {
			pthread_mutex_lock(&stack.lock);
			stack.fileToProcessCount--;
			printf("[PRODUCER] stack count %d\n",stack.fileToProcessCount );
			printf("[PRODUCER] Fichier en cours de traitement '%s'\n", stack.filesToProcess[stack.fileToProcessCount]);

			if (stack.count >= stack.max) {
				printf("[PRODUCER] En Attente, la stack est plein...\n");
				while (stack.count >= stack.max)
				{	
					pthread_cond_wait(&stack.can_produce, &stack.lock);
				}
				printf("[PRODUCER] la stack est prête !\n");
			} 

			char inputpath[90] = "/mnt/d/Document/workspace/C/Convertisseur-image-multithread/input/"; // peut etre sauvegarder dans la stack aussi
			printf("[PRODUCER] Ouverture de l'image\n");
			Image img = open_bitmap(strcat(inputpath,stack.filesToProcess[stack.fileToProcessCount]));
			Image new_i;
			printf("[PRODUCER] Application des effets\n");
			apply_effect(&img, &new_i);
			stack.data[stack.count] = new_i;
			stack.count++;
			printf("[PRODUCER] Fichier traité\n");
			pthread_cond_signal(&stack.can_consume);	
			pthread_mutex_unlock(&stack.lock);
		}
	}
	
	return NULL;
}

void* consumer(void* arg);
void* consumer(void* arg) {
	
	int outputCount = stack.fileToProcessCount;

	while(1) {
		pthread_mutex_lock(&stack.lock);

		while (stack.count == 0)
		{
			printf("[CONSUMER] Pas d'image a traiter pour le moment..\n");
			pthread_cond_wait(&stack.can_consume, &stack.lock);
		}

		stack.count--;
		outputCount--;
		printf("[CONSUMER] Enregistrement de l'image numéro : %d\n", stack.count);
		
		char outputPath[100] = "/mnt/d/Document/workspace/C/Convertisseur-image-multithread/output/";
		char outputImageName[50];
		sprintf(outputImageName, "bmp_tank_output%d.bmp", outputCount);
		strcat(outputPath, outputImageName);

		printf("[CONSUMER] Enregistrement de l'image dans le fichier suivant : '%s'", outputPath);

		
		save_bitmap(stack.data[stack.count], outputPath) ;
		
		
		
		printf("[CONSUMER] Fichié sauvegardé ! Le nombre d'image restant est  : %d\n", stack.count);
		
		if (outputCount < 0) {
			printf("[CONSUMER] Plus d'image à traiter.\n\n");
			break;
		}
		
		pthread_cond_broadcast(&stack.can_produce);
		pthread_mutex_unlock(&stack.lock);
	}
	
	return NULL;
}


/**/

void get_all_files_from_folder(char* inputPath) {
	printf("[GET_INPUT_FILES] Récupération des fichiers à traiter dans le répertoire '%s'\n", inputPath);

	DIR *directory;
	struct dirent *file;
	directory = opendir(inputPath);
	if (directory) {
		while ((file = readdir(directory)) != NULL)
		{
			printf("[GET_INPUT_FILES] Fichier en cours de traitement : %s \n", file->d_name);
			if (strstr(file->d_name, ".bmp") != NULL) {
				if (stack.fileToProcessCount < stack.maxFileToProcess) {
					stack.filesToProcess[stack.fileToProcessCount] = file->d_name;
					stack.fileToProcessCount++;
					printf("[GET_INPUT_FILES] Fichier enregistré. : %s \n", file->d_name);
				} else {
					printf("[GET_INPUT_FILES] Le nombre de fichier *.bmp a taité est trop important. Seul les %d premiers fichiers seront traités.\n", LENGTH_STACK_OF_FILES);
				}
			} 
		}
		closedir(directory);
		printf("[GET_INPUT_FILES] Fin de la récupération des fichiers.\n\n");
		
	} else {
		printf("[GET_INPUT_FILES] Impossible d'ouvrir le répertoire d'entrée.\n\n");
		return ;
	}
}


/**/

int main(int argc, char** argv) {
	pthread_t threads_id[5];

	stack_init();

	get_all_files_from_folder("/mnt/d/Document/workspace/C/Convertisseur-image-multithread/input/");

	for (int i = 0; i < stack.fileToProcessCount; i++) {
		printf("file number %d : %s\n", i, stack.filesToProcess[i]);
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	for ( int i = 0; i < 4; i++) {
	    pthread_create(&threads_id[i], &attr, producer, NULL);
	}

	pthread_create(&threads_id[4], NULL, consumer, NULL);

	pthread_join(threads_id[4], NULL);
	pthread_attr_destroy(&attr); 
	
	return 0;
}
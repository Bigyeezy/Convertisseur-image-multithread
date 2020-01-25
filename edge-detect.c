/*
	//gcc edge-detect.c bitmap.c -O2 -ftree-vectorize -fopt-info -mavx2 -fopt-info-vec-all
	//UTILISER UNIQUEMENT DES BMP 24bits
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bitmap.h"
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <dirent.h> 

#define DIM 3
#define LENGHT DIM
#define OFFSET DIM /2
#define STACK_MAX 10

const float KERNEL[DIM][DIM] = {{-1, -1,-1},
							   {-1,8,-1},
							   {-1,-1,-1}};

typedef struct stack_t {
    Image data[STACK_MAX];
    int count;
    int max;
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
    stack.count = 0;
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

	DIR *directory;
	struct dirent * dir;
	directory = opendir("./input");
	if(directory) {
		while((dir = readdir(directory) != NULL)) {
			Image img = open_bitmap(dir->d_name);
			Image new_i;

			apply_effect(&img, &new_i);
			if ( stack.count < stack.max) {
				stack.data[stack.count] = new_i;
				stack.count = stack.count +1;
			}
		}
		closedir(directory);
	}

	return NULL;
}

void* consumer(void* arg);
void* consumer(void* arg) {

	//int total = stack.count;
	time_t seconds;
	seconds = time(NULL);
	
	while(1) {
		if (stack.count > 0 ) {
			Image img = stack.data[stack.count];
			char imageName[] = seconds/3600 + "test_out.bmp";
			if ( save_bitmap(img, imageName) == 0) {	
				stack.count = stack.count - 1;
			} else {
				printf("Something went wrong with image number %d", stack.count);
			}
		} else {
			break;
		}
	}
	
	return NULL;
}

int main(int argc, char** argv) {

	//Image img = open_bitmap("bmp_tank.bmp");
	//Image new_i;

	pthread_t threads_id[2];

	stack_init();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	for ( int i = 0; i < 1; i++) {
		pthread_create(&threads_id[i], &attr, producer, NULL);
	}

	pthread_create(&threads_id[1], NULL, consumer, NULL);

	pthread_join(threads_id[1], NULL);
	pthread_attr_destroy(&attr);
	
	
	//apply_effect(&img, &new_i);
	//save_bitmap(new_i, "test_out.bmp");
	
	
	return 0;
}
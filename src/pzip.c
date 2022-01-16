#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pzip.h"

/**
 * pzip() - zip an array of characters in parallel
 *
 * Inputs:
 * @n_threads:		   The number of threads to use in pzip
 * @input_chars:		   The input characters (a-z) to be zipped
 * @input_chars_size:	   The number of characaters in the input file
 *
 * Outputs:
 * @zipped_chars:       The array of zipped_char structs
 * @zipped_chars_count:   The total count of inserted elements into the zippedChars array.
 * @char_frequency[26]: Total number of occurences
 *
 * NOTE: All outputs are already allocated. DO NOT MALLOC or REASSIGN THEM !!!
 *
 */
void *zipChars(void *arg);

int numCharsPerThread;
int *offsets;
pthread_barrier_t myBarrier;
pthread_mutex_t myMutex;

struct thread_data {
	char *input_chars;
	int tid;
	int input_chars_start;
	int input_chars_end;
	struct zipped_char *zipped_chars;
	int *zipped_chars_count;
	int *char_frequency;
};

void *zipChars(void *arg){

	struct thread_data *data = arg;

	int sizeZipped = 0;

	struct zipped_char *myZipped_chars = (struct zipped_char *)malloc(numCharsPerThread * sizeof(struct zipped_char));

	for(int i = data->input_chars_start; i < data->input_chars_end; i++){
		int count = 1;
		struct zipped_char aChar;
		aChar.character = data->input_chars[i];
		aChar.occurence = 0;
		
		while(i < data->input_chars_end - 1 && (data->input_chars[i] == data->input_chars[i+1])){
			count++;
			i++;
		}
				
		aChar.occurence += count;
		
		memcpy(myZipped_chars + sizeZipped, &aChar, sizeof(struct zipped_char));
	
		sizeZipped++;
	}
	
	offsets[data->tid] = sizeZipped;
	
	if(pthread_mutex_lock(&myMutex) != 0){
		perror("Error locking mutex");
	}

	int newSize = sizeZipped + *(data->zipped_chars_count);

	memcpy(data->zipped_chars_count, &newSize, sizeof(int));

	if(pthread_mutex_unlock(&myMutex) != 0){
		perror("Error unlocking mutex");
	}

	pthread_barrier_wait(&myBarrier);

	for(int i = 0; i < sizeZipped; i++){
		int char_index = (int)myZipped_chars[i].character - 97;
		int aChange = myZipped_chars[i].occurence + *(data->char_frequency + char_index);
		memcpy(data->char_frequency + char_index,  &aChange, sizeof(int));
	}
	
	int aStart = 0;

	for(int i = 0; i < data->tid; i++){
		aStart += *(offsets + i);
	}

	memcpy(data->zipped_chars + aStart,myZipped_chars,(sizeZipped * sizeof(struct zipped_char)));
	
	free(data);
	free(myZipped_chars);

	pthread_exit((void*) 0);

}
			
	

void pzip(int n_threads, char *input_chars, int input_chars_size,
	  struct zipped_char *zipped_chars, int *zipped_chars_count,
	  int *char_frequency)
{
	void *status;
	pthread_attr_t attr;
	numCharsPerThread = input_chars_size/n_threads;
	pthread_t threads[n_threads];

	int o[n_threads];
	offsets = o;

	pthread_mutex_init(&myMutex, NULL);
	pthread_barrier_init(&myBarrier, NULL, n_threads);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(int i = 0; i < n_threads; i++){
		int start = i * numCharsPerThread;
		int end = start + numCharsPerThread;
		struct thread_data *data_for_thread = malloc(sizeof(struct thread_data));
		(*data_for_thread).input_chars = input_chars;
		(*data_for_thread).tid = i;
		(*data_for_thread).input_chars_start = start;
		(*data_for_thread).input_chars_end = end;
		(*data_for_thread).zipped_chars = zipped_chars;
		(*data_for_thread).zipped_chars_count = zipped_chars_count;
		(*data_for_thread).char_frequency = char_frequency;

		//data_for_thread = {input_chars, i, start, end, zipped_chars, zipped_chars_count, char_frequency};

		if(pthread_create(&threads[i], &attr, zipChars, (void*)data_for_thread) != 0){
			perror("Could not create thread");
		}
	}
	
	pthread_attr_destroy(&attr);
	
	for(int i =0; i < n_threads; i++){
		pthread_join(threads[i], &status);
	}

	pthread_mutex_destroy(&myMutex);
	pthread_barrier_destroy(&myBarrier);
	return;
	
}

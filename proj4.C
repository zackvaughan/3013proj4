#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <iostream>
using namespace std;

int strings_in_file = 0;
int global_fd;
int global_buffer_size;
int global_strcounter = 0;
int global_num_threads;
const char *global_string;

void *thread_func (void *thread_id) {
  int tid = (int) thread_id;
  struct stat sb;
  char *buffer;
  int i;
  int strcounter = 0;
  int sslength = strlen(global_string);
  
  if (fstat(global_fd, &sb) < 0){
    perror("Could not stat file to obtain its size");
    exit(1);
  }

  int buffer_size = (sb.st_size / global_num_threads) + 1;
  
  if ((buffer = (char *) mmap (NULL, sb.st_size, PROT_READ, 
			       MAP_SHARED, global_fd, 0)) == (char *) -1) {
    perror("Could not mmap file");
    exit (1);
  }
  
  int starting_val = ((tid - 1) * buffer_size);

  for (i = starting_val; 
       (i < sb.st_size) && (i < starting_val + buffer_size - 1); i++) {

    if (buffer[i] == global_string[global_strcounter]) {
      global_strcounter++;
      
      if (global_strcounter == sslength) {
	strings_in_file++;
	strcounter = 0;
      }
    } else {
      global_strcounter = 0;
    }
  }

  if(munmap(buffer, sb.st_size) < 0){
    perror("Could not unmap memory");
    exit(1);
  }
}






int main (int argc, const char *argv[]) {
  if(argc < 3) {
    cerr << "Not enough parameters\n";
    exit(1);
  }
  
  const char *searchstring = argv[2];
  pthread_t threads[8];
  int sslength = strlen(searchstring);
  int mmap_on = 0;
  int thread_on = 0;
  int size = 1024;
  char *buffer;
  int strcounter = 0;
  int fd, filechecker, i, num_threads, status;
  struct stat sb;
  
  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    perror("Could not open file");
    exit (1);
  }
  
  if (argc >= 4) {
    if (!strcmp("mmap", argv[3])) {
      mmap_on = 1;
    } else if ('p' == argv[3][0]) {
      num_threads = (argv[3][1] - 48);

      if (num_threads > 8) {
	printf("Too many threads! Try 8 or less\n");
	exit(-1);
      }

      thread_on = 1;
    } else {
      if (atoi(argv[3]) <= 8192) {
	size = atoi(argv[3]);
      } else {
	printf("Specified size is too big. Try 8192 bytes or less");
	exit(1);
      }
    }
  }

  if (mmap_on) {
    if (fstat(fd, &sb) < 0){
      printf("Could not stat file to obtain its size");
      exit(1);
    }

    if ((buffer = (char *) mmap (NULL, sb.st_size, PROT_READ, 
				 MAP_SHARED, fd, 0)) == (char *) -1) {
      printf("Could not mmap file");
      exit (1);
    }

    for (i = 0; i < sb.st_size; i++) {
      if (buffer[i] == searchstring[strcounter]) {
	strcounter++;
	
	if (strcounter == sslength) {
	  strings_in_file++;
	  strcounter = 0;
	}
      } else {
	strcounter = 0;
      }
    }
  
    if(munmap(buffer, sb.st_size) < 0){
      perror("Could not unmap memory");
      exit(1);
    }
  } else if (thread_on) {
    global_fd = fd;
    global_string = searchstring;
    global_num_threads = num_threads;

    for (i = 0; i < num_threads; i++) {
      status = pthread_create(&threads[i], NULL, thread_func, (void *) (i + 1));

      if (status != 0) {
	printf("Oops. pthread_create returned error code %d\n", status);
	exit(1);
      }
    }

    for (i = 0; i < num_threads; i++) {
      pthread_join(threads[i], NULL);
    }
  } else {
    while ((filechecker = read(fd, buffer, size)) > 0) {
      for (i = 0; i < filechecker; i++) {
	if (buffer[i] == searchstring[strcounter]) {
	  strcounter++;

	  if (strcounter == sslength) {
	    strings_in_file++;
	    strcounter = 0;
	  }
	} else {
	  strcounter = 0;
	}
      }
    }
  }

  //printf("%d", filechecker);
  //printf("\33[2K\r");

  printf("Occurrences of the string \"%s\": %d\n", 
	 searchstring, strings_in_file);

  if (fd > 0) {
    close(fd);
  }

  return 0;
}

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


const int INF = 300000000;
//The group of arguments passed to thread
struct thread_data{
    int id;
    int start;
    int end; /* the sub-range is from start to end */
};

typedef struct {
    pthread_mutex_t count_lock;  //mutex semaphore for the barrier
    pthread_cond_t ok_to_proceed;  //condition variable for leaving
    int count; //count of the number of threads who have  arrived
} barrier_t;

bool *global_list;
int num_threads;
barrier_t barrier;

void barrier_init(barrier_t *b) {
  b -> count = 0;
  pthread_mutex_init(&(b -> count_lock), NULL);
  pthread_cond_init(&(b -> ok_to_proceed), NULL);
}

void barrier_(barrier_t *b, int id) {
  pthread_mutex_lock(&(b -> count_lock));
  b -> count ++;
  if (b -> count == num_threads) {
    b -> count = 0; // must be reset for future re-use
    pthread_cond_broadcast(&(b -> ok_to_proceed));
  } else {
    while (pthread_cond_wait(&(b -> ok_to_proceed), &(b -> count_lock)) != 0);
  }
  pthread_mutex_unlock(&(b -> count_lock));
}

void barrier_destroy(barrier_t *b)
{
  pthread_mutex_destroy(&(b -> count_lock));
  pthread_cond_destroy(&(b -> ok_to_proceed));
}

void* do_sieve(void *thrd_arg) {
  struct thread_data *t_data;
  int i, start, end;
  int k = 2;//The current prime number in first loop
  int myid;

  // Initialize part of the global array
  t_data = (struct thread_data *) thrd_arg;
  myid = t_data->id;
  start = t_data->start;
  end = t_data->end;

  //First loop: find all prime numbers that's less than sqrt(n)
  while (k * k <= end) {
    int flag;
    if(k * k >= start)
      flag = 0;
    else
      flag = 1;
    //Second loop: mark all multiples of current prime number
    for (i = !flag? k * k - 1 : start + k - start % k - 1; i <= end; i += k)
      global_list[i] = 1;
    i = k;
    //wait for other threads to finish the second loop for current prime   number
    barrier_(&barrier, myid);
    //find next prime number that's greater than current one
    while (global_list[i] == 1)
      i++;
    k = i + 1;
  }
  //decrement the counter of threads before exit
  pthread_mutex_lock (&barrier.count_lock);
  num_threads--;
  if (barrier.count == num_threads) {
    barrier.count = 0;  /* must be reset for future re-use */
    pthread_cond_broadcast(&(barrier.ok_to_proceed));
  }
  pthread_mutex_unlock (&barrier.count_lock);
  pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
  int i, n, n_threads;
  int k, nq, nr;
  struct thread_data *t_arg;
  pthread_t *thread_id;
  pthread_attr_t attr;

  if (argc == 1) {
    perror("Not enough arguments!");
    exit(EXIT_FAILURE);
  }
  n_threads = atoi(argv[1]);
  if (argc > 2) {
    n = atoi(argv[2]);
  } else {
    n = INF;
  }
  /* Pthreads setup: initialize barrier and explicitly create
   threads in a joinable state (for portability)  */
  barrier_init(&barrier);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  //Initialize global list
  global_list = (bool *)malloc(sizeof(bool) * n);
  for(i = 0; i < n;i++)
    global_list[i] = 0;
  /* create arrays of thread ids and thread args */
  thread_id = (pthread_t *)malloc(sizeof(pthread_t)*n_threads);
  t_arg = (struct thread_data *)malloc(sizeof(struct thread_data)*n_threads);

  /* distribute load and create threads for computation */
  nq = n / n_threads;
  nr = n % n_threads;

  k = 1;
  num_threads = n_threads;
  for (i = 0; i < n_threads; i++){
    t_arg[i].id = i;
    t_arg[i].start = k;
    if (i < nr)
      k = k + nq + 1;
    else
      k = k + nq;
    t_arg[i].end = k-1;
    pthread_create(&thread_id[i], &attr, do_sieve, (void *) &t_arg[i]);
  }

  /* Wait for all threads to complete then print all prime numbers */
  for (i = 0; i < n_threads; i++) {
    pthread_join(thread_id[i], NULL);
  }
  int j = 1;
  //Get the spent time for the computation works by all participanting threads
  for (i = 1; i < n; i++) {
    if (global_list[i] == 0) {
      printf("%d\n", i + 1);
      j++;
    }
  }
  // Free memory
  free(global_list);
  pthread_attr_destroy(&attr);
  barrier_destroy(&barrier); // destroy barrier object
  pthread_exit (NULL);
}
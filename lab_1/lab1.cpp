#include <unordered_map>
#include <vector>
#include <string>
#include <list>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

using namespace std ;

static vector<int> running_sum = {} ;


template<typename K, typename V>
class ThreadSafeKVStore{
public:
	void init_mutex(const K& key){
		if(map_mutex.find(key) != map_mutex.end()){
			return ;
		} 
		pthread_mutex_lock(&u_lock) ;
		if(map_mutex.find(key) != map_mutex.end()){ 
			pthread_mutex_unlock(&u_lock) ;
			return ;
		}
		map_mutex[key] = PTHREAD_MUTEX_INITIALIZER ;
		pthread_mutex_unlock(&u_lock) ;
		return ;
	} ;

	bool insert(const K& key, const V& value){
		init_mutex(key) ;
		pthread_mutex_lock(&map_mutex[key]) ;
		map[key] = value ;
		pthread_mutex_unlock(&map_mutex[key]) ;
		return true ;
	} ;

	bool accumulate(const K& key, const V& value){
		init_mutex(key) ;
		pthread_mutex_lock(&map_mutex[key]) ;
		if(map.find(key) == map.end())
			map[key] = value ;
		else
			map[key] = map[key] + value ;
		pthread_mutex_unlock(&map_mutex[key]) ;
		return true ;
	} ;

	bool lookup(const K& key, V& value){

		//check if the mutex of the key exist
		if(map_mutex.find(key) == map_mutex.end())
			return true ;

		pthread_mutex_lock(&map_mutex[key]) ;
		/*critical section*/
		if(map.find(key) == map.end()){
			return false ; //no longer there
			pthread_mutex_unlock(&map_mutex[key]) ;
		}
		else
			value = map[key] ;
		/******************/
		pthread_mutex_unlock(&map_mutex[key]) ;
		return true ;
		//}
	} ;

	bool remove(const K& key){
		//return true when operation succeed.
		//return false if certain invariants do not hold.

		//check if the mutex of the key exist
		if(map_mutex.find(key) == map_mutex.end())
			return true ;
		pthread_mutex_lock(&map_mutex[key]) ;
		/*critical section*/
		if(map.find(key) == map.end()){
			pthread_mutex_unlock(&map_mutex[key]) ;
			return true ;
		}
		else
			map.erase(key) ;
		/******************/
		pthread_mutex_unlock(&map_mutex[key]) ;
		return true ;
	} ;

	void print_map(){
		for(auto it = map.begin() ; it != map.end() ; ++it)
		cout << "key: " << it->first << " val: " << it->second << endl ; 
	}

	int return_sum(){
		auto begin = map.begin() ;
		auto end = map.end() ;
		int acc_sum = 0 ;
		int running_sum_ = 0 ;
		for(auto it = begin ; it != end ; ++it){
			acc_sum += it->second ;
		}
		return acc_sum ;
	}

private:
	unordered_map<K, V> map ;
	unordered_map<K, pthread_mutex_t> map_mutex ; //should always exist rather than delete with map
	pthread_mutex_t u_lock = PTHREAD_MUTEX_INITIALIZER ; //utility lock
} ;

template<typename T>
class ThreadSafeListenerQueue{
public:
	ThreadSafeListenerQueue<T>(){
		semaphore = sem_open("/semaphore", O_CREAT, 0644 ,0);
	}

	~ThreadSafeListenerQueue<T>(){
		//semaphore is just like pipe that will be initialized as a file
		//and should be close and deleted after being used.
		sem_close(semaphore);
		sem_unlink("/semaphore") ;
	}

	bool push(const T& element){
		//pthread_mutex_lock(&lock) ;
		//cout << "push" << endl ;
		q.push_back(element) ;
		sem_post(semaphore);
		//pthread_mutex_unlock(&lock) ;
		return true ; 
	}
	bool pop(T& element){
		pthread_mutex_lock(&lock) ;
		if(q.empty()){
			pthread_mutex_unlock(&lock) ;
			return false ;
		}
		else{
			element = q.front() ;
			q.pop_front() ;
			pthread_mutex_unlock(&lock) ;
			return true ;
		}
	}
	bool listen(T& element){
		pthread_mutex_lock(&lock) ;
		sem_wait(semaphore);
		element = q.front() ;
		cout << "pop" << endl ;
		q.pop_front() ;
		pthread_mutex_unlock(&lock) ;
		return true ;
	}
private:
	list<T> q ;
	sem_t* semaphore;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
};

typedef void* (*thread_func_ptr)(void*) ;

typedef struct thread_info {   
   int id;
   string key ;        
   ThreadSafeKVStore<string, int>* vk_map = NULL;
   ThreadSafeListenerQueue<int>* vk_q = NULL;
}thread_info;

int get_opt(int argc, char** argv){
	int res = 1 ; 
    int opt ;
    while ((opt = getopt(argc,argv,"n:")) != EOF)
    	//get the optional argument into sname
	res = stoi(optarg) ; 
	if(res < 0){
		cout << "Number of thread should be greater than 0." << endl;
		abort() ;
	}else if(res == 0)
		return 1 ;
	else
    	return res ; 

}


/*
static void * thread_start1(void *arg){
	thread_info* info = (thread_info*)arg ;
	srand(time(0));
	string victim = "user" + to_string(rand()%10) ;
	string member = "user" + to_string(rand()%10) ; 
	info->vk_map->remove(info->key) ;
}
static void * thread_start2(void *arg){
	thread_info2* info = (thread_info2*)arg ;
	srand(time(0));
	int num_gen = 5;//rand()%5;
	int num_lis = 7;//rand()%5;
	int num_victim = rand()%5 ;

	cout<< "gen: " << num_gen << " lis: " << num_lis << endl ;
	for(int i = 0 ; i < num_gen; ++i){
		info->vk_q->push(i) ;
	}
	
	for(int i = 0 ; i < num_lis ; ++i){
		int tmp = 0 ;
		info->vk_q->listen(tmp) ;
		cout << tmp << endl ;
	}
}
*/

static void * thread_start(void *arg){
	thread_info* info = (thread_info*)arg ;
	int threads_id = info->id ;
	srand(time(0));
	//string victim = "user" + to_string(rand()%10) ;
	for(int i = 0 ; i < 2000 ; ++i){
		string key = "user" + to_string(rand()%500) ;
		int val = rand()%513 - 256 ;
		running_sum[threads_id] += val ;
		info->vk_map->accumulate(key, val) ;
	}

	for (int i = 0; i < 8000; ++i)
	{
		string key = "user" + to_string(rand()%500) ;
		int val ;
		info->vk_map->lookup(key, val) ;
	}

	info->vk_q->push(running_sum[threads_id]) ;
}

int main(int argc, char** argv){
	int NUM_THREADS=  get_opt(argc, argv) ;
	running_sum = vector<int>(NUM_THREADS, 0) ;
	pthread_t threads[NUM_THREADS];
    int thread_id;
    void** t = NULL;
    int sum = 0 ;
    int i = 0 ;
    ThreadSafeKVStore<string, int>* vk_map = new ThreadSafeKVStore<string, int>() ;
    ThreadSafeListenerQueue<int>* vk_q = new ThreadSafeListenerQueue<int>() ;

	for(; i < NUM_THREADS; i++ ) {
		thread_info* arg = new thread_info() ;
		arg->id = i ;
		arg->key = "user" + to_string(i%10) ;
		arg->vk_map = vk_map ;
		arg->vk_q = vk_q ;
		thread_id = pthread_create(&threads[i], NULL, &thread_start, (void*)arg);
	}

	for(i = 0 ; i < NUM_THREADS ; ++i){
		int tmp ;
		vk_q->listen(tmp) ;
		sum += tmp ;
	}

	int tmp_sum = 0 ; 
	int acc_sum = vk_map->return_sum() ;

	for(i = 0 ; i < NUM_THREADS ; ++i)
		tmp_sum += running_sum[i] ;

	if(acc_sum == sum){
		cout << "pass the test:" << tmp_sum << " sum_from_map:" << acc_sum << " sum_from_queue:" << sum << endl ;
	}else{
		cout << "test not pass, running_sum:" << tmp_sum << " sum_from_map:" << acc_sum << " sum_from_queue:" << sum << endl ;
	}

	return 0 ;
}
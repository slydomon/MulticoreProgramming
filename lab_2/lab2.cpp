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
#include <random>
#include <cfloat>
#include <chrono>
#include <time.h>
#include <thread>
#include <condition_variable>

using namespace std ;

static const double range = 10.0 ;

typedef void* (*thread_func_ptr)(void*) ;

typedef struct thread_info { 
    vector<pair<int, int> > points ;
    double current_best ;
}thread_info;

static int get_opt(int argc, char** argv){
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

static vector<double> find_coefficient(const vector<pair<int, int> >& points, double& recent_best){
	//srand (time(NULL));
	int size = points.size() ;
	vector<double> coefficients(size , 0.0f) ;
	double result ;
	double tmp ; 
	double cur ;
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	std::default_random_engine generator (seed);
  	uniform_real_distribution<double> distribution(-range, range);

  	//generate the ascending value
	do{
		//generate the coefficient
		for(int i = 0 ; i < size ; ++i){
			coefficients[i] = distribution(generator);
		}
		result = 0 ;  
		for(int i = 0 ; i < size ; ++i){
			tmp  = 0 ; 
			cur = 1 ; 
			for(int j = 0 ; j < size ; ++j){
				tmp = tmp + cur * coefficients[j] ;
				cur = cur * points[i].first ;
			}
			result = result + abs(points[i].second - tmp) ;
		}
		//cout << fixed << result << endl;   	
    }while(result > recent_best) ;
    //cout << fixed << "last_best: " << recent_best << " update: " << result << endl;
    recent_best = result ;
	coefficients.push_back(result) ;
	return coefficients ;
}

//wrapper function to invoke the real job.
static void * thread_start(void *arg){
	thread_info* info = (thread_info*)arg ;;
	vector<double>* tmp = new vector<double>(find_coefficient(info->points, info->current_best)) ;
	return (void*)tmp ;	
}

#include <mutex>
#include <condition_variable>

class semaphore
{
private:
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
    pthread_cond_t cond;
    unsigned int count = 1; // Initialized as locked.

public:
	semaphore(){
		pthread_cond_init(&cond, NULL) ;
	}

    void sem_post() {
    	cout << "sem_post wait for lock" << endl;
        pthread_mutex_lock(&lock);
        cout << "sem_post hold the lock" << endl;
        ++count;
        //cout << count << endl;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock) ;
        cout << "sem_post release the lock" << endl;
    }

    void sem_wait() {
    	//cout << "count" << count << endl ;
    	cout << "sem_wait wait for lock" << endl;
        pthread_mutex_lock(&lock) ;
        cout << "sem_wait hold the lock" << endl;
        //cout << "count" << count << endl ;
        bool flag = true ;
        //cout << count << endl; 
        --count;
        while(!count){ // Handle spurious wake-ups.
        	cout << "sem_wait is wait for signal and release lock" <<endl; 
            if(pthread_cond_wait(&cond, &lock) == -1) perror("sem_wait: cond_wait");
            flag = false ;
            cout << "sem_wait is awake" << endl; 
        }
        if(!flag) return ;
        if(pthread_mutex_unlock(&lock) == -1) perror("sem_wait: unlock");
        cout << "sem_wait release the lock" << endl;
        return ;
        //cout << "exit" << endl ;
    }

    long get_count(){
    	return count;
    }
};

template<typename T>
class ThreadSafeListenerQueue{
public:
	static pthread_mutex_t name_lock ;
	static int sema_name ;
	ThreadSafeListenerQueue<T>(){
		//increment the count to acquire the new name for semaphore.
		pthread_mutex_lock(&name_lock) ;
		int tmp = sema_name ;
		++sema_name ;
		pthread_mutex_unlock(&name_lock) ;
		sprintf(name, "/semaphore_%d", tmp) ;
		//printf("%s \n", name) ;
		if(sem_unlink(name) == -1)
			perror("constructor: sema_unlink") ; //very important: semaphore make sure we do not use the old, contaminated one.
		semaphore = sem_open(name, O_CREAT|O_EXCL, S_IRWXU|S_IWUSR , 0) ;
		if(semaphore == SEM_FAILED){
		    perror("open semaphore fail!!");
		    exit(0);
		}
	}

	list<T> get_list(){
		pthread_mutex_lock(&lock) ;
		//int size = q.size() ;
		list<T> copy ;
		for(auto it = q.begin() ; it != q.end() ; ++it){
			copy.push_back(*it) ;
		}
		pthread_mutex_unlock(&lock) ;
		return copy ;
	}

	~ThreadSafeListenerQueue<T>(){
		//semaphore is just like pipe that will be initialized as a file
		//and should be close and deleted after being used.
		if(sem_close(semaphore) == -1)
			perror("distructor sem_close: ");
		if(sem_unlink(name) == -1)
			perror("distructor sem_unlink: ") ;
			
	}

	bool push(const T& element){
		//pthread_mutex_lock(&lock) ;
		//cout << "push" << endl ;
		q.push_back(element) ;
		if(sem_post(semaphore) == -1)
			perror("push sem_post: ");
		//sema.sem_post() ;
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
			if(!q.empty())
				q.pop_front() ;
			else{
				cout << "pop: pop fail: queue is empty" << endl ;
				exit(0) ;
			}
			pthread_mutex_unlock(&lock) ;
			return true ;
		}
	}
	bool listen(T& element){
		pthread_mutex_lock(&lock) ;
		if(sem_wait(semaphore) == -1){
			perror("listen: ") ;
			exit(0) ;
		}
		//sema.sem_wait() ;
		//cout << "count: " << sema.get_count() << endl ;
		if(!q.empty()){
			element = q.front() ;
			q.pop_front() ;
		}
		else{
			cout << "Listen: pop fail: queue is empty" <<endl ;
			exit(0) ;
		}
		pthread_mutex_unlock(&lock) ;
		return true ;
	}

	// need to figure out a way to fix this buggy function.
	/*T front(){
		pthread_mutex_lock(&lock) ;
		T tmp ;
		if(!q.empty())
			tmp = q.front() ;
		else{
			cout << "Front: queue is empty" << endl ;
			return NULL;
		}
		pthread_mutex_unlock(&lock) ;
		return tmp ;
	}
	*/

	//be careful to use this function since it only guarantee the current size;
	int size(){
		return q.size() ;
	}
private:
	list<T> q ;
	//semaphore sema;
	sem_t* semaphore ;
	char name[25] ;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
};

template<class T>
void printv(vector<T> input){
	int size = input.size() ;
	for(int i = 0 ; i < size ; ++i){
		cout<<fixed ;
		cout << input[i] << " " ;
	}
	cout << endl ;
}

class task
{
public:
	task() ;
	task(void* _input, thread_func_ptr _job) //constructor
	{ 
		input = _input;
		job = _job;
	}
	task(const task& t) //copy constructor, if task1 = task2 => task1 is task2 rather than the copy of task2.
	{	
		input = t.input; 
		job = t.job;
	} 
	~task() ;
	const thread_func_ptr get_job(){return job ;}
	void* get_input(){return input ;}
	const void* get_output(){return output ;}
	const void set_output(void* out){output = out; } 
	const void set_id(const int& _id){id = _id ;} 
	//const void set_output(void* _output){output = _output ;}
private:
	int id ;
	void* input ;
	void* output ;
	thread_func_ptr job ;
};

//user-define compare function
task* my_cmp(task* t1, task* t2){
	if(!t1 && !t2) return NULL ;
	else if(!t1) return t2 ;
	else if(!t2) return t1 ;
	else{
		vector<double>* tmp1 = (vector<double>*)(t1->get_output()) ;
		vector<double>* tmp2 = (vector<double>*)(t2->get_output()) ;
		int size = tmp1->size() ;
		if(tmp1->at(size-1) < tmp2->at(size-1)){
			//delete t2 ;
			return t1 ;
		}
		else{
			//delete t1 ;
			return t2 ;
		}
	}
}

class thread_pool
{
typedef void* (thread_pool::*func_ptr)(void*) ;
typedef task* (*cmp)(task* t1, task* t2) ;

public:
	thread_pool(int thread_number){
		thread_num = thread_number ;
		threads = new pthread_t[thread_num] ;
	}

	~thread_pool(){
		delete threads ;
		//pending.~ThreadSafeListenerQueue() ;
		//complete.~ThreadSafeListenerQueue() ;
	}

	/*
	void thread_pool_set_cmp(cmp my_cmp){
		user_cmp_func = my_cmp ;
	}
	*/

	task* thread_pool_aggregate(cmp my_cmp){
		pthread_mutex_lock(&complete_q_lock) ;
		int size = complete.size() ;
		task* res = nullptr ;
		if(size > 1){
			//cout << "agg case2" <<endl ;
			for(int i = 0 ; i < size-1 ; ++i){

				task* t1 ;
				if(!complete.pop(t1))
				{ 
					cout << "thread_pool_aggregate_at_least: first pop fail, complete q is empty, return null" << endl ;
					return res ;
				}
				if(!complete.pop(res)){ 
					cout << "thread_pool_aggregate_at_least: second pop fail, return t1" << endl ;
					complete.push(res) ;
					return t1;
				}
				//res = complete.front() if fixed, it can improve performance
				res = my_cmp(t1, res) ;
				complete.push(res) ;
			}
		}
		pthread_mutex_unlock(&complete_q_lock) ;
		return res; 
	}

	task* thread_pool_aggregate_at_least(cmp my_cmp, int batch_volume, timespec* estimate_time_per_task){
		estimate_time_per_task->tv_sec *= batch_volume ;
		estimate_time_per_task->tv_nsec *= batch_volume ;
		nanosleep(estimate_time_per_task, NULL) ; //in order to successfully execute the batch.
		task* res = nullptr ;
		pthread_mutex_lock(&complete_q_lock) ;
		int size = complete.size() ;
		if(size > batch_volume){
			for(int i = 0 ; i < size-1 ; ++i){
				task* t1 ;
				if(!complete.pop(t1))
				{ 
					cout << "thread_pool_aggregate_at_least: first pop fail, complete q is empty" << endl ;
					return res ;
				}
				if(!complete.pop(res)){ 
					cout << "thread_pool_aggregate_at_least: second pop fail, return t1" << endl ;
					complete.push(res) ;
					return t1;
				}
				//res = complete.front() ;
				res = my_cmp(t1, res) ;
				complete.push(res) ;
			}
		}
		pthread_mutex_unlock(&complete_q_lock) ;
		return res ;
	}

	//1.wait for the job, 2.execute the job, 3. put the result into task->output, 4. push into complete for aggregating. 
	void* dispatching(void* arg){
		void* tmp ;
		while(proceed){
			//debug:
			//cout << "round" << endl ;
			task* t = nullptr ;
			pending.listen(t) ;
			thread_func_ptr job = t->get_job() ;
			tmp = job(t->get_input()) ;
			t->set_output(tmp) ;
			pthread_mutex_lock(&complete_q_lock) ;
			complete.push(t) ;
			pthread_mutex_unlock(&complete_q_lock) ;
		}
		return NULL ;
	}

	void add_task(task* t){
		//debug:
		//cout << "add_task" << endl;
		pending.push(t) ;
		//cout << "push task" << endl ;
		/*pthread_mutex_lock(&id_lock) ;
		t->set_id(id) ; 
		thread_pool::update_id() ;
		pthread_mutex_unlock(&id_lock) ;*/
	}

	//if ture, task is dispatched ; if false, there is no task in pending 
	void dispatch_task(int pid){ 
		pthread_create(&threads[pid], NULL, run_dispatching, this) ;
		pthread_detach(threads[pid]) ;
		return ; 
	}

	void thread_pool_run(){
		for(int i = 0 ; i < thread_num ; ++i){
			//debug:
			//cout << "create_pthread" << endl;
			this->dispatch_task(i) ;
		}
	}

	void thread_pool_stop(){
		pthread_mutex_lock(&continue_lock) ;
		proceed = false ;
		pthread_mutex_unlock(&continue_lock) ;
	}

	list<task*> get_complete_task(){
		pthread_mutex_lock(&complete_q_lock) ;
		return complete.get_list() ;
		pthread_mutex_unlock(&complete_q_lock) ;
	}
private:
	//private data
	ThreadSafeListenerQueue<task*> pending ;
	ThreadSafeListenerQueue<task*> complete ;
	pthread_mutex_t complete_q_lock = PTHREAD_MUTEX_INITIALIZER ;
	int thread_num ;
	pthread_mutex_t continue_lock = PTHREAD_MUTEX_INITIALIZER ;
	bool proceed = true ;
	//pthread_mutex_t id_lock = PTHREAD_MUTEX_INITIALIZER ;
	int id = 0 ;
	pthread_t* threads;
	//cmp user_cmp_func = NULL ;

	//private function
	/*void update_id(){
		pthread_mutex_lock(&id_lock) ;
		++id ;
		pthread_mutex_unlock(&id_lock) ;
	}*/

	static void* run_dispatching(void* arg){
		thread_pool* tmp = reinterpret_cast<thread_pool*>(arg);
        tmp->dispatching(NULL);
        return 0;
	}


};

//user-defined get best ;
static void get_best(task* t, double& current_best){
	if(t == NULL)
		return ; 
	else{
		vector<double>* tmp = (vector<double>*)((t)->get_output()) ;
		int last = tmp->size() ;
		double best = tmp->at(last-1) ;
		if(best > 0 && best < current_best){
			current_best = best ;
			cout << "update: " << current_best << endl;
		}
	}
	return ; 
}

//double current_best = 0.01;

static const double BENCHMARK = 0.1;
static double current_best = 10000 ;
template<>
pthread_mutex_t ThreadSafeListenerQueue<task*>::name_lock = PTHREAD_MUTEX_INITIALIZER ;

template<>
int ThreadSafeListenerQueue<task*>::sema_name = 0 ;

int main(int argc, char** argv){
	int NUM_THREADS = get_opt(argc, argv) ;
	//ThreadSafeListenerQueue<task*>::static_var_initializer() ;
	thread_pool pool(NUM_THREADS) ;
	//sem_t* semaphore = sem_open("/semaphore", O_CREAT, 0644 , -NUM_THREADS + 2);
    for(int i = 0 ; i < 100 ; ++i){
    	//input initialize:
    	thread_info* input = new thread_info() ;
	    input->points.push_back(pair<int, int>(1, 3)) ;
	    input->points.push_back(pair<int, int>(10, 4)) ;
	    input->points.push_back(pair<int, int>(-4, 5)) ;
	    //input->points.push_back(pair<int, int>(-4, 5)) ;
	    input->current_best = current_best ;

	    //task initialize:
	    pool.add_task(new task((void*)input, thread_start)) ;
    }

    //pool.thread_pool_set_cmp(my_cmp) ;

    //fire all the thread in the thread pool
	pool.thread_pool_run() ;

	//main thread keeps updating until it meet the requirement. 
	timespec* time_rest = new timespec() ; 
	time_rest->tv_sec = 0;
	time_rest->tv_nsec = 11111111 ;

    do{
  		task* tmp = pool.thread_pool_aggregate_at_least(my_cmp, 20, time_rest) ;
  		//task* tmp = pool.thread_pool_aggregate(my_cmp) ;
  		get_best(tmp, current_best) ;
  		thread_info* input = new thread_info() ;
	    input->points.push_back(pair<int, int>(1, 3)) ;
	    input->points.push_back(pair<int, int>(10, 4)) ;
	   	input->points.push_back(pair<int, int>(-4, 5)) ;
	   	//input->points.push_back(pair<int, int>(-4, 5)) ;
  		input->current_best = current_best ;
  		pool.add_task(new task((void*)input, thread_start)) ;
  		//nanosleep(time_rest, NULL) ;
  		//sleep(1);
    }while(abs(current_best) > BENCHMARK) ;
	
    pool.thread_pool_stop() ;
    cout << "main end:"  << current_best << endl;
	
	return 0 ;
}
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

using namespace std ;

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
  	uniform_real_distribution<double> distribution(-5.0, 5.0);
	do{
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
			double diff = (points[i].second - tmp) ;
			result = result + diff*diff ;
		}
		//cout << fixed << result << endl;   	
    }while(result > recent_best && -result < recent_best) ;
    cout << fixed << "last_best: " << recent_best << " update: " << result << endl;
    recent_best = result ;
	coefficients.push_back(result) ;
	return coefficients ;
}

static void * thread_start(void *arg){
	thread_info* info = (thread_info*)arg ;
	//cout << "begin work" <<endl ;
	vector<double>* tmp = new vector<double>(find_coefficient(info->points, info->current_best)) ;
	return (void*)tmp ;	
}

template<typename T>
class ThreadSafeListenerQueue{
public:
	ThreadSafeListenerQueue<T>(){
		semaphore = sem_open("/semaphore", O_CREAT, 0644 ,0);
	}

	list<T> get_list(){
		pthread_mutex_lock(&lock) ;
		int size = q.size() ;
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
		//cout << "pop" << endl ;
		q.pop_front() ;
		pthread_mutex_unlock(&lock) ;
		return true ;
	}
private:
	list<T> q ;
	sem_t* semaphore;
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


class thread_pool
{
typedef void* (thread_pool::*func_ptr)(void*) ;

public:
	thread_pool(int thread_number){
		thread_num = thread_number ;
		threads = new pthread_t[thread_num] ;
		//pid_proceed = new bool[thread_num] ;
	}

	~thread_pool(){
		delete threads ;
	}

	void* dispatching(void* arg){
		void* tmp ;
		while(proceed){
			task* t ;
			pending.listen(t) ;
			thread_func_ptr job = t->get_job() ;
			tmp = job(t->get_input()) ;
			t->set_output(tmp) ;
			pending.push(t) ;
		}
		cout << "exit" << endl ;
		return NULL ;
	}

	void add_task(task* t){
		pending.push(t) ;
		pthread_mutex_lock(&id_lock) ;
		t->set_id(id) ; 
		thread_pool::update_id() ;
		pthread_mutex_unlock(&id_lock) ;
	}

	//if ture, task is dispatched ; if false, there is no task in pending 
	void dispatch_task(int pid){ 
		pthread_create(&threads[pid], NULL, run_dispatching, this) ;
		pthread_detach(threads[pid]) ;
		return ; 
	}

	void thread_pool_run(){
		for(int i = 0 ; i < thread_num ; ++i){
			this->dispatch_task(i) ;
		}
	}

	void thread_pool_stop(){
		pthread_mutex_lock(&continue_lock) ;
		proceed = false ;
		pthread_mutex_unlock(&continue_lock) ;
	}

	list<task*> get_complete_task(){
		//debug
		//cout << "get_complete_task" << endl;
		return pending.get_list() ;
	}

	

private:
	//ThreadSafeListenerQueue<pthread_t*> avail_threads ;
	ThreadSafeListenerQueue<task*> pending ;
	ThreadSafeListenerQueue<task*> complete ;
	int thread_num ;
	pthread_mutex_t continue_lock ;
	bool proceed = true ;
	pthread_mutex_t id_lock ;
	int id = 0 ;
	pthread_t* threads;
	//bool* pid_proceed ;
	void update_id(){
		pthread_mutex_lock(&id_lock) ;
		++id ;
		pthread_mutex_unlock(&id_lock) ;
	}

	static void* run_dispatching(void* arg){
		thread_pool* tmp = reinterpret_cast<thread_pool*>(arg);
        tmp->dispatching(NULL);
        return 0;
	}


};

static void get_best(const list<task*>& tasks, double& current_best){
	//cout << "get_best" << endl ;
	int size = tasks.size() ;
	for(auto it = tasks.begin() ; it != tasks.end() ; ++it){
		vector<double>* tmp = (vector<double>*)((*it)->get_output()) ;
		if(tmp->at(3) < current_best){
			current_best = tmp->at(3) ;
			cout << "update: " << current_best << endl;
		}
	}
	return ; 
}

//double current_best = 0.01;

int main(int argc, char** argv){
	double current_best = 0.1;
	int NUM_THREADS = get_opt(argc, argv) ;
	//running_sum = vector<int>(NUM_THREADS, 0) ;
	thread_pool pool(NUM_THREADS) ;
	thread_info* input = new thread_info() ;
	//(1, 3); (10,4); (-4, 5)
    input->points.push_back(pair<int, int>(1, 3)) ;
    input->points.push_back(pair<int, int>(10, 4)) ;
    input->points.push_back(pair<int, int>(-4, 5)) ;
    input->current_best = current_best ;

    task* t1 = new task((void*)input, thread_start) ;
	task* t2 = new task((void*)input, thread_start) ;
	task* t3 = new task((void*)input, thread_start) ;
	task* t4 = new task((void*)input, thread_start) ;
	pool.add_task(t1) ;
	pool.add_task(t2) ;
	pool.add_task(t3) ;
	pool.add_task(t4) ;
	pool.thread_pool_run() ;

    do{
  		list<task*> tmp = pool.get_complete_task() ;
  		get_best(tmp, current_best) ;
    }while(abs(current_best) > 0.004) ;
	
    pool.thread_pool_stop() ;
    cout << "end:"  << current_best << endl;

	return 0 ;
}
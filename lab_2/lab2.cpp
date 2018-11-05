#include <unordered_map>
#include <vector>
#include <string>
//#include <list>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
//#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <random>
#include <cfloat>
#include <chrono>
#include <time.h>
#include <thread>
#include <deque> 
#include <condition_variable>
#include <mutex>

using namespace std ;

/***********************************global**********************************/
static double BENCHMARK = 0.1;
static double current_best = 10000 ;
static int NUM_THREADS = 1 ;
static int dimension = 3 ;
static double l_bound = -10.0 ;
static double up_bound = 10.0 ;


typedef void* (*thread_func_ptr)(void*) ;

typedef struct thread_info { 
    vector<pair<int, int> > points ;
    double current_best ;
}thread_info;

//get the command-line optional
static int get_opt(int argc, char** argv){
	int res = 1 ; 
    int opt ;
    while ((opt = getopt(argc,argv,"n:d:t:b:l:u:h")) != EOF){
    	switch(opt)
        {
            case 'n': 
                NUM_THREADS = stoi(optarg) ; 
                break;
            case 'd':
            	dimension = stoi(optarg) ; 
                break;
            case 't': 
            	BENCHMARK = stod(optarg, NULL) ;
            	break ; 
            case 'l':
            	l_bound = stod(optarg, NULL) ;
            	break ;
            case 'u':
            	up_bound = stod(optarg, NULL) ;
            	break ;
            case 'h':
            	cout << "-n ($int) to specify number of threads in threads pool; default: 1" << endl; 
            	cout << "-d ($int) to specify the dimension of the polynomial function; defult: 2" << endl ;
            	cout << "-t ($double) to specify the tolerance of the solution; default: 0.1" << endl ;
            	cout << "-l ($double) to specify the lower_bound of the solution; default: -10.0" << endl; 
            	cout << "-u ($double) to specify the upper_bound of the solution; default: 10.0" << endl ;
            	exit(0) ;
            	break ;
            default: 
            	cout << "The program will use the default setup, unspecified optional" << endl ;
            	break ;
        }
    }
	/*res = stoi(optarg) ; 
	if(res < 0){
		cout << "Number of thread should be greater than 0." << endl;
		abort() ;
	}else if(res == 0)
		return 1 ;
	else
    	return res ; */
    return 0 ;
}

//user-define job
static vector<double> find_coefficient(const vector<pair<int, int> >& points, double& recent_best){
	int size = points.size() ;
	vector<double> coefficients(size , 0.0f) ;
	double result ;
	double tmp ; 
	double cur ;
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	std::default_random_engine generator (seed);
  	uniform_real_distribution<double> distribution(l_bound, up_bound);

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
    }while(result > recent_best) ;
    recent_best = result ;
	coefficients.push_back(result) ;
	return coefficients ;
}

/*
wrapper function to invoke the real job: The return value will be set as the output of the current task and
the task will be pushed into "complete queue" for further retrieval.
*/
static void * thread_start(void *arg){
	thread_info* info = (thread_info*)arg ;;
	vector<double>* tmp = new vector<double>(find_coefficient(info->points, info->current_best)) ;
	return (void*)tmp ;	
}

//contain: input, job(function pointer), and output for generic purpose.
class task
{
public:
	task() ;

	task(void* _input, thread_func_ptr _job)
	{ 
		input = _input;
		job = _job;
	}

	task(const task& t)
	{	
		input = t.input; 
		job = t.job;
	}

	~task(){
		if(input) delete input ;
		if(output) delete output ;
	} ;

	const thread_func_ptr get_job(){return job ;}
	void* get_input(){return input ;}
	const void* get_output(){return output ;}
	const void set_output(void* out){output = out; } 
	const void set_id(const int& _id){id = _id ;} 
private:
	int id ;
	void* input = nullptr ;
	void* output = nullptr ;
	thread_func_ptr job ;
};

template<typename T>
class ThreadSafeListenerQueue2{
public:

	ThreadSafeListenerQueue2<T>(){
		pthread_cond_init(&cond, NULL) ;
	}

	deque<T> get_list(){
		pthread_mutex_lock(&lock) ;
		deque<T> copy ;
		for(auto it = q.begin() ; it != q.end() ; ++it){
			copy.push_back(*it) ;
		}
		pthread_mutex_unlock(&lock) ;
		return copy ;
	}

	~ThreadSafeListenerQueue2<T>(){}

	bool push(const T& element){
		pthread_mutex_lock(&lock) ;
		q.push_back(element) ;
		this->increment_count() ;
		pthread_cond_signal(&cond) ;
		pthread_mutex_unlock(&lock) ;
		return true ; 
	}
	bool pop(T& element){
		pthread_mutex_lock(&lock) ;
		if(q.empty()){
			pthread_mutex_unlock(&lock) ;
			return false ;
		}
		else{
			if(!q.empty()){
				if(!this->decrement_count()){
					cout << "pop: decrement_count fail, counter is zero" << endl ;
					pthread_mutex_unlock(&lock) ;
					return false ;
				}
				element = q.front() ;
				q.pop_front() ;
			}
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
		while(!counter)
			pthread_cond_wait(&cond, &lock) ;
		if(!q.empty()){
			if(!this->decrement_count()){
				cout << "listen: decrement_count fail, counter is zero" << endl ;
				exit(0) ;
			}
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

	bool empty(){
		pthread_mutex_lock(&lock) ;
		bool tmp = q.empty() ;
		pthread_mutex_unlock(&lock) ;
		return tmp ;
	}

	int size(){
		return q.size() ;
	}
private:
	deque<T> q ;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
	pthread_cond_t cond ;
	unsigned long long int counter = 0 ;
	bool increment_count(){
    	if(counter < ULLONG_MAX){
    		counter++ ;
    		return true ;
    	}
    	else{
    		return false ;
    	}
    }

    bool decrement_count(){
    	if(counter != 0){
    		counter-- ;
    		return true ;
    	}
    	else{
    		return false ;
    	}
    }
};


//user-define compare function: compare which output in the tasks is "better", and return the better one.
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
		if(threads)
			delete threads ;
	}

	task* thread_pool_aggregate(cmp my_cmp){
		int size = complete.size() ;
		task* res = nullptr ;
		if(size > 1){
			for(int i = 0 ; i < size-1 ; ++i){

				task* t1 ;
				if(!complete.pop(t1))
				{ 
					return res ;
				}
				if(!complete.pop(res)){  
					int tmp_s = ((vector<double>*)(t1->get_output()))->size() ;
					cout << "t1: "<< ((vector<double>*)(t1->get_output()))->at(tmp_s-1) << endl ;
					return t1;
				}
				res = my_cmp(t1, res) ;
				complete.push(res) ;
			}
		}
		return res; 
	}

	task* thread_pool_aggregate_at_least(cmp my_cmp, int batch_volume, timespec* estimate_time_per_task){
		estimate_time_per_task->tv_sec *= batch_volume ;
		estimate_time_per_task->tv_nsec *= batch_volume ;
		nanosleep(estimate_time_per_task, NULL) ; //in order to successfully execute the batch.
		task* res = nullptr ;
		int size = complete.size() ;
		if(size > batch_volume){
			for(int i = 0 ; i < size-1 ; ++i){
				task* t1 ;
				if(!complete.pop(t1))
				{ 
					return res ;
				}
				if(!complete.pop(res)){ 
					complete.push(t1) ;
					return t1;
				}
				res = my_cmp(t1, res) ;
				complete.push(res) ;
			}
		}
		return res ;
	}

	//1.wait for the job, 2.execute the job, 3. put the result into task->output, 4. push into complete for aggregating. 
	void* dispatching(void* arg){
		void* tmp ;
		while(proceed){
			task* t = nullptr ;
			pending.listen(t) ;
			thread_func_ptr job = t->get_job() ;
			tmp = job(t->get_input()) ;
			t->set_output(tmp) ;
			complete.push(t) ;
		}
		return NULL ;
	}

	void add_task(task* t){
		pending.push(t) ;
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

	deque<task*> get_complete_task(){
		return complete.get_list() ;
	}
private:
	ThreadSafeListenerQueue2<task*> pending ;
	ThreadSafeListenerQueue2<task*> complete ;
	int thread_num  = 1 ;
	pthread_mutex_t continue_lock = PTHREAD_MUTEX_INITIALIZER ;
	bool proceed = true ;
	pthread_t* threads = nullptr ;

	static void* run_dispatching(void* arg){
		thread_pool* tmp = reinterpret_cast<thread_pool*>(arg);
        tmp->dispatching(NULL);
        return 0;
	}


};

//user-defined get best: retrieve the current best result;
static void get_best(task* t, double& current_best){
	if(t == NULL){
		return ; 
	}
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

//make initial input for the following input.
thread_info* make_input(const int& dimension, const int& current_best){
	thread_info* input = new thread_info() ;
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	std::default_random_engine generator (seed);
  	uniform_real_distribution<int> distribution(-1, 1);
	for(int i = 0 ; i < dimension ; ++i){
		int x = distribution(generator)%10;
		int y = distribution(generator)%10;
	    input->points.push_back(pair<int, int>(x, y)) ;
	}
	input->current_best = current_best ;
	return input ;
}

int main(int argc, char** argv){
	//get optional from command
	get_opt(argc, argv) ;
	//initialize input.
	thread_info* input = make_input(dimension, current_best) ;
	//print out all user-defined variables.
	cout << "number of threads: "<< NUM_THREADS << endl;  
	cout << "Dimension: " << dimension << endl ;
	cout << "Benchmark: " << BENCHMARK << endl ; 
	cout << "Lower_bound: " << l_bound << endl ; 
	cout << "upper_bound: " << up_bound << endl ;
	
	thread_pool pool(NUM_THREADS) ;
	
    for(int i = 0 ; i < 100 ; ++i){
    	//input initialize:
    	thread_info* tmp = new thread_info(); 
    	tmp->points = input->points ;
    	tmp->current_best = current_best ;
	    //task initialize:
	    pool.add_task(new task((void*)tmp, thread_start)) ;
    }

    //fire all the thread in the thread pool
	pool.thread_pool_run() ;

	//main thread keeps updating until it meet the requirement. 
	timespec* time_rest = new timespec() ; 
	time_rest->tv_sec = 0;
	time_rest->tv_nsec = 11111111 ;

    do{
  		//task* tmp = pool.thread_pool_aggregate_at_least(my_cmp, 20, time_rest) ;
  		task* tmp = pool.thread_pool_aggregate(my_cmp) ;
  		get_best(tmp, current_best) ;
  		thread_info* in = new thread_info() ;
  		in->points = input->points ;
  		in->current_best = current_best ;
  		pool.add_task(new task((void*)in, thread_start)) ;
    }while(abs(current_best) > BENCHMARK) ;
	
    pool.thread_pool_stop() ;
    cout << "main end:"  << current_best << endl;
	
	return 0 ;
}
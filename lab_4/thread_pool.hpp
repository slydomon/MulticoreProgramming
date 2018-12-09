
#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <map>
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
#include <iostream>
#include <ctime>
#include <ratio>
#include <climits>
#include "maze.hpp"



//#define stat ;
#define verbose ;
//#define debug ;

typedef void* (*thread_func_ptr)(void*) ;
typedef enum DIR { STAY = 0, UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4} ;

class genome
{
public:
    genome() ;
    genome(const genome& input) ;
    ~genome(){} ; 
    inline std::vector<unsigned> get_genome() const {return gen ;}
    void set_genome(const std::vector<unsigned>& input) ;
    genome mix(const genome& mate) ;
    genome mutate() ;
    genome compact() ;
    unsigned count_fitness(Maze* maze) ;
    void count_fitness_debug(Maze* maze) const ;
    static inline void set_genome_len(const int& len){gen_size = len ;}
    //inline void set_genome_len(const int& len){gen_size = len ;}
private:
    static unsigned gen_size  ; //need to be able to modified.
    //unsigned gen_size = 20 ;
    std::vector<unsigned> gen ;
};

inline unsigned genome::gen_size = 20 ;

class task
{
public:
	task() ;
	task(void* _input, thread_func_ptr _job) ;
	task(const task& t) ; //copy constructor
	~task() ;
	inline const thread_func_ptr get_job(){return job ;}
	inline void* get_input(){return input ;}
	inline const void* get_output(){return output ;}
	inline const void set_output(void* out){output = out; } 
	inline const void set_id(const int& _id){id = _id ;} 
private:
	int id ;
	void* input = nullptr ;
	void* output = nullptr ;
	thread_func_ptr job ;
};

template<typename T>
class ThreadSafeListenerQueue2{
public:
	ThreadSafeListenerQueue2<T>() {pthread_cond_init(&cond, NULL) ;};
	~ThreadSafeListenerQueue2<T>(){} ;
    std::deque<T> get_list()
    {
        pthread_mutex_lock(&lock) ;
        std::deque<T> copy ;
        for(auto it = q.begin() ; it != q.end() ; ++it){
            copy.push_back(*it) ;
        }
        pthread_mutex_unlock(&lock) ;
        return copy ;
    }

    bool push(const T& element)
    {
        pthread_mutex_lock(&lock) ;
        q.push_back(element) ;
        //q.push_back(std::move(element)) ;
        this->increment_count() ;
        pthread_cond_signal(&cond) ;
        pthread_mutex_unlock(&lock) ;
        return true ; 
    }

    bool pop(T& element)
    {
        pthread_mutex_lock(&lock) ;
        if(q.empty()){
            pthread_mutex_unlock(&lock) ;
            return false ;
        }
        else{
            if(!q.empty()){
                if(!this->decrement_count()){
                    std::cout << "pop: decrement_count fail, counter is zero" << std::endl ;
                    pthread_mutex_unlock(&lock) ;
                    return false ;
                }
                element = q.front() ;
                //element = std::move(q.front()) ;
                q.pop_front() ;
            }
            else{
                std::cout << "pop: pop fail: queue is empty" << std::endl ;
                exit(0) ;
            }
            pthread_mutex_unlock(&lock) ;
            return true ;
        }
    }

    bool listen(T& element)
    {
        #ifdef debug
           //std::cout << "wait for offspring " << std::endl ;
        #endif
        pthread_mutex_lock(&lock) ;
        while(!counter)
            pthread_cond_wait(&cond, &lock) ;
        if(!q.empty()){
            if(!this->decrement_count()){
                std::cout << "listen: decrement_count fail, counter is zero" << std::endl ;
                exit(0) ;
            }
            element = q.front();
            //element = std::move(q.front());
            q.pop_front() ;
        }
        else{
            std::cout << "Listen: pop fail: queue is empty" << std::endl ;
            exit(0) ;
        }
        pthread_mutex_unlock(&lock) ;
        return true ;
    }

    bool empty()
    {
        pthread_mutex_lock(&lock) ;
        bool tmp = q.empty() ;
        pthread_mutex_unlock(&lock) ;
        return tmp ;
    }

    inline int size(){return q.size() ;}
private:
    std::deque<T> q ;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
    pthread_cond_t cond ;
    unsigned long long int counter = 0 ;
    bool increment_count()
    {
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

template<typename K, typename V>
class ThreadSafeKVStore{
public:
    /*void init_mutex(const K& key)
    {
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
    }
    */

    bool insert(const K& key, const V& value)
    {
        //init_mutex(key) ;
        //pthread_mutex_lock(&map_mutex[key]) ;
        pthread_mutex_lock(&lock) ;
        if(map.find(key) == map.end()){
            map[key] = std::vector<V>(1, value);
            //delete map[key] ;
            ////map[key] = value ;

        }
        else
        {
            /*if(map.size() > 10){
                int size = map[key].size() ;
                delete map[key][size-1] ;
                map[key].pop_back() ;
            }*/
            //if(map[key].size() > 30) map[key].pop_back() ;
            map[key].push_back(value) ;
            //delete map[key] ; 
            ///map[key] = value ;
            ///delete value ;
        }
        pthread_mutex_unlock(&lock) ;
        //pthread_mutex_unlock(&map_mutex[key]) ;
        return true ;
    } ;

    /*bool accumulate(const K& key, const V& value)
    {
        init_mutex(key) ;
        pthread_mutex_lock(&map_mutex[key]) ;
        if(map.find(key) == map.end())
            map[key] = value ;
        else
            map[key] = map[key] + value ;
        pthread_mutex_unlock(&map_mutex[key]) ;
        return true ;
    } ;*/

    V operator[](const unsigned& index)
    {
        int size = map.size() ;
        if(index > size){
            #ifdef debug
                std::cout << "operator[] fail: index is greater than map size; size: "<< size << " index: "<< index << std::endl ;
            #endif
            abort() ;
        }
        auto it = map.begin() ;
        for(int i = 0 ; i < index && it != map.end() ; ++i) ++it ;
        K key = it->first ;
        //pthread_mutex_lock(&map_mutex[key]) ;
        pthread_mutex_lock(&lock);
        /*critical section*/
        V tmp ;
        if(map.find(key) == map.end())
        {
            //pthread_mutex_unlock(&map_mutex[key]) ;
            //pthread_mutex_unlock(&lock) ;
            #ifdef debug
                std::cout << "operator[] fail: givn index < size, still cannot find value ; size: "<< size << " key: "<< key<< std::endl ;
            #endif
            //tmp = ((map.begin())->second)[0] ;
            //abort() ; maybe truncate right before operator[]
        }
        else{
            //choose a random one;
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	        std::default_random_engine generator (seed);
            std::uniform_int_distribution<int> distribution(0, INT_MAX);
            int select = distribution(generator) % map[key].size() ;
            tmp = map[key][select] ;
            ///tmp = map[key] ;
            //tmp = std::move(map[key][select]) ;
        }
        /******************/
        pthread_mutex_unlock(&lock) ;
        //pthread_mutex_unlock(&map_mutex[key]) ;
        #ifdef debug 
            /*if(!tmp)
            {
                std::cout << "operator[] fail: tmp is null; select: "<< select << std::endl; 
                abort() ;
            }*/
        #endif
        return tmp ;
    }

    bool truncate(const unsigned& i)
    {   
        pthread_mutex_lock(&lock) ;
        auto it = map.begin();
        //K key_1 = it->first ;  
        /*critical section*/
        if(it == map.end())
        {
            #ifdef debug
                //std::cout << "remove key pass: " << key << std::endl ;
            #endif
            //pthread_mutex_unlock(&map_mutex[key]) ;
            pthread_mutex_unlock(&lock) ;
            return true ;
        }
        else
        {
            for(int k = 0 ; k < i && it != map.end(); ++k) ++it ;
            //int size = map[key].size() ;
            if(it != map.end())
                map.erase(it, map.end()) ;
            /*for(;it!= map.end(); ++it)
            {
                //std::cout << "map val:" << it->first << std::endl ;
                if(it != map.end()) map.erase(it) ;
                    //std::cout << "illeagl" << std::endl ;
            }*/
            ///delete map[key] ;
            #ifdef debug
                //std::cout << "remove key: " << key << std::endl ;
            #endif
            //map.erase(key) ;
        }          
        /******************/
        //pthread_mutex_unlock(&map_mutex[key]) ;
        pthread_mutex_unlock(&lock) ;
        return true  ;    
    }

    
    bool lookup(const K& key, V& value)
    {

        //check if the mutex of the key exist
        //if(map_mutex.find(key) == map_mutex.end())
        //    return true ;

        //pthread_mutex_lock(&map_mutex[key]) ;
        pthread_mutex_lock(&lock) ;
        /*critical section*/
        if(map.find(key) == map.end()){
            pthread_mutex_unlock(&lock) ;
            //pthread_mutex_unlock(&map_mutex[key]) ;
            return false ; //no longer there
            
        }
        else
            value = map[key] ;
        /******************/
        pthread_mutex_unlock(&lock) ;
        //pthread_mutex_unlock(&map_mutex[key]) ;
        return true ;
        //}
    } ;
    
    bool remove(const K& key)
    {
        //return true when operation succeed.
        //return false if certain invariants do not hold.

        //check if the mutex of the key exist
        //if(map_mutex.find(key) == map_mutex.end())
        //    return true ;
        //pthread_mutex_lock(&map_mutex[key]) ;
        pthread_mutex_lock(&lock) ;
        /*critical section*/
        if(map.find(key) == map.end())
        {
            #ifdef debug
                //std::cout << "remove key pass: " << key << std::endl ;
            #endif
            //pthread_mutex_unlock(&map_mutex[key]) ;
            pthread_mutex_unlock(&lock) ;
            return true ;
        }
        else
        {
            int size = map[key].size() ;
            for(int i = 0 ; i < size ; ++i)
            {
                //std::cout << "delete" << std::endl ;
                //map[key][i].~genome() ;
            }
            ///delete map[key] ;
            #ifdef debug
                //std::cout << "remove key: " << key << std::endl ;
            #endif
            map.erase(key) ;
        }          
        /******************/
        //pthread_mutex_unlock(&map_mutex[key]) ;
        pthread_mutex_unlock(&lock) ;
        return true ;
    }

    void print_map()
    {
        for(auto it = map.begin() ; it != map.end() ; ++it)
        std::cout << "key: " << it->first << " val: " << it->second << std::endl ; 
    }

    int return_sum()
    {
        auto begin = map.begin() ;
        auto end = map.end() ;
        int acc_sum = 0 ;
        int running_sum_ = 0 ;
        for(auto it = begin ; it != end ; ++it){
            acc_sum += it->second ;
        }
        return acc_sum ;
    }
    
    inline int size(){return map.size() ;}
private:
	std::map<K, std::vector<V> > map ;
    ///std::map<K, V> map ;
	//std::map<K, pthread_mutex_t> map_mutex ; //should always exist rather than delete with map
	//pthread_mutex_t u_lock = PTHREAD_MUTEX_INITIALIZER ; //utility lock
    //pthread_mutex_t it_lock = PTHREAD_MUTEX_INITIALIZER ; //iterator lock
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
};


template<>
inline genome ThreadSafeKVStore<unsigned,genome>::operator [] (const unsigned& index)
{
    int size = map.size() ;
    if(index > size){
        #ifdef debug
            //std::cout << "operator[] fail: index is greater than map size; size: "<< size << " index: "<< index << std::endl ;
        #endif
        //abort() ;
    }
    /*auto it = map.begin() ;
    for(int i = 0 ; i < index && it != map.end() ; ++i) ++it ;
    unsigned key = it->first ;*/
    //pthread_mutex_lock(&map_mutex[key]) ;
    pthread_mutex_lock(&lock);
    auto it = map.begin() ;
    for(int i = 0 ; i < index && it != map.end() ; ++i) ++it ;
    unsigned key = it->first ;
    ///critical section
    genome tmp ;
    if(map.find(key) == map.end())
    {
        //pthread_mutex_unlock(&map_mutex[key]) ;
        //pthread_mutex_unlock(&lock) ;
        #ifdef debug
            // std::cout << "operator[] fail: givn index < size, still cannot find value ; size: "<< size << " key: "<< key<< std::endl ;
        #endif
        //tmp = ((map.begin())->second)[0] ;
        //abort() ; maybe truncate right before operator[]
    }
    else{
        //choose a random one;
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine generator (seed);
        std::uniform_int_distribution<int> distribution(0, INT_MAX);
        int select = distribution(generator) % map[key].size() ;
        tmp = map[key][select] ;
        ///tmp = map[key] ;
        //tmp = std::move(map[key][select]) ;
    }

    pthread_mutex_unlock(&lock) ;
    //pthread_mutex_unlock(&map_mutex[key]) ;
    #ifdef debug 
        //if(!tmp)
        //{
        //    std::cout << "operator[] fail: tmp is null; select: "<< select << std::endl; 
        //    abort() ;
        //}
    #endif
    #ifdef debug
    if(tmp.get_genome().empty())
        std::cout << "tmp does not possess any gene" << std::endl;
    #endif
    return tmp ;
}


class thread_pool
{    
typedef void* (thread_pool::*func_ptr)(void*) ;
typedef task* (*cmp)(task* t1, task* t2) ;
public:
    thread_pool(int thread_number) ;
    ~thread_pool() ;
    task* thread_pool_aggregate(cmp my_cmp) ;
    task* thread_pool_aggregate_at_least(cmp my_cmp, int batch_volume, 
        timespec* estimate_time_per_task, double current_best, double init_best) ;

    //1.wait for the job, 2.execute the job, 3. put the result into task->output, 4. push into complete for aggregating. 
    void* dispatching(void* arg) ;
    inline void add_task(task* t){ pending.push(t) ;}
    //if ture, task is dispatched ; if false, there is no task in pending 
    void dispatch_task(int pid) ;
    void thread_pool_run() ;
    void thread_pool_stop() ;
    inline std::deque<task*> get_complete_task(){return complete.get_list();}

    #ifdef stat
        inline int get_guess_counter(){return guess_counter ;}
        void* dispatching_stat(void* arg) ;
        //return 0 is min, 1 is max, 2 is avg;
        std::vector<double> calculate_time_stat();
    #endif

    bool stop_add_task() ;
    #ifdef debug
        int pending_size(){return pending.size() ;}
    #endif
private:
    ThreadSafeListenerQueue2<task*> pending ;
    ThreadSafeListenerQueue2<task*> complete ;
    int thread_num  = 1 ;
    pthread_mutex_t continue_lock = PTHREAD_MUTEX_INITIALIZER ;
    bool proceed = true ;
    pthread_t* threads = nullptr ;

    static void* run_dispatching(void* arg){
        thread_pool* tmp = reinterpret_cast<thread_pool*>(arg);
        #ifdef stat 
            tmp->dispatching_stat(NULL) ;
        #else
            tmp->dispatching(NULL);
        #endif
        return 0;
    }

    //specifically for calculating stat ;
    #ifdef stat 
        int guess_counter = 0 ;
        pthread_mutex_t guess_counter_lock = PTHREAD_MUTEX_INITIALIZER ;
        ThreadSafeListenerQueue2<double> job_exec_time ;
    #endif
    };


#endif
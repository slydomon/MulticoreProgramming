
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
#define debug ;

static unsigned int ROW = 7 ;
static unsigned int COL = 7 ;
typedef void* (*thread_func_ptr)(void*) ;
typedef enum DIR { STAY = 0, UP, DOWN, LEFT, RIGHT} ;

class genome
{
public:
    genome(){
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	    std::default_random_engine generator (seed);
        std::uniform_int_distribution<int> distribution(0, INT_MAX);
        gen = std::vector<unsigned>(gen_size, 0) ;
        for(int i = 0 ; i < gen_size ; ++i){
            gen[i] = distribution(generator)%5;
        }
    }
    genome(const genome& input){
        gen = input.gen ;
    }

    ~genome()
    {
        
    } 

    inline std::vector<unsigned> get_genome() const {return gen ;}

    void set_genome(const std::vector<unsigned>& input)
    {
        for(int i = 0 ; i < gen_size ; ++i){
            if(input[i] > 4) gen[i] = input[i]%5 ; //make sure value is within 0-4
            else gen[i] = input[i] ;
        }
    }

    /*genome* mix(const genome& mate)
    //unique_ptr<genome> mix(const genome& mate)
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	    std::default_random_engine generator (seed);
        uniform_int_distribution<int> distribution(0, INT_MAX);
        int slice_index = distribution(generator) % gen_size ;
        int s ; 
        if(slice_index >= gen_size-1) s = gen_size - 2 ;
        else if(slice_index == 0) s = 1 ;
        else s = slice_index ;
        //first part of gen derives from caller and last part derive mate
        genome* res = new genome() ;
        //unique_ptr<genome> res(new genome) ;
        vector<unsigned> tmp = mate.get_genome() ;
        for(int i = 0 ; i < s ; ++i) tmp[i] = gen[i] ;
        res->set_genome(tmp) ;
        return res; 
    }*/

    genome mix(const genome& mate)
    //unique_ptr<genome> mix(const genome& mate)
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	    std::default_random_engine generator (seed);
        std::uniform_int_distribution<int> distribution(0, INT_MAX);
        int slice_index = distribution(generator) % gen_size ;
        int s ; 
        if(slice_index >= gen_size-1) s = gen_size - 2 ;
        else if(slice_index == 0) s = 1 ;
        else s = slice_index ;
        //first part of gen derives from caller and last part derive mate
        genome res ;
        //unique_ptr<genome> res(new genome) ;
        std::vector<unsigned> tmp = mate.get_genome() ;
        for(int i = 0 ; i < s ; ++i) tmp[i] = gen[i] ;
        res.set_genome(tmp) ;
        return res; 
    }

    //if not mutate, return null to avoid create "clone"
    /*
    genome* mutate()
    //unique_ptr<genome> mutate()
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	    std::default_random_engine generator (seed);
        uniform_int_distribution<int> distribution(0, INT_MAX);
        int mut = distribution(generator) ;
        int check = mut % 101 ; //for counting percentage
        #ifdef debug
            if(check < 0){
                cout << "mutate function mut error: check=" << check << endl; 
                abort() ;
            }
        #endif
        if(check > 40)
        { 

            return new unmutated(*this);
        }
        else{
            int pos = mut % gen_size ;
            int gene = mut % 5 ;
            #ifdef debug
                if(pos < 0){
                    cout << "mutate function mut error: pos=" << pos << endl; 
                    abort() ;
                }
                else if(gene < 0 ){
                    cout << "mutate function mut error: gene=" << gene << endl; 
                    abort() ;  
                }
            #endif
            unsigned int tmp = gen[pos] ;
            this->gen[pos] = gene ;
            genome* mutated = new genome(*this) ;
            //genome* mutated = new genome() ;
            //mutated.set_genome(this->get_genome()) ;
            //unique_ptr<genome> mutated(new genome(*this)) ;
            this->gen[pos] = tmp ;
            return mutated ;
        }
    }
    */

    genome mutate()
    //unique_ptr<genome> mutate()
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  	    std::default_random_engine generator (seed);
        std::uniform_int_distribution<int> distribution(0, INT_MAX);
        int mut = distribution(generator) ;
        int check = mut % 101 ; //for counting percentage
        #ifdef debug
            if(check < 0){
                std::cout << "mutate function mut error: check=" << check << std::endl; 
                abort() ;
            }
        #endif
        if(check > 40)
        { 
            /*genome* tmp = new genome() ;
            tmp->set_genome(this->get_genome()) ;
            return tmp ;*/
            genome unmutated ;
            unmutated.set_genome(this->get_genome()) ;
            return unmutated;
        }
        else{
            int pos = mut % gen_size ;
            int gene = mut % 5 ;
            #ifdef debug
                if(pos < 0){
                    std::cout << "mutate function mut error: pos=" << pos << std::endl; 
                    abort() ;
                }
                else if(gene < 0 ){
                    std::cout << "mutate function mut error: gene=" << gene << std::endl; 
                    abort() ;  
                }
            #endif
            unsigned int tmp = gen[pos] ;
            this->gen[pos] = gene ;
            genome mutated ;
            //genome* mutated = new genome() ;
            mutated.set_genome(this->get_genome()) ;
            //unique_ptr<genome> mutated(new genome(*this)) ;
            this->gen[pos] = tmp ;
            return mutated ;
        }
    }

    unsigned count_fitness(Maze* maze){
        //const bool get(const size_t row, const size_t col) const;
	    //const Coord getStart();
	    //const Coord getFinish();
        //start from (0, 0) ;
        Coord cur = maze->getStart() ;
        Coord end = maze->getFinish() ;
        unsigned wall = 0 ;
        unsigned dist = 0 ;
        for(int i = 0 ; i < gen_size ; ++i){
            switch(gen[i])
            {
                case STAY:
                    break ; //if start is end, you need to stay to become the fitness
                case LEFT : 
                    if(cur.col == 0 || maze->get(cur.row, cur.col-1)) ++wall ;
                    else --cur.col ;
                    break ;
                case RIGHT:
                    if(cur.col == COL-1 || maze->get(cur.row, cur.col+1)) ++wall ;
                    else ++cur.col ;
                    break;
                case UP:
                    if(cur.row == 0 || maze->get(cur.row-1, cur.col)) ++wall ;
                    else --cur.row ;
                    break ;
                case DOWN:
                    if(cur.row == ROW-1 || maze->get(cur.row + 1, cur.col)) ++wall ;
                    else ++cur.row ;
                    break;
            }
            //check if reach the end point, if so return 0(the fitness)
            if(cur.col == end.col && cur.row == end.row)
                return 0 ;
        }
        dist = abs(static_cast<int>(cur.col - end.col)) + abs(static_cast<int>(cur.row - end.row)) ;
        if(dist == 0)
            return 0 ;
        else
            return 2*dist + wall ;
    }

private:
    //unsigned gen_size = GENO_LEN ; //depends on global, not good.
    unsigned gen_size = 20 ;
    std::vector<unsigned> gen ;
};

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

    bool truncate(unsigned i)
    {
        int size = map.size() ;
        if(size < i || i == 0){
            #ifdef debug
                //std::cout << "truncate function: did not truncate: size:" << size << " index:"<< i << std::endl ;
            #endif
            return false ;
        }
        else{
            #ifdef debug
                //std::cout << "truncate: index:" << i << std::endl;
            #endif
            //pthread_mutex_lock(&it_lock) ;
            auto it = map.begin() ;
            //auto end = map.end() ;
            //for(; i > 0 && it != map.end() ; --i) ++it ; //iterate to the actual truncate start point.
            //for(; it != map.end(); ++it)
            //{
                K key = it->first ;
                #ifdef debug
                    //std::cout << "************** start remove*****************" << std::endl ;
                #endif
                //pthread_mutex_lock(&map_mutex[key]) ; 
                pthread_mutex_lock(&lock) ;
                #ifdef debug
                    //std::cout << "************** start remove: get lock*****************" << std::endl ;
                #endif
                //int size = it->second.size() ;
                /*for(int k = size-1 ; k > i && k > 0 ; --k)
                {
                    //if(!it->second.empty() && it->second[k] != nullptr)
                    //{
                    //    delete it->second[k] ;
                    //    it->second[k] = nullptr ;
                    //    it->second.pop_back() ;
                        //it->second.erase(k);
                    //}
                    if(!it->second.empty())
                        it->second.pop_back() ;
                }*/
                /*if(!remove(it->first)){
                    #ifdef debug
                        std::cout << "truncate function: truncate fail during truncating" << std::endl ;
                    #endif
                    abort() ;
                }
                else
                {
                    #ifdef debug
                        //std::cout << "**************successfully remove*****************" << std::endl ;
                    #endif
                }*/
                #ifdef debug
                    //std::cout << "successfully remove: key: "<< key << std::endl ;
                #endif
                //map.erase(it++) ;
                //map.erase(it++) ;
                map.erase(it) ;
                
                //pthread_mutex_unlock(&map_mutex[key]) ; 
                pthread_mutex_unlock(&lock) ;
            //}
            //pthread_mutex_unlock(&it_lock) ;
            return true ;
        }
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
                std::cout << "operator[] fail: index is greater than map size; size: "<< size << " index: "<< index << std::endl ;
            #endif
            abort() ;
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
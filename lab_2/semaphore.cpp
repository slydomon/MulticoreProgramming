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
//#include <thread>
#include <condition_variable>
#include <mutex>

using namespace std ;

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

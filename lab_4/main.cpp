#include "thread_pool.hpp"
#include <string>
#include <unordered_set>

using namespace std ;

#define stat ;
#define verbose ;
//#define debug ;

/***********************************global**********************************/
static unsigned int NUM_THREADS = 1 ;
static long long unsigned int MUT_THRESHOLD = 1000000 ; //1,000,000
static unsigned int ROW = 7;
static unsigned int COL = 7;
static unsigned int GENO_LEN = 20 ;
static unsigned int TRUNCATE_CONSTANT = 4 ;
static unsigned int OFFSPRING_LIMIT = 10000 ;
static long long unsigned int counter = 0 ;
static unsigned int mut_counter = 0 ;
static unsigned int mix_counter = 0 ;
static unsigned int current_fitness = INT_MAX ;
static vector<unsigned> best_genome ;
static Maze* maze ;
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER ;
static pthread_mutex_t mix_counter_lock = PTHREAD_MUTEX_INITIALIZER ;

typedef void* (*thread_func_ptr)(void*) ;

static ThreadSafeKVStore<unsigned, genome> population ;
static ThreadSafeListenerQueue2<genome> offspring ;
static unordered_set<string> gene_bank ;

//get the command-line optional
/*./lab4 <number of total threads> <threshold g for determining
completion> <rows> <cols> <genome length>*/
static int get_opt(int argc, char** argv)
{
	int res = 1 ; 
    int opt ;
    while ((opt = getopt(argc,argv,"n:g:r:c:l:h")) != EOF){
    	switch(opt)
        {
            case 'n': 
                NUM_THREADS = stoi(optarg) ; 
                break;
            case 't':
            	MUT_THRESHOLD = stoi(optarg) ; 
                break;
            case 'r': 
            	ROW = stod(optarg, NULL) ;
            	break ; 
            case 'c':
            	COL = stod(optarg, NULL) ;
            	break ;
            case 'l':
            	GENO_LEN = stod(optarg, NULL) ;
            	break ;
            case 'h':
            	cout << "-n ($int) to specify number of threads in threads pool; default: 1" << endl; 
            	cout << "-t ($int) to specify the threshold of the number of mutation; defult: 1,000,000" << endl ;
            	cout << "-r ($int) to specify the number of row of the maze; default: 7" << endl ;
            	cout << "-c ($int) to specify the number of column of the maze; default: 7" << endl; 
            	cout << "-l ($int) to specify the genome length; default: 20" << endl ;
            	exit(0) ;
            	break ;
            default: 
            	cout << "The program will use the default setup, unspecified optional" << endl ;
            	break ;
        }
    }
    return 0 ;
}

string show_genome(const vector<unsigned>& gen)
{
    string tmp = "" ;
    for(auto it = gen.begin(); it != gen.end() ; ++it)  tmp = tmp + to_string(*it) ;
    return tmp;
}

static void counter_decrease_and_check(const int& fitness, const genome& mut)
{
    pthread_mutex_lock(&counter_lock) ;
    ++counter ;
    #ifdef debug
        ++mut_counter ;
        if(mut_counter > 100000)
        {
            cout << "******************mutator responds heartbeat.********************" << endl ;
            mut_counter = 0 ;
        }
    #endif
    string gene = show_genome(mut.get_genome()) ;
    //gene_bank.insert(gene) ;
    if(current_fitness > fitness)
    {    
        current_fitness = fitness ;
        best_genome = mut.get_genome() ;
        counter = 0 ;
        #ifdef verbose
            #ifdef debug
                mut.count_fitness_debug(maze) ;
            #endif
            cout <<
            "*******************************Update******************************" << endl << 
            " Current fitness: " << current_fitness << endl <<
            " Genome: " << gene << endl <<
            " Generation: " << counter << endl <<
            " Offspring_size: " << offspring.size() << endl <<
            " popliation_size: " << population.size() << endl <<
            "*******************************************************************" << endl;
        #endif
    }
    if(counter > MUT_THRESHOLD)
    {
        #ifdef verbose
            cout << "Reach the generation threshold" << endl ;
            cout << "Genome: " << show_genome(best_genome) << endl ;
            cout << "Current best fitness: " << current_fitness << endl;
        #endif
    }   
    pthread_mutex_unlock(&counter_lock) ;
}

//return a genome pointer
static void * thread_func_wrapper_mixer(void *arg)
{
    string gene ;
    genome tmp ; 
    #ifdef debug
        pthread_mutex_lock(&mix_counter_lock) ;
        ++mut_counter ;
        if(mut_counter > 100000)
        {
            cout << "*************mixer responds heartbeat.**************" << endl ;
            mut_counter = 0 ;
        }
        pthread_mutex_unlock(&mix_counter_lock) ;
    #endif
    do{
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine generator (seed);
        uniform_int_distribution<int> distribution(0, INT_MAX);
        int size = population.size() ;
        int select_1 = distribution(generator) % size ;
        int select_2 = distribution(generator) % size ;
        //issue: father or mother might be empty ;
        genome father = population[select_1] ;
        genome mother = population[select_2] ;
        tmp = father.mix(mother) ;
        gene = show_genome(tmp.get_genome()) ;
    }
    while(gene_bank.find(gene) != gene_bank.end()) ;
    offspring.push(tmp) ;
    return nullptr;
}

//return a genome pointer
static void * thread_func_wrapper_mutator(void *arg)
{
    genome select ;
    if(!offspring.listen(select)){
        cout << "offspring listen fail: return fasle" << endl;
        exit(0) ;
    }
    string gene ;
    genome mut ;
    do{
        mut = select.mutate() ;
        gene = show_genome(mut.get_genome()) ;
    }
    while(gene_bank.find(gene) != gene_bank.end()) ;

    unsigned int fitness = mut.count_fitness(maze) ;
    counter_decrease_and_check(fitness, mut) ;
    //genome compact = mut.compact() ;
    //population.insert(fitness, compact) ;
    population.insert(fitness, mut) ;
    population.truncate(TRUNCATE_CONSTANT) ;
    return nullptr;
}
static bool check_has_solution(const Coord& start, const Coord& end, const int& gen_len = GENO_LEN)
{
    int dist = abs(static_cast<int>(start.row - end.row)) - abs(static_cast<int>(start.col-end.col)) ;
    if(dist > gen_len)
        return false ; //impossible to have solution
    else
        return true ;
}
/************************************************************************************************/




typedef struct thread_input_wrapper_mixer { 
    genome* father ;
    genome* mother ;
    int fitness ;
}mixer_input ;

typedef struct thread_input_wrapper_mutator { 
    genome* cur_fitness ;
    int fitness ;
}mutator_input ;



int main(int argc, char** argv){
	//get optional from command
	get_opt(argc, argv) ;

	//initialize input.
	//print out all user-defined variables.
	#ifdef verbose
		cout << "Number of threads: "<< NUM_THREADS << endl;  
		cout << "MUT_THRESHOLD: " << MUT_THRESHOLD << endl ;
		cout << "ROW: " << ROW << endl ; 
		cout << "COL: " << COL << endl ; 
		cout << "GENOME_LEN: " << GENO_LEN << endl ;
		cout << "**********************start calculating**********************"<<endl ;
	#endif

    //start count time
	#ifdef stat
		std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>>  start_time = std::chrono::steady_clock::now();
	#endif

    //init TRUNCATE_CONSTANT
    TRUNCATE_CONSTANT = NUM_THREADS * 4 ;
    int mixer_threads = 3 ;
    int mutate_threads = 5 ;
    if(mixer_threads < 1) mixer_threads = 1 ;
    if(mutate_threads < 1) mutate_threads = 1 ;
    #ifdef debug
        cout << "mixer: " << mixer_threads <<" mutate: " << mutate_threads << endl ;
    #endif
    genome::set_genome_len(GENO_LEN) ;

    //init maze
    maze = new Maze(ROW,COL) ;

    #ifdef verbose
        cout << *maze << endl ; 
        cout << "start point: (" << maze->getStart().col <<" , " << maze->getStart().row <<")" <<endl;
        cout << "end point: (" << maze->getFinish().col <<" , " << maze->getFinish().row <<")" <<endl;
    #endif

    //check if the gen_len can lead a solution
    if(!check_has_solution(maze->getStart(), maze->getFinish()))
    {
        cout << "Warning: There is no way to reach the end point because gen_len is not large enough!!" << endl ;
        //abort() ;
    }

    thread_pool pool_mix(mixer_threads) ;
    thread_pool pool_mutate(mutate_threads) ;

    //init population
    int size = NUM_THREADS * 4 ;
    unsigned int fitness ;

    #ifdef debug
        cout << "mix_pool pending: " << pool_mix.pending_size() <<" mutate_pool pending: " << pool_mutate.pending_size() <<endl ; 
    #endif

    for(int i = 0 ; i < size ; ++i)
    {
        genome tmp ;
        fitness = tmp.count_fitness(maze) ;
        if(current_fitness > fitness)
            current_fitness = fitness ;
        population.insert(fitness, tmp) ;
    }

    for(int i = 0 ; i < size ; ++i)
    {
        pool_mix.add_task(new task(nullptr, thread_func_wrapper_mixer)) ;
    }
    pool_mix.thread_pool_run() ;

    for(int i = 0 ; i < size ; ++i)
    {
        pool_mutate.add_task(new task(nullptr, thread_func_wrapper_mutator)) ;
    }
    pool_mutate.thread_pool_run() ;

    int heartbeat = 10000000;
    do{
        if(!pool_mix.stop_add_task()){
  			pool_mix.add_task(new task(nullptr, thread_func_wrapper_mixer)) ;
  		}
  		//using stop_add_task to avoid memory leak
  		if(!pool_mutate.stop_add_task()){
  			pool_mutate.add_task(new task(nullptr, thread_func_wrapper_mutator)) ;
  		}
        
        #ifdef debug
            heartbeat-- ;
            if(heartbeat == 0)
            {
                cout << 
                "heartbeat: current fitness: " << current_fitness << 
                " Generation: " << counter << 
                " Offspring_size: " << offspring.size() << 
                " popliation_size: " << population.size() <<
                " gene_bank_size: " << gene_bank.size() <<
                endl ;
                heartbeat = 10000000;
            }
        #endif
        
    }while(current_fitness > 0 && counter <= MUT_THRESHOLD) ;
    //cout << "finished" << endl ;

    #ifdef stat
	    std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> end_time = std::chrono::steady_clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time);
	#endif

    pool_mutate.thread_pool_stop() ;
    pool_mix.thread_pool_stop() ;

    #ifdef stat
	    double total_time = time_span.count() ;
		#ifdef verbose
			//print_sol(solution) ;
		    //cout << fixed<<"Threads: " << NUM_THREADS <<" Dimension:" << dimension <<" Guesses: "<< guess <<" time: min:" << tmp[MIN] << " max:" <<tmp[MAX] << " avg:" << tmp[AVG] << " total:" << total_time << endl;
		    cout << "**********************end calculating**********************"<<endl ;
	    #else
	    	cout << NUM_THREADS << ROW << " " << COL << " " << total_time << " "<< current_fitness << endl;
	    #endif
    
    #endif

    exit(0) ;
    return 0 ;
}
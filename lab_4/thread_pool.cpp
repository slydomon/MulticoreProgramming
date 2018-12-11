#include "thread_pool.hpp"

//contain: input, job(function pointer), and output for generic purpose.

genome::genome(){
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator (seed);
    std::uniform_int_distribution<int> distribution(0, INT_MAX);
    gen = std::vector<unsigned>(gen_size, 0) ;
    if(!gen_size) gen_size = 20 ; //default gen_size = 20 ;
    for(int i = 0 ; i < gen_size ; ++i){
        gen[i] = distribution(generator)%5;
    }
}

genome::genome(const genome& input)
{
    gen = input.gen ;
}

void genome::set_genome(const std::vector<unsigned>& input)
{
    for(int i = 0 ; i < gen_size ; ++i){
        if(input[i] > 4) gen[i] = input[i]%5 ; //make sure value is within 0-4
        else gen[i] = input[i] ;
    }
}

genome genome::mix(const genome& mate)
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator (seed);
    std::uniform_int_distribution<int> distribution(0, INT_MAX);
    int slice_index = distribution(generator) % gen_size ;
    int s ;
    if(slice_index >= gen_size-1 || slice_index == 0) s = gen_size / 2;
    else s = slice_index ;
    //first part of gen derives from caller and last part derive mate
    genome res ;
    std::vector<unsigned> tmp = mate.get_genome() ;
    //std::vector<unsigned> tmp = this->get_genome() ;
    //std::vector<unsigned> tmp_m = mate.get_genome() ;
    for(int i = s ; i < gen_size ; ++i) tmp[i] = gen[i];
    res.set_genome(tmp) ;
    return res; 
}

genome genome::mutate()
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator (seed);
    std::uniform_int_distribution<int> distribution(0, INT_MAX);
    int mut = distribution(generator) ;
    int check = mut % 101 ; //for counting percentage
    #ifdef debug
        if(check < 0 || check > 100){
            std::cout << "mutate function mut error: check=" << check << std::endl; 
            abort() ;
        }
    #endif
    if(check > 40)
    { 
        genome unmutated ;
        unmutated.set_genome(this->get_genome()) ;
        return unmutated;
    }
    else
    {
        int pos = mut % gen_size ;
        int gene = mut % 5 ;
        while(gene == gen[pos]) gene = distribution(generator) % 5; //to avoid the mutation failure.
        
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
        mutated.set_genome(this->get_genome()) ;
        this->gen[pos] = tmp ;
        return mutated ;
    }
}

genome genome::compact()
{
    std::vector<unsigned> compact_gene(gen_size, 0) ;
    int i = 0 ;
    for(auto it = gen.begin() ; it != gen.end() ; ++it)
    {
        if(*it != 0)
        {
            compact_gene[i] = *it ;
            ++i ;
        }
    }
    genome tmp ;
    tmp.set_genome(compact_gene) ;
    return tmp ;
}

unsigned genome::count_fitness(Maze* maze)
{
    Coord cur = maze->getStart() ;
    Coord end = maze->getFinish() ;
    int col_size = static_cast<int>(maze->get_col_size()) ;
    int row_size = static_cast<int>(maze->get_row_size()) ;
    unsigned wall = 0 ;
    unsigned dist = 0 ;
    for(int i = 0 ; i < gen_size ; ++i){
        //1 is wall, 0 is path
        switch(gen[i])
        {
            case STAY:
                break ; //if start is end, you need to stay to become the fitness
            case LEFT : 
                if(cur.col == 0 || maze->get(cur.row, cur.col-1)) ++wall ;
                //if(maze->get(cur.row, cur.col-1)) ++wall ;
                else --cur.col ;
                break ;
            case RIGHT:
                if(cur.col == col_size || maze->get(cur.row, cur.col+1)) ++wall ;
                //if(maze->get(cur.row, cur.col+1)) ++wall ;
                else ++cur.col ;
                break;
            case UP:
                if(cur.row == 0 || maze->get(cur.row-1, cur.col)) ++wall ;
                //if(maze->get(cur.row-1, cur.col)) ++wall ;
                else --cur.row ;
                break ;
            case DOWN:
                if(cur.row == row_size || maze->get(cur.row + 1, cur.col)) ++wall ;
                //if(maze->get(cur.row + 1, cur.col)) ++wall ;
                else ++cur.row ;
                break;
            default:
                std::cout << "gene does not range from 0 to 4" << std::endl;
                abort() ;
        }
        //check if reach the end point, if so return 0(the fitness)
        #ifdef debug
            if(cur.col == end.col && cur.row == end.row) std::cout << "**********fitness_counter::find path*********" << std::endl ;
        #endif
    }
    dist = abs(static_cast<int>(cur.col - end.col)) + abs(static_cast<int>(cur.row - end.row)) ;
    /*if(dist == 0)
        return 0 ;
    else*/
        return 2*dist + wall ;
}

std::vector<unsigned> string_to_vector(std::string s)
{
    std::string::size_type sz; 
    int size = s.size() ;
    std::vector<unsigned> tmp ; 
    for(int i = 0 ; i < size ; ++i)
        tmp.push_back(s[i] - '0') ;
    return tmp ;
}

void genome::count_fitness_debug(Maze* maze) const
{
    Coord cur = maze->getStart() ;
    Coord end = maze->getFinish() ;
    unsigned wall = 0 ;
    unsigned dist = 0 ;
    std::vector<unsigned> gene = string_to_vector("442233222244441111114422222200") ;
    std::string s = "" ;
    for(int i = 0 ; i < gen_size ; ++i)
        s = s + std::to_string(gene[i]) ;

    int col_size = maze->get_col_size() ;
    int row_size = maze->get_row_size() ;
    std::cout << "COL:" << col_size << " ROW:" << row_size << std::endl ;

    for(int i = 0 ; i < gen_size ; ++i){
        //1 is wall, 0 is path
        //switch(gen[i])
        switch(gene[i])
        {
            case STAY:
                break ; //if start is end, you need to stay to become the fitness
            case LEFT : 
                if(cur.col == 0 || maze->get(cur.row, cur.col-1))
                {
                    std::cout << "move left and hit the wall" << std::endl ;
                    ++wall ;
                }
                else
                {
                    std::cout << "move left" << std::endl ;
                    --cur.col ;
                } 
                break ;
            case RIGHT:
                if(cur.col == col_size-1 || maze->get(cur.row, cur.col+1))
                {
                    std::cout << "move right and hit the wall" << std::endl ;
                    ++wall ;
                }
                else 
                {
                    std::cout << "move right" << std::endl ;
                    ++cur.col ;
                }
                break;
            case UP:
                if(cur.row == 0 || maze->get(cur.row-1, cur.col))
                {
                    std::cout << "move up and hit the wall" << std::endl ;
                    ++wall ;
                }
                else 
                {
                    std::cout << "move up" << std::endl; 
                    --cur.row ;
                }
                break ;
            case DOWN:
                if(cur.row == row_size-1 || maze->get(cur.row + 1, cur.col))
                //if(maze->get(cur.row + 1, cur.col))
                {
                    std::cout << "move down hit the wall" << std::endl ;
                    ++wall ;
                }
                else
                {
                    std::cout << "move down" << std::endl ;
                    ++cur.row ;
                }
                break;
            default:
                std::cout << "gene does not range from 0 to 4" << std::endl;
                abort() ;
        }
        //check if reach the end point, if so return 0(the fitness)
        if(cur.col == end.col && cur.row == end.row) std::cout << "**********debug::find path*********" << std::endl ;
    }
    dist = abs(static_cast<int>(cur.col - end.col)) + abs(static_cast<int>(cur.row - end.row)) ;
    std::cout << "*******************************new fitness*****************************" << std::endl <<  
    "final location : (" << cur.col <<" , " << cur.row <<")" << std::endl <<
    "hit wall time: " << wall << std::endl <<
    "dist: " << dist << std::endl <<
    "score" << dist*2 + wall << std::endl <<
    //"genome: " << s << std::endl <<
    "***********************************************************************" << std::endl; 
    /*if(dist == 0)
        return 0 ;
    else*/
    return ;
}

task::task(void* _input, thread_func_ptr _job)
{ 
    input = _input;
    job = _job;
}

task::task(const task& t)
{	
    input = t.input; 
    job = t.job;
}

task::~task(){
    if(input) delete input ;
    if(output) delete output ;
} ;


thread_pool::thread_pool(int thread_number)
{
    thread_num = thread_number ;
    threads = new pthread_t[thread_num] ;
}

thread_pool::~thread_pool()
{
    if(threads)
        delete[] threads ;
}

task* thread_pool::thread_pool_aggregate(cmp my_cmp)
{
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
                int tmp_s = ((std::vector<double>*)(t1->get_output()))->size() ;
                std::cout << "t1: "<< ((std::vector<double>*)(t1->get_output()))->at(tmp_s-1) << std::endl ;
                return t1;
            }
            res = my_cmp(t1, res) ;
            complete.push(res) ;
        }
    }
    return res; 
}

task* thread_pool::thread_pool_aggregate_at_least(cmp my_cmp, int batch_volume, 
    timespec* estimate_time_per_task, double current_best, double init_best)
{
    //estimate_time_per_task->tv_sec *= batch_volume ;
    //estimate_time_per_task->tv_nsec *= batch_volume ;
    //nanosleep(estimate_time_per_task, NULL) ; //in order to successfully execute the batch.
    task* res = nullptr ;
    int size = complete.size() ;
    double factor = (current_best/init_best) * static_cast<double>(batch_volume) ;
    if(factor < 1)
        batch_volume = 1 ;
    else
        batch_volume = static_cast<int>(factor) ;
    //the batch size will decrease by percentage, it becomes the original thread_pool_aggregate.
    if(size > batch_volume){
        //cout << "batch_volume: " << batch_volume << endl ;
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
void* thread_pool::dispatching(void* arg)
{
    void* tmp ;
    while(proceed){
        task* t = nullptr ;
        pending.listen(t) ;
        //std::cout << pending.size() << std::endl ;
        thread_func_ptr job = t->get_job() ;
        //tmp = job(t->get_input()) ;
        job(t->get_input()) ;
        delete t ;
        //t->set_output(tmp) ;
        //complete.push(t) ;
    }
    return NULL ;
}

//if ture, task is dispatched ; if false, there is no task in pending 
void thread_pool::dispatch_task(int pid)
{ 
    pthread_create(&threads[pid], NULL, run_dispatching, this) ;
    pthread_detach(threads[pid]) ;
    return ; 
}

void thread_pool::thread_pool_run()
{
    for(int i = 0 ; i < thread_num ; ++i){
        this->dispatch_task(i) ;
    }
}

void thread_pool::thread_pool_stop()
{
    pthread_mutex_lock(&continue_lock) ;
    proceed = false ;
    pthread_mutex_unlock(&continue_lock) ;
}

#ifdef stat
    void* thread_pool::dispatching_stat(void* arg)
    {
        //using namespace std ;
        void* tmp ;
        while(proceed){
            task* t = nullptr ;
            pending.listen(t) ;
            thread_func_ptr job = t->get_job() ;
            //measure the execution time of each job
            std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>>  start_time = std::chrono::steady_clock::now();
            tmp = job(t->get_input()) ;
            delete t ;
            std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> end_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time);
            job_exec_time.push(time_span.count()) ;

            //set output.
            //t->set_output(tmp) ;
            //complete.push(t) ;
            //pthread_mutex_lock(&guess_counter_lock) ;
            //++guess_counter ; //not acutually how much time it guess; instead, it is how much "task" it executes.
            //pthread_mutex_unlock(&guess_counter_lock) ;

        }
        return NULL ;
    }
    //return 0 is min, 1 is max, 2 is avg;
    std::vector<double> thread_pool::calculate_time_stat()
    {
        double min = DBL_MAX ;
        double max = -1 ;
        double temp = 0 ;
        double cur ; 
        int size = job_exec_time.size() ; //maybe unsave ;
        for(int i = 0 ; i < size ; ++i){
            job_exec_time.pop(cur) ; 
            temp = temp + cur ;
            if(cur > max) max = cur ;
            if(cur < min) min = cur ;
        }
        std::vector<double> res(3,0) ;
        double tmp_size = static_cast<double>(size) ;
        res[0] = min ;
        res[1] = max ;
        res[2] = temp/tmp_size ;
        return res ;
    }

#endif

bool thread_pool::stop_add_task()
{
    if(!pending.empty() && pending.size() > thread_num*10)
        return true ;
    else
        return false ;
}


#include <vector>
#include <list>
#include <iostream>
#include <cstdlib>


using namespace std ;

template<class K, class V>
class my_multimap{
private:
	list<pair<K, list<V> > >table  ; ///< The container to save Key Value pair.	
public:
	my_multimap(){} ;
	/**Should return true if the operation
	succeeded, or false otherwise. It’s likely you won’t have a case where you need to return false
	in this simple implementation.*/
	bool insert(const K& key, const V& value){
		auto end = table.end() ; 
		for(auto it = table.begin() ; it != end ; ++it){
			if(it->first == key){
				it->second.push_back(value) ;
				return true ;
			}
		}
		table.push_back(pair<K, list<V> >(key, list<V>(1,value))) ;
		return true ;
	}

	/**Should return true if key is in the multimap, or false
	otherwise.*/
	bool find(const K& key){
		auto end = table.end() ; 
		for(auto it = table.begin() ; it != end ; ++it){
			if(it->first == key)
				return true ;
		}
		return false ;
	} 

	/**Same as (2), but if true is
	returned, also populates the passed-by-reference list values with each of the values
	present for the given key.*/
	bool find(const K& key, std::list<V>& values){
		auto end = table.end() ; 
		for(auto it = table.begin() ; it != end ; ++it){
			if(it->first == key){
				values = it->second ;
				return true ;
			}
		}
		return false ;
	} 

	/**Should remove any key-value pair with the given key, and
	return the number of key-value pairs removed. As with insert(), it’s likely you won’t have a
	case where you need to return -1 in this simple implementation.*/
	int remove(const K& key){
		auto end = table.end() ;
		for(auto it = table.begin() ; it != end; ++it){
			if(it->first == key){
				int size = it->second.size() ;
				table.erase(it) ;
				return size ;
			}
		}

		///if do not find key ;
		return -1 ; 
	}

};

template<class V>
bool cmp_list(list<V> l1, list<V> l2){
	if(l1.size()!= l2.size())
		return false ;
	auto it1 = l1.begin() ;
	auto it2 = l2.begin() ;
	auto end1 = l1.end() ;
	auto end2 = l2.end() ;
	while(it1 != end1 && it2 != end2){
		if(to_string(*it1).compare(to_string(*it2)) !=0)
			return false ;
		it1++ ;
		it2++ ;
		//cout << to_string(*it1) << " " << to_string(*it2) <<endl; 
	}
	return true ;
}



int main(int argc, char* argv[]){
	my_multimap<int, int> int_map = my_multimap<int,int>() ;
	vector<pair<int, int> > record ;
	list<int> test ;
	srand (time(NULL));

	for(int i = 0 ; i < 10 ; ++i){ 
		int random_key = rand()%201 ;
		int random_val = rand()%201 ; 
		record.push_back(pair<int, int>(random_key,random_val)) ;
    	if(int_map.insert(random_key,random_val) == true)
    		cout << "pass \"insert\" test key:" << random_key << " value:" << random_val << endl ;
    	else
    		cout << "fail \"insert\" test key:" << random_key << " value:" << random_val << endl ;
    }

    for(int i = 0 ; i < 10 ; ++i){ 
    	if(int_map.find(record[i].first) == true)
    		cout << "pass \"find\" test key:" << record[i].first << endl ;
    	else
    		cout << "fail \"find\" test key:" << record[i].first << endl ;
    }

    for(int i = 0 ; i < 10 ; ++i){ 
    	int_map.find(record[i].first, test) ;
    	int size = test.size() ;
    	if(int_map.remove(record[i].first) == size)
    		cout << "pass \"remove\" test key:" << record[i].first << " size" << size << endl ;
    	else
    		cout << "fail \"remove\" test key:" << record[i].first << " size" << size << endl ;
    }
    return 0;

}
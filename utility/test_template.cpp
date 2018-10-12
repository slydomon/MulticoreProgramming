#include <vector>
#include <list>
#include <iostream>
#include <cstdlib>
#include <gtest/gtest.h>

using namespace std ;
		
TEST(basic, Class_Requirement) {
    EXPECT_EQ(1, 1); //required the return value to be equal ;
    ASSERT_TRUE(true) ; //assert whether the return value is true;
}

int main(int argc, char* argv[]){

	testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}

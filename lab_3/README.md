### 1.Please use the Makefile to build the project by typing: make:

### 2.User can specify the following information to run the program:

    "-n ($int) to specify number of threads in threads pool; default: 1"
    "-d ($int) to specify the dimension of the polynomial function; defult: 2"
    "-t ($double) to specify the tolerance of the solution; default: 0.1"
    "-l ($double) to specify the lower_bound of the solution; default: -10.0"
    "-u ($double) to specify the upper_bound of the solution; default: 10.0"

### 3.If dimension is specified, the program will ramdonly generaten points and calculate the coefficients.
    
    The range of the (x, y) of the random-generated points is from 0 to 9 for simplification.
    
### 4.using Makefile to compile the binary.

### 5.after compiling the file, run ./stat.sh to extract the required stat report.

### 6.the report will be in stat.txt file and the column detail is as following: 

    |Threads |Dimension |GuessCount |min time |max time |avg time |total time |.

#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <functional>
#include <unistd.h>

#include "libs/json.hpp"
#include "libs/md5.hpp"
#include "libs/SparseMatrix/SparseMatrix.cpp"

#define KB  1024
#define MB  1048576
#define REP (KB * KB)
#define OFFSET 1024
#define ARR_SIZE (100 * MB)

#define GIGA  1.0e+9
#define MEGA  1.0e+6
#define KILO  1.0e+3
#define MILI  1.0e-3
#define MICRO 1.0e-6
#define NANO  1.0e-9

#define CHAR_SIZE sizeof(char)
#define INT_SIZE  sizeof(int)

#define SHOW_DURATION true
#define SHOW_DETAILS  true

using namespace std;
using json = nlohmann::json;


static unsigned long x=123456789, y=362436069, z=521288629;

/**
 * Function will return pseudorandom value (long)
 * @return  pseudorandom value with period of 2^96-1
 */
unsigned long random_long(void) {          //period 2^96-1
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

   t = x;
   x = y;
   y = z;
   z = t ^ x ^ y;

  return z;
}

/**
 * Function will return random value between min and max
 * @param  min min value inclusive
 * @param  max max value inclusive
 * @return     <min, max>
 */
int random_range(int min=1, int max=9) {
    return min + (rand() % (int)(max - min + 1));
}


/**
 * Simple class for measuring time
 */
class Timer {
    protected:
        chrono::high_resolution_clock::time_point _start;
        chrono::high_resolution_clock::time_point _stop;
    public:
        chrono::duration<double, nano> duration;
        /**
         * Starts the timer
         */
        void start() {
            this->_start = std::chrono::high_resolution_clock::now();
        }
        /**
         * Stops the timer and calculates duration of the measured block
         */
        void stop() {
            this->_stop = std::chrono::high_resolution_clock::now();
            this->duration = chrono::duration_cast<chrono::nanoseconds>(this->_stop - this->_start);
        }
};


/**
 * Function will return pseudorandom string using string generator
 * Returned string is consisting of A-Za-z0-9
 * @param  length length of the string
 * @return        A-Za-z0-9 string of given length
 */
std::string random_string( size_t length ) {
    auto randchar = []() -> char {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ random_long() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}


/**
 * Function prints message (with printf format support) 
 * with \r carriage set (output will remain on the same line)
 * @param fmt     format prescription
 * @param VARARGS additional arguments passed to format function
 */
void printf_debug(const char * fmt, ...) {
    printf("                                          \r");
    string newfmt = string(fmt) + "\r";
    const char * newfmt_c = newfmt.c_str();
    va_list args;
    va_start(args, fmt);
    vprintf(newfmt_c, args);
    va_end(args);
    cout << flush;
}


/**
 * TEST will sum random longs for the given amount of reps
 * total number of is also given by global REP value
 * total = reps*REP
 *
 * This test is testing CPU speed, there should not be
 * any memory stress since we are only using single variable
 * All other routines such as random generation should fit into L1 cache
 *
 * Following json dictionary is merged with given json:
 *    {
 *      duration: <duration in seconds>,
 *      size:     <total number of reps>
 *    }
 * 
 * @param results json holder
 * @param reps    number of reps
 */
void test_reg_simple(json &results, int reps = 512) {
    printf_debug("cpu simple, size=%d, reps=%d", REP, reps);
    Timer timer;
    
    //-------------------------------------------------------
    timer.start();
    int sum;
    for (size_t j = 0; j < reps; j++) {
        for (size_t i = 0; i < REP; i++) {
            sum += random_long();
        }
    }
    timer.stop();
    //-------------------------------------------------------
    results["duration"] = timer.duration.count() * NANO;
    results["size"]     = REP;
    results["reps"]     = reps;
}


/**
 * TEST will generate random longs and will use simple build-in hash function
 * storing it in the single variable (no append, value is set - memory consumption is constant)
 * total number of is also given by global REP value
 * total = reps*REP
 *
 * This test is testing CPU speed, there should not be
 * any memory stress since we are only using single variable
 * All other routines such as random generation should fit into L1 cache
 *
 * Following json dictionary is merged with given json:
 *    {
 *      duration: <duration in seconds>,
 *      size:     <total number of reps>
 *    }
 * 
 * @param results json holder
 * @param reps    number of reps
 */
void test_reg_hash(json &results, int reps = 128) {
    printf_debug("cpu hash, size=%d, reps=%d", REP, reps);
    Timer timer;
    hash<int> int_hash;
    
    //-------------------------------------------------------
    timer.start();
    string res;
    const int HALF = RAND_MAX / 2;
    for (size_t j = 0; j < reps; j++) {
        for (size_t i = 0; i < REP; i++) {
            res = int_hash(random_long() - HALF);
        }
    }
    timer.stop();
    //-------------------------------------------------------
    results["duration"] = timer.duration.count() * NANO;
    results["size"]     = REP;
    results["reps"]     = reps;
}


/**
 * TEST will generate random string of length 16 y default and will use md5 hash on result
 * storing it in the single variable (no append, value is set - memory consumption is constant)
 * total number of is also given by global REP value
 * total = reps*REP
 *
 * This test is testing CPU speed, there should not be
 * any memory stress since we are only using single variable
 * All other routines such as random generation should fit into L1-l2 cache
 *
 * Following json dictionary is merged with given json:
 *    {
 *      duration: <duration in seconds>,
 *      size:     <total number of reps>
 *    }
 * 
 * @param results json holder
 * @param reps    number of reps
 * @param str_len length of the random string
 */
void test_reg_md5(json &results, int reps = 1, int str_len = 16) {
    printf_debug("cpu md5, size=%d, reps=%d", REP, reps);
    Timer timer;
    
    //-------------------------------------------------------
    timer.start();
    string res;
    for (size_t j = 0; j < reps; j++) {
        for (size_t i = 0; i < REP; i++) {
            res = md5(random_string(str_len));
        }
    }
    timer.stop();
    //-------------------------------------------------------
    results["duration"] = timer.duration.count() * NANO;
    results["size"]     = REP * str_len;
    results["reps"]     = reps;
}


/**
 * Test will read and write values on array elements. R/W order is given by 
 * value sizes, where lower number restricts access of the array elements
 * meaning lower number will force lower cache to work
 *
 * This test is testing MEMORY speed. It can stress all types of memory 
 * from L1 to RAM based on given sizes value.
 * 
 * Following json dictionary is merged with given json:
 *    {
 *      duration: <duration in seconds>,
 *      size:     <total number of reps>
 *      N:        <number of sizes in test>
 *    }
 */
template <int N>
int* test_mem(json &results, int (&sizes)[N], int size = ARR_SIZE, int reps=32) {
    printf_debug("creating array...         ");
    static int arr[ARR_SIZE];
    int sum = 0;
    int mod = 16-1;
    Timer timer;
    printf_debug("mem, size=%d, reps=%d", REP * reps, N);
    
    //-------------------------------------------------------
    timer.start();
    for (int i = 0; i < N; i++) {
        printf_debug("mem, size=%d, reps=%d, (%02d/%02d)", REP * reps, N, i+1, N);
        mod = sizes[i] - 1;

        for (int j = 0; j < REP * reps; j++) {
            arr[(j * OFFSET) & mod]++;
            // arr[(j * OFFSET) & mod]--;
            // sum += arr[(j * OFFSET) & mod];
            // arr[(i * MB + i) & ARR_SIZE] = i;//random_long();
        }
        // results["duration_" + to_string(sizes[i])] = timer.duration.count() * NANO;
    }
    timer.stop();
    //-------------------------------------------------------
    
    results["duration"] = timer.duration.count() * NANO;
    results["size"]     = N;
    results["reps"]     = reps;
    return arr;
}


/**
 * Test will read and write values on array elements. R/W order is given by 
 * value sizes, where lower number restricts access of the array elements
 * meaning lower number will force lower cache to work
 *
 * This test is testing MEMORY speed. It can stress all types of memory 
 * from L1 to RAM based on given sizes value.
 * 
 * Following json dictionary is merged with given json:
 *    {
 *      duration: <duration in seconds>,
 *      size:     <total number of reps>
 *    }
 */
void mat_mul(json &results, int size=256, int reps=16) {
    int aMatrix[size][size];
    int bMatrix[size][size];
    int product[size][size];
    Timer timer;
    printf_debug("generating matrices, size=%dx%d", size, size);
    
    // first create matrices
    for (size_t i = 0; i < size; i++) {
        for (size_t j = 0; j < size; j++) {
            aMatrix[i][j] = random_long();
            bMatrix[i][j] = random_long();
            product[i][j] = 0;
        }
    }
    
    printf_debug("mat mul, size=%dx%d, reps=%d", size, size, reps);
    
    //-------------------------------------------------------
    int k, row, col, inner;
    timer.start();
    // do the multiplication
    for (k = 0; k < reps; k++) {
        for (row = 0; row < size; row++) {
            for (col = 0; col < size; col++) {
                // Multiply the row of A by the column of B to get the row, column of product.
                for (inner = 0; inner < size; inner++) {
                    product[row][col] += aMatrix[row][inner] * bMatrix[inner][col];
                }
            }
        }
    }
    timer.stop();
    //-------------------------------------------------------
    
    results["duration"] = timer.duration.count() * NANO;
    results["size"]     = size;
    results["reps"]     = reps;
}


/**
 * Test will generate random matrix rows x cols, and 
 * vector of length cols and populate both structures with data (for matrix 
 * total of n data is generated).
 * We then perform matrix vector multiplication (format CSR for sparse matrices)
 *
 * This test is testing MEMORY speed and also CPU speed. Memory access is random-like
 * 
 * 
 * Following json dictionary is merged with given json:
 *    {
 *      duration: <duration in seconds>,
 *      size:     <total number of operations>
 *    }
 */
void test_sparse_mat_vec(json &results, int rows=100, int cols=100, int n=10, int reps=512) {
    SparseMatrix<int> mat(rows, cols);
    vector<int> vec(cols, 0);
    Timer timer;
    
    printf_debug("generating matrices, size=%dx%d, reps=%d", rows, cols, reps);
    
    // initialize matrix values
    int x, y;
    for (size_t i = 0; i < n; i++) {
        int val = 0;
        do {
            x = random_range(1, rows);
            y = random_range(1, cols);
            val = mat.get(x, y);
        } while(val != 0);
        
        mat.set(
            random_long(),      // random value
            x,                  // random x coordinate (indexing from 1)
            y                   // random y coordinate (indexing from 1)
        );
    }
    
    // initialize vector values
    for (size_t i = 0; i < cols; i++) {
        vec[i] = random_long();
    }
    
    printf_debug("sparse mat vec mul, size=%dx%d, reps=%d", rows, cols, reps);
    double time_total = 0;
    //-------------------------------------------------------
    int i;
    timer.start();
    for (i = 0; i < reps; i++) {
        vector<int> result;
        result = mat * vec;
    }
    timer.stop();
    //-------------------------------------------------------
    
    
    // std::cout << "\n" << mat << "\n\n * \n\n";
    // for (auto i = vec.begin(); i != vec.end(); ++i) std::cout << *i << ' ';
    // std::cout << "\n\n = \n\n";
    // 
    // for (auto i = result.begin(); i != result.end(); ++i)
    //     std::cout << *i << ' ';
    // std::cout << "\n\n";
    
    results["duration"] = timer.duration.count() * NANO;
    results["size"]     = cols;
    results["reps"]     = reps;
}


/**
 * Test will generate random matrix rows x cols, and 
 * vector of length cols and populate both structures with data (for matrix 
 * total of n data is generated).
 * We then perform matrix vector multiplication (format CSR for sparse matrices)
 *
 * This test is testing MEMORY speed and also CPU speed. Memory access is random-like
 * 
 * 
 * Following json dictionary is merged with given json:
 *    {
 *      duration: <duration in seconds>,
 *      size:     <total number of operations>
 *    }
 */
void test_sparse_mat_mat(json &results, int rows=100, int cols=100, int n=10, int reps=512) {
    SparseMatrix<int> A(rows, cols);
    SparseMatrix<int> B(cols, rows);
    Timer timer;
    
    printf_debug("generating matrices, size=%dx%d, reps=%d", rows, cols, reps);
    
    // initialize matrix values
    int x, y;
    for (size_t i = 0; i < n; i++) {
        int val = 0;
        do {
            x = random_range(1, rows);
            y = random_range(1, cols);
            val = A.get(x, y);
        } while(val != 0);
        
        A.set(
            random_range(),      // random value
            x,                  // random x coordinate (indexing from 1)
            y                   // random y coordinate (indexing from 1)
        );
    }
    
    // initialize matrix values
    for (size_t i = 0; i < n; i++) {
        int val = 0;
        do {
            x = random_range(1, cols);
            y = random_range(1, rows);
            val = B.get(x, y);
        } while(val != 0);
        
        B.set(
            random_range(),      // random value
            x,                  // random x coordinate (indexing from 1)
            y                   // random y coordinate (indexing from 1)
        );
    }
    
    printf_debug("sparse mat mat mul, size=%dx%d, reps=%d", rows, cols, reps);
    double time_total = 0;
    
    
    
    //-------------------------------------------------------
    int i;
    timer.start();
    for (i = 0; i < reps; i++) {
        SparseMatrix<int> C = A * B;
        // std::cout << A;;
        // std::cout << "\n\n*\n\n";
        // std::cout << B;
        // std::cout << "\n\n=\n\n";
        // std::cout << C;
    }
    timer.stop();
    //-------------------------------------------------------
    
    
    // for (auto i = vec.begin(); i != vec.end(); ++i) std::cout << *i << ' ';
    // std::cout << "\n\n = \n\n";
    // 
    // for (auto i = result.begin(); i != result.end(); ++i)
    //     std::cout << *i << ' ';
    // std::cout << "\n\n";
    
    results["duration"] = timer.duration.count() * NANO;
    results["size"]     = cols;
    results["reps"]     = reps;
}
    


/**
 * Start benchmark, usage:
 * optional <output> file: will create json file output with results
 * optional <scale> float: scales repetition count for benchmark
 *                         default is 1, for example value 2 will run tests 
 *                         twice as many times, value 0.5 will experiments half
 *                         as many times
 */
int main(int argc,  char* argv[]) {
    int rep_cnt = (int)(argc >= 3 ? std::stof(argv[2]) * REP : 1 * REP);
    
    printf_debug("running tests...         ");
    json results;
    results["version"] = "1.0.1";
    
    
    Timer test_timer;
    test_timer.start();
    // ------------------------------------------------------------------------
    
    // valgrind: D1   miss rate  0.0%
    // valgrind: LLd  miss rate  0.0%
    // valgrind: LL   miss rate  0.0%
    int sizes_l1[] = { 4, 8, 16, 32, 64, 128, 256, 512, 1 * KB, 2 * KB };
    test_mem(results["mem_l1"], sizes_l1);
    // 
    // // valgrind: D1   miss rate  9.5%
    // // valgrind: LLd  miss rate  0.0%
    // // valgrind: LL   miss rate  0.0%
    int sizes_l2[] = { 4 * KB, 8 * KB, 16 * KB, 32 * KB, 64 * KB, 128 * KB };
    test_mem(results["mem_l2"], sizes_l2);
    
    // valgrind: D1   miss rate 14.2%
    // valgrind: LLd  miss rate  5.7%
    // // valgrind: LL   miss rate  1.9%
    int sizes_l3[] = { 256 * KB, 512 * KB, 1 * MB, 2 * MB, 4 * MB };
    test_mem(results["mem_l3"], sizes_l3);
    // 
    // // valgrind: D1   miss rate 14.2%
    // // valgrind: LLd  miss rate 14.2%
    // // valgrind: LL   miss rate  4.9%
    int sizes_ll[] = { 8 * MB, 16 * MB, 32 * MB };
    test_mem(results["mem_ll"], sizes_ll);
    
    // valgrind: D1   miss rate  0.0%
    // valgrind: LLd  miss rate  0.0%
    // valgrind: LL   miss rate  0.0%
    test_reg_simple(results["cpu_simple"]);
    
    // valgrind: D1   miss rate  0.0%
    // valgrind: LLd  miss rate  0.0%
    // valgrind: LL   miss rate  0.0%
    test_reg_hash  (results["cpu_hash"]);
    
    // valgrind: D1   miss rate  0.0%
    // valgrind: LLd  miss rate  0.0%
    // valgrind: LL   miss rate  0.0%
    test_reg_md5   (results["cpu_md5"]);
    
    
    mat_mul(results["mmn_s1"],  16, 4*8*8*8*8*8);
    mat_mul(results["mmn_s2"],  64, 4*8*8*8);
    mat_mul(results["mmn_s3"], 128, 4*8*8);
    mat_mul(results["mmn_s4"], 512, 4);
    
    
    test_sparse_mat_mat(results["mms_s1"],        8,        8,         8*2, 64*64*64);
    test_sparse_mat_mat(results["mms_s2"],       32,       32,        32*2, 64*64);
    test_sparse_mat_mat(results["mms_s3"],      128,      128,       128*2, 64);
    test_sparse_mat_mat(results["mms_s4"],      512,      512,       512*2, 1);
    
    test_sparse_mat_vec(results["mvs_s1"],        8,        8,         8*2, 100*64*64*4*4);
    test_sparse_mat_vec(results["mvs_s2"],       32,       32,        32*2, 100*64*64*4);
    test_sparse_mat_vec(results["mvs_s3"],      128,      128,       128*2, 100*64*64);
    test_sparse_mat_vec(results["mvs_s4"],     8192,      8192,     8192*2, 100*64);
    // ------------------------------------------------------------------------
    test_timer.stop();
    printf("---------------------------------\n");
    
    printf("%-30s: %1.3f\n", "time taken", test_timer.duration.count() * NANO);
    
    printf_debug("generating output...    \n");
    cout << results.dump(true) << endl;

    // if arg is set we redirect dump to file
    if (argc >= 2) {
        ofstream ofs (argv[1]);
        ofs << results.dump(true) << endl;
    }
    
    return 0;
}
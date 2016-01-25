//C++ Standard Libraries
#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

//external libraries
#include "concurrentqueue.hpp"  //lockfree queue
#include <gmpxx.h>              //MPIR of GNU-MP BigInt libraries

//Object to store all parameters for the queue
class Parameters
{
    public:
        std::vector< std::vector< bool > > grid;
        int startrow;
        int length;
        int row;
        int col;
        int remaining;
};

void printVector(std::vector<std::vector<bool> > &grid);                                  //print the vector to the screen for debugging purposes
void initForbidden(std::vector<std::vector<bool>> &grid, const unsigned int length);      //set primary off-limits fields
void FillLookupTable(std::vector<std::vector<bool>> &g, const unsigned int length);       //generate lookuptable


void ProduceStep(std::vector<std::vector<bool>> &grid, mpz_class &counter, const int &startrow, const int &length, int row, int col, int remaining, int &depth);    //Produce queue jobs
void TakeStep(std::vector<std::vector<bool>> &grid, mpz_class &counter, const int &startrow, const int &length, int row, int col, int remaining);       //General walking function

void IncreaseCCC(mpz_class &Incr);  //Thread-safe increase of the total counter
mpz_class ReadCCC();                //Thread-safe read from the total counter, not absolutely necessary

int checkInput();                       //check if the given input meets the requirements of >4 and an even number
void WorkerFunc();                      //Retrieves jobs from queue en proceeds calculating them with TakeStep
void HostWorkerFunc(unsigned int n, bool quiet, int d);
int main(int argc,char *argv[]);        //Main function. Variables are initialized and all parts of the code are managed.

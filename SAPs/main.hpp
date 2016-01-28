//C++ Standard Libraries
#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <iomanip>

//external libraries
#include "concurrentqueue.hpp"  //lockfree queue
#include <gmpxx.h>              //MPIR of GNU-MP BigInt libraries

//Object to store all parameters for the queue
class Parameters
{
    public:
        std::vector< std::vector< int > > grid;
        unsigned int startrow;
        unsigned int row;
        unsigned int col;
        int remaining;
};

template <typename VecT>
void printVector(VecT &grid);                                  //print the vector to the screen for debugging purposes

//generate distance lookuptable && set primary off-limits fields 
void InitMaps(std::vector<std::vector<int>> &lookuptable, std::vector<std::vector<int>> &grid, const unsigned int length);       

bool CheckStep(std::vector< std::vector< int > > &lookuptable, std::vector<std::vector<int>> &grid, mpz_class &counter, unsigned int row, unsigned int col, int remaining);
void ProduceStep(std::vector< std::vector< int > > &lookuptable, std::vector<std::vector<int>> &grid, mpz_class &counter, const unsigned int &startrow, const int &length,unsigned int row,unsigned int col, int remaining, int &depth);    //Produce queue jobs
void TakeStep(std::vector< std::vector< int > > &lookuptable, std::vector<std::vector<int>> &grid, mpz_class &counter, const unsigned int &startrow, unsigned int row, unsigned int col, int remaining);       //General walking function

void IncreaseCCC(mpz_class &Incr);  //Thread-safe increase of the total counter
mpz_class ReadCCC();                //Thread-safe read from the total counter, not absolutely necessary

void WorkerFunc(std::vector< std::vector< int > > &lookuptable, unsigned int n, unsigned int d, bool isHost);                      //Retrieves jobs from queue en proceeds calculating them with TakeStep if arguments given it will behave as the reporting host thread
void ThreadDivideWork(std::vector< std::vector< int > > &lookuptable, unsigned int ReportRate);

int checkInput();                       //check if the given input meets the requirements of >4 and an even number
int checkArguments(int &argc, char *argv[]);

int main(int argc,char *argv[]);        //Main function. Variables are initialized and all parts of the code are managed.

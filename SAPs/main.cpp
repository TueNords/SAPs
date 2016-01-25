#include "main.hpp"

std::mutex tex;
mpz_class CCCounter;
moodycamel::ConcurrentQueue<Parameters> q;
moodycamel::ProducerToken ptok(q);

void printVector(std::vector<std::vector<bool> > &grid)
{
    for ( std::vector<std::vector<bool>>::size_type i = 0; i < grid.size(); ++i )
        {
            for ( std::vector<bool>::size_type j = 0; j < grid.at(i).size(); ++j )
            {
                //std::cout << grid[i][j] << ' ';
                std::cout << grid.at(i).at(j);
            }
            std::cout << std::endl;
        }
}

void initForbidden(std::vector<std::vector<bool>> &grid, const unsigned int length)
{
    int starty=(length-6)/2+1;

    for ( std::vector< std::vector<bool> >::size_type i = 0; i < grid.size(); ++i )
        {
            grid.at(i).at(0)=1;
            grid.at(i).at(grid.at(i).size()-1)=1;
            if (i<=(length-6)/2 && length>4) //if length == 4, no other fields should be off limits, else the grid would become too small.
            {
                grid.at(i).at(1)=1;
            }
        }
    for ( std::vector< std::vector<bool> >::size_type i = 0; i < grid.at(0).size(); ++i )
        {
            grid.at(0).at(i)=1;
            grid.at(grid.size()-1).at(i)=1;
        }

    if (length>4)
    {
        for ( int i = 1; i < (int)grid.size()-1; ++i )
        {
            for (int j = 1; j < (int)grid.at(i).size()-1; ++j)
            {
                if (i<starty)
                {
                   if (abs(i-starty)+abs(j-1)>(int)length/2-1)
                    {
                        grid.at(i).at(j)=1;
                    }
                }
                else
                {
                    if (abs(i-starty)+abs(j-1)>(int)length/2)
                    {
                        grid.at(i).at(j)=1;
                    }
                }

            }
        }
    }
}


void FillLookupTable(std::vector<std::vector<bool>> &g, const unsigned int length)
{
    int starty=(length-6)/2+1;

    for ( int i = 0; i < (int)grid.size()-1; ++i )
    {
        for (int j = 0; j < (int)grid.at(i).size()-1; ++j)
        {
            grid.at(i).at(j)=abs(i-1)+abs(j-starty);
        }
    }
}



void ProduceStep(std::vector<std::vector<bool>> &grid, mpz_class &counter, const int &startrow, const int &length, int row, int col, int remaining, int &depth)
{
    if (!grid.at(row).at(col))
    {
        if (row==startrow+1 && col==1 && remaining==1)
        {
            ++counter;
        }
        else
        {
            if (abs(row-startrow-1)+abs(col-1) <= remaining)
            {
                if (row==startrow+1 && col==1 && remaining>1);
                else
                {
                    grid.at(row).at(col)=true;
                    if (remaining>=length-depth)
                    {
                        ProduceStep(grid,counter,startrow,length,row,col+1,remaining-1,depth);
                        ProduceStep(grid,counter,startrow,length,row+1,col,remaining-1,depth);
                        ProduceStep(grid,counter,startrow,length,row,col-1,remaining-1,depth);
                        ProduceStep(grid,counter,startrow,length,row-1,col,remaining-1,depth);
                    }
                    else
                    {
                        Parameters taskparm;
                        taskparm.grid = grid;
                        taskparm.startrow = startrow;
                        taskparm.length = length;
                        taskparm.remaining = remaining-1;
                        if (!grid.at(row).at(col+1))
                        {
                            taskparm.row = row;
                            taskparm.col = col+1;
                            q.enqueue(ptok, taskparm);
                        }
                        if (!grid.at(row+1).at(col))
                        {
                            taskparm.row = row+1;
                            taskparm.col = col;
                            q.enqueue(ptok, taskparm);
                        }
                        if (!grid.at(row).at(col-1))
                        {
                            taskparm.row = row;
                            taskparm.col = col-1;
                            q.enqueue(ptok, taskparm);
                        }
                        if (!grid.at(row-1).at(col))
                        {
                            taskparm.row = row-1;
                            taskparm.col = col;
                            q.enqueue(ptok, taskparm);
                        }
                    }
                    grid.at(row).at(col)=false;
                }
            }
        }
    }
}

void TakeStep(std::vector<std::vector<bool>> &grid, mpz_class &counter, const int &startrow, const int &length, int row, int col, int remaining)
{
    if (!grid.at(row).at(col))
    {
        if (row==startrow+1 && col==1 && remaining==1)
        {
            ++counter;
        }
        else
        {
            if (abs(row-startrow-1)+abs(col-1) <= remaining)
            {
                if (row==startrow+1 && col==1 && remaining>1);
                else
                {
                    grid.at(row).at(col)=true;

                    TakeStep(grid,counter,startrow,length,row,col+1,remaining-1);
                    TakeStep(grid,counter,startrow,length,row+1,col,remaining-1);
                    TakeStep(grid,counter,startrow,length,row,col-1,remaining-1);
                    TakeStep(grid,counter,startrow,length,row-1,col,remaining-1);

                    grid.at(row).at(col)=false;
                }
            }
        }
    }
}

void IncreaseCCC(mpz_class &Incr)
{
    std::lock_guard<std::mutex> lock(tex);
    CCCounter=CCCounter+Incr;
}

mpz_class ReadCCC()
{
    std::lock_guard<std::mutex> lock(tex);
    return CCCounter;
}

int checkInput()
{
    int n = 0;
    //prompt user for length
    std::cout << "Enter the length in steps of the SAP (integer, even, larger or equal to 4) : ";
    std::cin  >> n;

    //while the length doesn't meet the requirements, keep asking.
    while ((n % 2) != 0 || n < 4) {
        std::cout << "Error, Wrong number! Please enter an even integer, larger or equal to 4 : ";
        std::cin  >> n;
    }
    return n;
}

void WorkerFunc()
{
    mpz_class counter;
    Parameters recvparm;
    while (q.try_dequeue_from_producer(ptok, recvparm))
    {
        TakeStep(recvparm.grid,counter,recvparm.startrow,recvparm.length,recvparm.row,recvparm.col,recvparm.remaining);
    }
    IncreaseCCC(counter);
}

void HostWorkerFunc(unsigned int n, bool quiet, int d)
{
    mpz_class counter;
    Parameters recvparm;
    int i=0;

    if (n==0)
    {
        n=1;
    }

    while (q.try_dequeue_from_producer(ptok, recvparm))
    {
        TakeStep(recvparm.grid,counter,recvparm.startrow,recvparm.length,recvparm.row,recvparm.col,recvparm.remaining);
        if (++i%d==0 && !quiet)
        {
            std::cout << q.size_approx() << "/" << n << " " << (1.0-((float)q.size_approx()/(float)n))*100.0 << "%" << std::endl;
        }
    }
    IncreaseCCC(counter);
}

int main(int argc,char *argv[])
{
    int SAPlength = 0;
    bool quiet = false;
    bool superquiet = false;
    int depth=7;
    int ReportRate = 30;
    std::ofstream outputFile;
    outputFile.open("SAPsLog.csv", std::ios::out | std::ios::app);

    //check if length was an argument or if it is still necessary to ask it in runtime
    if (outputFile.is_open())
    {
        if (argc <= 1)
        {
            SAPlength = checkInput();
            std::cout << "SAP length is: " << SAPlength << std::endl << std::endl;
        }
        else
        {
            std::istringstream ss(argv[1]);
            int x;
            if (!(ss >> x))
            {
                SAPlength = checkInput();
                std::cout << "SAP length is: " << SAPlength << std::endl << std::endl;
            }
            else
            {
                SAPlength = x;
            }
            if (argc > 2)
            {
                std::string arg2(argv[2]);
                if (arg2=="-q")
                {
                    quiet=true;
                }
                if (arg2=="-qq")
                {
                    superquiet=true;
                }
            }

        }

        if (!quiet && !superquiet)
        {
            std::cout << "Initializing startup variables" << std::endl;
        }

        //calculate the dimensions of the grid the program is going to walk over.
        int width=SAPlength/2+2;
        int height;
        int startrow;
        if (SAPlength==4) //exception for length 4 because else the grid will be too small
        {
            height=4;
            startrow=1;
        }
        else
        {
            height=SAPlength/2+(2+(SAPlength-6)/2);
            startrow=(SAPlength-6)/2+1;
        }

        if (SAPlength>=28)
        {
            ReportRate = 15;
        }
        if (SAPlength>=34)
        {
            ReportRate = 5;
        }

        std::vector<std::thread> threads;
        mpz_class counter;

        //initialize grid and set primary off-limits fields
        if (!quiet && !superquiet)
        {
            std::cout <<width << "x" << height << " Grid initializing...";
        }
        std::vector< std::vector< bool > > grid ( height, std::vector<bool> ( width, 0 ) );

        initForbidden(grid,SAPlength);

        std::vector< std::vector< int > > lookuptable ( height, std::vector<int> ( width, 0 ) );

        FillLookupTable(lookuptable, SAPlength);

        if (!quiet && !superquiet)
        {
            std::cout << "  Complete"<< std::endl;
            printVector(grid);
            printVector(lookuptable);
            std::cout << "Initialize queue...";
            std::cout << "  Complete"<< std::endl;
            std::cout << "Produce and Queue subjobs...";
        }

        //Set first field to forbidden and force first step to right
        grid.at(startrow).at(1)=true;

        ProduceStep(grid,counter,startrow,SAPlength,startrow,2,SAPlength-1,depth);

        IncreaseCCC(counter);

        if (!quiet && !superquiet)
        {
            std::cout << "  Complete"<< std::endl;
            std::cout << "Approximately " << q.size_approx() << " items in queue" <<std::endl;
            std::cout << "Starting Consumer Threads..."<<std::endl;
        }

        unsigned int queueSize = q.size_approx();

        unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
        unsigned concurentThreadsUsed = 0;

        if (queueSize > 2512)
        {
            switch(concurentThreadsSupported)
            {
                case 0:
                    if (!quiet && !superquiet)
                    {
                        std::cout << "Request for amount of threads failed, spawning 0 additional threads for safety" <<std::endl;
                    }
                    break;
                case 1:
                    if (!quiet && !superquiet)
                    {
                        std::cout << "This Machine reports: 1 available thread, 0 additional threads will be spawned" <<std::endl;
                    }
                    break;
                default:
                    concurentThreadsUsed = concurentThreadsSupported-1;
                    if (!quiet && !superquiet)
                    {
                        std::cout << "This Machine reports: "<< concurentThreadsSupported << " available threads." <<std::endl;
                        std::cout << concurentThreadsUsed << " additional Threads will be spawned (main thread will also occupy one thread)." <<std::endl;
                    }
            }


            for (unsigned int i = 0; i<concurentThreadsUsed; ++i)
            {
                if (!quiet && !superquiet)
                {
                    std::cout << "Thread " << i+1 << "/" << concurentThreadsUsed << "spawned" <<std::endl;
                }
                threads.emplace_back(std::thread(WorkerFunc));
            }

            if (!quiet && !superquiet)
            {
                std::cout << "  Complete"<<std::endl;
                std::cout << "Waiting for Threads to Finish..." << std::endl;
            }
        }

        HostWorkerFunc(queueSize, superquiet, ReportRate);

        for(auto& thread : threads)
        {
            thread.join();
        }

        if (!quiet && !superquiet)
        {
            std::cout << "Finished!" << std::endl << "Total Correct Paths: ";
        }

        std::cout << ReadCCC() << std::endl;
        outputFile << SAPlength << ";" << ReadCCC() << std::endl;
        //end program successfully
        if (!quiet && !superquiet)
        {
            std::cout << "Program finished"<< std::endl;
        }
    }

    outputFile.close();

    return 0;
}

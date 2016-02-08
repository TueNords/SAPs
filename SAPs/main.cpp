#include "main.hpp"

std::mutex tex;
mpz_class CCCounter;
moodycamel::ConcurrentQueue<Parameters> q;
moodycamel::ProducerToken ptok(q);

template <typename VecT>
void printVector(VecT &grid)
{
    int width=0;
    if (grid.at(grid.size()-1).at(grid.at(0).size()-1)>9)
		width=2;
    if (grid.at(grid.size()-1).at(grid.at(0).size()-1)>99)
		width=3;	
    
    
    for ( unsigned int i = 0; i < grid.size(); ++i )
        {
            for ( unsigned int j = 0; j < grid.at(i).size(); ++j )
            {
                std::cout << std::setfill(' ') << std::setw(width) << grid.at(i).at(j) << " ";
            }
            std::cout << std::endl;
        }
}

void InitMaps(std::vector<std::vector<int>> &lookuptable, std::vector<std::vector<int>> &grid, const unsigned int length)
{
    unsigned int starty=(length-6)/2+1;
    
    if (length==4)
		starty=1;

    for (size_t i = 0; i < lookuptable.size(); ++i )
    {
        grid.at(i).at(0)=1;
        grid.at(i).at(grid.at(i).size()-1)=1;
        for (size_t j = 0; j < lookuptable.at(i).size(); ++j)
        {
            int temp=1;
			
			grid.at(0).at(j)=1;
            grid.at(grid.size()-1).at(j)=1;
			
            if (i == starty && j==1)
            {
				lookuptable.at(i).at(j)=0;
			}
            else
            {           
				if (i<starty && j==2)
				{
					temp=3;
				}
				
				if (i<starty && j==1)
				{
					grid.at(i).at(j)=1;
				}
				
				int value = abs(j-1)+abs(i-starty-1)+temp;
				lookuptable.at(i).at(j)=value;
				
				if (static_cast<unsigned int>(abs(i-starty)+abs(j-1)+value)>length)
					grid.at(i).at(j)=1;
			}
        }
    }
}

bool CheckStep(std::vector< std::vector< int > > &lookuptable, std::vector<std::vector<int>> &grid, mpz_class &counter,unsigned int row,unsigned int col, int remaining)
{
    if (!grid[row][col])
    {
        if (lookuptable[row][col]==1)
        {
            if (remaining==1)
			{
				++counter;
			}
        }
        else
        {
            if (lookuptable[row][col] <= remaining)
            {
				return true;
            }
        }
    }
    return false;
}

void ProduceStep(std::vector< std::vector< int > > &lookuptable, std::vector<std::vector<int>> &grid, mpz_class &counter, const int &length,unsigned int row,unsigned int col, int remaining, int depth)
{
    if (CheckStep(lookuptable,grid,counter,row,col,remaining))
	{
		grid.at(row).at(col)=true;
		if (remaining>=length-depth)
		{
			ProduceStep(lookuptable,grid,counter,length,row,col+1,remaining-1,depth);
			ProduceStep(lookuptable,grid,counter,length,row+1,col,remaining-1,depth);
			ProduceStep(lookuptable,grid,counter,length,row,col-1,remaining-1,depth);
			ProduceStep(lookuptable,grid,counter,length,row-1,col,remaining-1,depth);
		}
		else
		{
			Parameters taskparm;
			taskparm.grid = grid;
			--remaining;
			if (CheckStep(lookuptable,grid,counter,row,col+1,remaining))
			{
				taskparm.row = row;
				taskparm.col = col+1;
				q.enqueue(ptok, taskparm);
			}
			if (CheckStep(lookuptable,grid,counter,row+1,col,remaining))
			{
				taskparm.row = row+1;
				taskparm.col = col;
				q.enqueue(ptok, taskparm);
			}
			if (CheckStep(lookuptable,grid,counter,row,col-1,remaining))
			{
				taskparm.row = row;
				taskparm.col = col-1;
				q.enqueue(ptok, taskparm);
			}
			if (CheckStep(lookuptable,grid,counter,row-1,col,remaining))
			{
				taskparm.row = row-1;
				taskparm.col = col;
				q.enqueue(ptok, taskparm);
			}
		}
		grid.at(row).at(col)=false;
	}
}

void TakeStep(std::vector< std::vector< int > > &lookuptable, std::vector<std::vector<int>> &grid, mpz_class &counter,unsigned int row,unsigned int col, int remaining)
{
	grid[row][col]=true;
	--remaining;

	if (CheckStep(lookuptable,grid,counter,row,col+1,remaining))
	{
		TakeStep(lookuptable,grid,counter,row,col+1,remaining);
	}
	if (CheckStep(lookuptable,grid,counter,row+1,col,remaining))
	{
		TakeStep(lookuptable,grid,counter,row+1,col,remaining);
	}
	
	if (row!=1)
	{
		if (CheckStep(lookuptable,grid,counter,row,col-1,remaining))
		{
			TakeStep(lookuptable,grid,counter,row,col-1,remaining);
		}
		if (CheckStep(lookuptable,grid,counter,row-1,col,remaining))
		{
			TakeStep(lookuptable,grid,counter,row-1,col,remaining);
		}
	}

	grid[row][col]=false;
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

void WorkerFunc(std::vector< std::vector< int > > &lookuptable, int remaining, unsigned int n, unsigned int d, bool isHost)
{
    mpz_class counter;
    Parameters recvparm;
    
    mpz_realloc2(counter.get_mpz_t(),256);
    
	if (isHost==1)
	{
		unsigned int i=0;

		if (n==0)
		{
			n=1;
		}

		while (q.try_dequeue_from_producer(ptok, recvparm))
		{
			TakeStep(lookuptable,recvparm.grid,counter,recvparm.row,recvparm.col,remaining);
			if (++i%d==0)
			{
				std::cout << q.size_approx() << "/" << n << " " << (1.0-(static_cast<float>(q.size_approx())/static_cast<float>(n)))*100.0 << "%" << std::endl;
			}
		}
	}
	else
	{
		while (q.try_dequeue_from_producer(ptok, recvparm))
		{
			TakeStep(lookuptable,recvparm.grid,counter,recvparm.row,recvparm.col,remaining);
		}
	}
    IncreaseCCC(counter);
}

void ThreadDivideWork(std::vector< std::vector< int > > &lookuptable, int remaining, unsigned int ReportRate)
{
	unsigned int queueSize = q.size_approx();
	std::vector<std::thread> threads;
	unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
	unsigned concurentThreadsUsed = 0;

	if (queueSize > 2484)
	{
		switch(concurentThreadsSupported)
		{
			case 0:
				std::cout << "Request for amount of threads failed, spawning 0 additional threads for safety" <<std::endl;
				break;
			case 1:
				std::cout << "This Machine reports: 1 available thread, 0 additional threads will be spawned" <<std::endl;
				break;
			default:
				concurentThreadsUsed = concurentThreadsSupported-1;
				std::cout << "This Machine reports: "<< concurentThreadsSupported << " available threads." <<std::endl;
				std::cout << concurentThreadsUsed << " additional Threads will be spawned (main thread will also occupy one thread)." <<std::endl;
		}


		for (unsigned int i = 0; i<concurentThreadsUsed; ++i)
		{
			std::cout << "Thread " << i+1 << "/" << concurentThreadsUsed << "spawned" <<std::endl;
			threads.emplace_back(std::thread(WorkerFunc,std::ref(lookuptable),remaining,0,0,false));
		}

		std::cout << "  Complete"<<std::endl;
		std::cout << "Waiting for Threads to Finish..." << std::endl;

	}

	WorkerFunc(std::ref(lookuptable),remaining,queueSize, ReportRate, true);

	for(auto& thread : threads)
	{
		thread.join();
	}
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

int checkArguments(int &argc, char *argv[])
{
	if (argc <= 1)
	{
		return checkInput();
	}
	else
	{
		std::istringstream ss(argv[1]);
		int x;
		if (!(ss >> x))
		{
			return checkInput();
		}
		else
		{
			return x;
		}
	}
}

int main(int argc,char *argv[])
{
    int depth=7;
    unsigned int ReportRate = 30;
    std::ofstream outputFile;
    outputFile.open("SAPsLog.csv", std::ios::out | std::ios::app);

    //check if length was an argument or if it is still necessary to ask it in runtime
    if (outputFile.is_open())
    {
        int SAPlength = checkArguments(argc, argv);
        
        std::cout << "SAP length is: " << SAPlength << std::endl << std::endl;
		std::cout << "Initializing startup variables" << std::endl;

        //calculate the dimensions of the grid the program is going to walk over.
        unsigned int width=static_cast<unsigned int>(SAPlength/2+2);
        unsigned int height=0;
        unsigned int startrow=0;
        if (SAPlength==4) //exception for length 4 because else the grid will be too small
        {
            height=4;
            startrow=1;
        }
        else
        {
            height=static_cast<unsigned int>(SAPlength/2+(2+(SAPlength-6)/2));
            startrow=static_cast<unsigned int>((SAPlength-6)/2+1);
        }

        if (SAPlength>=28)
        {
            ReportRate = 15;
        }
        if (SAPlength>=34)
        {
            ReportRate = 5;
        }

        //initialize grid and set primary off-limits fields

		std::cout <<width << "x" << height << " Grid initializing...";
        std::vector< std::vector< int > > grid ( height, std::vector<int> ( width, 0 ) );
        std::vector< std::vector< int > > lookuptable ( height, std::vector<int> ( width, 0 ) );

        InitMaps(lookuptable, grid ,static_cast<unsigned int>(SAPlength));

		std::cout << "  Complete"<< std::endl;
		printVector(grid);
		printVector(lookuptable);
		std::cout << "Initialize queue...";
		std::cout << "  Complete"<< std::endl;
		std::cout << "Produce and Queue subjobs...";

        //Set first field to forbidden and force first step to right
        grid.at(startrow).at(1)=true;

        mpz_class counter;
        ProduceStep(lookuptable,grid,counter,SAPlength,startrow,2,SAPlength-1,depth);

        IncreaseCCC(counter);

		std::cout << "  Complete"<< std::endl;
		std::cout << "Approximately " << q.size_approx() << " items in queue" <<std::endl;
		std::cout << "Starting Consumer Threads..."<<std::endl;

        ThreadDivideWork(lookuptable,SAPlength-depth-2,ReportRate);
        
		std::cout << "Finished!" << std::endl << "Total Correct Paths: ";

        std::cout << ReadCCC() << std::endl;
        outputFile << SAPlength << ";" << ReadCCC() << std::endl;
        //end program successfully

        std::cout << "Program finished"<< std::endl;
    }

    outputFile.close();

    return 0;
}

#include <iostream>
#include <thread>

class TestThread : public Thread
{
    private:
       int a;

    public:
        TestThread() {
	    a = 0;
	}

	~TestThread(){}
	virtual void run();
}

void TestThread::run()
{
    a++;
    std::cout<<a<<std::endl;
}

int main(int argc, char* argv[])
{
    TestThread tThread;
    tThread.start();
    tThread.join();
    return 0;
}

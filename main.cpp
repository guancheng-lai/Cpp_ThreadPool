#include <iostream>
#include "SimpleThreadPool.h"
using namespace std;

int main(int argc, char *argv[]) {
    ThreadPool pool;
    int cnt = 0;
    int target = 1000000;
    for (int i = 0; i < target; ++i) {
        pool.enqueueTask([](int &x) { x++; }, std::ref(cnt));
    }

    cin.get();
    pool.stopAll();
    cout << cnt << " should equal to or less then " << target << endl;
}
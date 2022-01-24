//
// Created by Guancheng Lai on 1/23/22.
//

#ifndef SIMPLETHREADPOOL_SIMPLETHREADPOOL_H
#define SIMPLETHREADPOOL_SIMPLETHREADPOOL_H

#include <iostream>
#include <thread>
#include <vector>
#include <queue>

namespace helper {
    template <int... Is>
    struct index {};

    template <int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

    template <int... Is>
    struct gen_seq<0, Is...> : index<Is...> {};
}

template <typename... Ts>
class ExecutionTask {
private:
    std::function<void (Ts...)> f;
    std::tuple<Ts...> args;
public:
    template <typename F, typename... Args>
    ExecutionTask(F&& func, Args&&... args)
            : f(std::forward<F>(func)),
              args(std::forward<Args>(args)...)
    {}

    template <typename... Args, int... Is>
    void func(std::tuple<Args...>& tup, helper::index<Is...>) {
        f(std::get<Is>(tup)...);
    }

    template <typename... Args>
    void func(std::tuple<Args...>& tup) {
        func(tup, helper::gen_seq<sizeof...(Args)>{});
    }

    void execute() {
        func(args);
    }
};

template <typename F, typename... Args>
ExecutionTask<>* createTask(F&& f, Args&&... args) {
    return reinterpret_cast<ExecutionTask<>*>(new ExecutionTask<Args...>(std::forward<F>(f), std::forward<Args>(args)...));
}

class ThreadPool {
public:
    ThreadPool() : ThreadPool(std::thread::hardware_concurrency() - 1) {}

    explicit ThreadPool(size_t poolSize) : currentSize(poolSize), stop(false) {
        threads.reserve(poolSize);
        for (int i = 0; i < poolSize; ++i) {
            threads.emplace_back([this, i] { startThread(i); });
        }

        std::cout << "---------------------------------------------------------------------" << std::endl;
        std::cout << "SimpleThreadPool has started with " << this->currentSize << " workers!" << std::endl;
        std::cout << "---------------------------------------------------------------------" << std::endl;
    };

    ~ThreadPool() {
        std::cout << "---------------------------------------------------------------------" << std::endl;
        std::cout << "SimpleThreadPool has been shut down! Thank you!" << std::endl;
        std::cout << "---------------------------------------------------------------------" << std::endl;
    };

    template <typename F, typename... Args>
    void enqueueTask(F&& f, Args&&... args) {
        {
            std::unique_lock lock(mtx);
            pendingTask.push(createTask(std::forward<F>(f), std::forward<Args>(args)...));
        }
        cond.notify_one();
    }

    void startThread(int threadId) {
        ExecutionTask<>* task;
        while (!stop) {
            {
                std::unique_lock lk(mtx);
                while (!stop && pendingTask.empty()) {
                    cond.wait(lk);
                }
                if (stop) {
                    return;
                }
                if (pendingTask.empty()) {
                    continue;
                }
                task = pendingTask.front();
                pendingTask.pop();
            }
            task->execute();
            delete(task);
        }
    }

    void stopAll() {
        this->stop = true;
        cond.notify_all();
        for (auto &&t : threads) {
            t.join();
        }
    }

    size_t numWorkers() const {
        return currentSize;
    }

    size_t numPendingTask() const {
        // size() of a std container is const, which is thread safe
        return pendingTask.size();
    }

private:
    bool stop;
    size_t currentSize;
    std::vector<std::thread> threads;
    std::queue<ExecutionTask<>*> pendingTask;
    std::mutex mtx;
    std::condition_variable cond;
};

#endif //SIMPLETHREADPOOL_SIMPLETHREADPOOL_H

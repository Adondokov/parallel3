#include <iostream>
#include <queue>
#include <future>
#include <thread>
#include <chrono>
#include <cmath>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <random>
#include <condition_variable>
#include <fstream>
#include <atomic>

template <typename T>
class Server {
public:
    Server() : running(true), next_task_id(0) {}

    void start() {
        server_thread = std::thread(&Server::process_tasks, this);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running = false;
        }
        cond_var.notify_all();
        server_thread.join();
    }

    size_t add_task(std::function<T()> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t task_id = next_task_id++;
        tasks.emplace(task_id, std::async(std::launch::async, task));
        cond_var.notify_one();
        return task_id;
    }

    T request_result(size_t task_id) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var.wait(lock, [this, task_id] { return results.find(task_id) != results.end(); });
        T result = results[task_id];
        results.erase(task_id);
        return result;
    }

private:
    void process_tasks() {
        while (running) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var.wait(lock, [this] { return !tasks.empty() || !running; });
            if (!running && tasks.empty()) return;

            auto task_pair = std::move(tasks.front());
            tasks.pop();
            lock.unlock();

            size_t task_id = task_pair.first;
            T result = task_pair.second.get();

            lock.lock();
            results[task_id] = result;
            cond_var.notify_all();
        }
    }

    std::thread server_thread;
    std::atomic<bool> running;
    size_t next_task_id;

    std::queue<std::pair<size_t, std::future<T>>> tasks;
    std::unordered_map<size_t, T> results;
    std::mutex mutex_;
    std::condition_variable cond_var;
};

void client_sin(Server<double>& server, size_t N) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 2 * M_PI);
    std::vector<size_t> task_ids;

    for (size_t i = 0; i < N; ++i) {
        double arg = dis(gen);
        task_ids.push_back(server.add_task([arg] { return std::sin(arg); }));
    }

    std::ofstream outfile("sin_results.txt");
    for (const auto& id : task_ids) {
        outfile << "Task ID: " << id << ", Result: " << server.request_result(id) << "\n";
    }
}

void client_sqrt(Server<double>& server, size_t N) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 100);
    std::vector<size_t> task_ids;

    for (size_t i = 0; i < N; ++i) {
        double arg = dis(gen);
        task_ids.push_back(server.add_task([arg] { return std::sqrt(arg); }));
    }

    std::ofstream outfile("sqrt_results.txt");
    for (const auto& id : task_ids) {
        outfile << "Task ID: " << id << ", Result: " << server.request_result(id) << "\n";
    }
}

void client_pow(Server<double>& server, size_t N) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 10);
    std::vector<size_t> task_ids;

    for (size_t i = 0; i < N; ++i) {
        double base = dis(gen);
        double exp = dis(gen);
        task_ids.push_back(server.add_task([base, exp] { return std::pow(base, exp); }));
    }

    std::ofstream outfile("pow_results.txt");
    for (const auto& id : task_ids) {
        outfile << "Task ID: " << id << ", Result: " << server.request_result(id) << "\n";
    }
}

int main() {
    Server<double> server;
    server.start();

    const size_t N = 100; // Number of tasks per client
    std::thread client1(client_sin, std::ref(server), N);
    std::thread client2(client_sqrt, std::ref(server), N);
    std::thread client3(client_pow, std::ref(server), N);

    client1.join();
    client2.join();
    client3.join();

    server.stop();

    return 0;
}

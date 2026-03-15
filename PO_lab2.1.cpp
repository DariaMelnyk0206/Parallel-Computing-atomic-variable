#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <climits>

using namespace std;
const int THREADS = 6;

pair<long long, int> sequential(const vector<int>& arr)
{
    long long sum = 0;
    int min_even = INT_MAX;

    for (int x : arr)
    {
        if (x % 2 == 0)
        {
            sum += x;

            if (x < min_even)
                min_even = x;
        }
    }

    return { sum, min_even };
}

mutex mtx;
long long sum_mutex;
int min_even_mutex;

void worker_mutex(const vector<int>& arr, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        if (arr[i] % 2 == 0)
        {
            lock_guard<mutex> lock(mtx);

            sum_mutex += arr[i];

            if (arr[i] < min_even_mutex)
                min_even_mutex = arr[i];
        }
    }
}

pair<long long, int> mutex_version(const vector<int>& arr)
{
    sum_mutex = 0;
    min_even_mutex = INT_MAX;

    vector<thread> threads;

    int block = arr.size() / THREADS;

    for (int i = 0; i < THREADS; i++)
    {
        int start = i * block;
        int end = (i == THREADS - 1) ? arr.size() : start + block;

        threads.emplace_back(worker_mutex, ref(arr), start, end);
    }

    for (auto& t : threads)
        t.join();

    return { sum_mutex, min_even_mutex };
}

atomic<long long> sum_atomic;
atomic<int> min_even_atomic;

void worker_atomic(const vector<int>& arr, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        if (arr[i] % 2 == 0)
        {
            sum_atomic.fetch_add(arr[i]);

            int current = min_even_atomic.load();

            while (arr[i] < current &&
                !min_even_atomic.compare_exchange_weak(current, arr[i]))
            {
            }
        }
    }
}

pair<long long, int> atomic_version(const vector<int>& arr)
{
    sum_atomic = 0;
    min_even_atomic = INT_MAX;

    vector<thread> threads;

    int block = arr.size() / THREADS;

    for (int i = 0; i < THREADS; i++)
    {
        int start = i * block;
        int end = (i == THREADS - 1) ? arr.size() : start + block;

        threads.emplace_back(worker_atomic, ref(arr), start, end);
    }

    for (auto& t : threads)
        t.join();

    return { sum_atomic, min_even_atomic };
}

int main()
{
    vector<int> sizes = { 1000000000 };

    for (int N : sizes)
    {
        cout << "\nArray size: " << N << "\n";

        vector<int> arr(N);

        for (int i = 0; i < N; i++)
            arr[i] = rand() % 1000;

        // Sequential

        auto start = chrono::high_resolution_clock::now();
        auto result_seq = sequential(arr);
        auto end = chrono::high_resolution_clock::now();

        cout << "Sequential\n";
        cout << "Sum = " << result_seq.first << endl;
        cout << "Min even = " << result_seq.second << endl;
        cout << "Time = "
            << chrono::duration<double>(end - start).count()
            << " sec\n\n";

        // Mutex

        start = chrono::high_resolution_clock::now();
        auto result_mutex = mutex_version(arr);
        end = chrono::high_resolution_clock::now();

        cout << "Mutex\n";
        cout << "Sum = " << result_mutex.first << endl;
        cout << "Min even = " << result_mutex.second << endl;
        cout << "Time = "
            << chrono::duration<double>(end - start).count()
            << " sec\n\n";

        // Atomic

        start = chrono::high_resolution_clock::now();
        auto result_atomic = atomic_version(arr);
        end = chrono::high_resolution_clock::now();

        cout << "Atomic (CAS)\n";
        cout << "Sum = " << result_atomic.first << endl;
        cout << "Min even = " << result_atomic.second << endl;
        cout << "Time = "
            << chrono::duration<double>(end - start).count()
            << " sec\n";
    }
}
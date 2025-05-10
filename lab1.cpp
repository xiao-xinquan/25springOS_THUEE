#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>

using namespace std;

struct Customer {
    int id;
    int arrival_time; // in units
    int service_time; // in units
    int start_service_time;
    int end_service_time;
    int teller_id;
};

class Semaphore {
private:
    mutex mtx;
    condition_variable cv;
    int count;

public:
    Semaphore(int init = 0) : count(init) {}

    void P() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return count > 0; });
        --count;
    }

    void V() {
        lock_guard<mutex> lock(mtx);
        ++count;
        cv.notify_one();
    }
};

mutex queue_mutex;
queue<Customer*> waiting_queue;

int num_tellers = 2;
vector<Customer> customers;
chrono::time_point<chrono::steady_clock> start_time;

int TIME_UNIT_MS = 200; // adjustable time unit

Semaphore teller_available(num_tellers); 
Semaphore customer_ready(0);
mutex cout_mutex;

int get_relative_time_unit() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time).count() / TIME_UNIT_MS;
}

void teller_function(int teller_id) {
    while (true) {
        customer_ready.P();

        Customer* cust = nullptr;
        {
            lock_guard<mutex> lock(queue_mutex);
            cust = waiting_queue.front();
            waiting_queue.pop();
        }

        if (cust == nullptr) return;

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "at " << get_relative_time_unit() << " units: teller " << teller_id << " is calling customer " << cust->id << endl;
        }

        teller_available.P();

        cust->start_service_time = max(get_relative_time_unit(), cust->arrival_time);
        cust->end_service_time = cust->start_service_time + cust->service_time;
        cust->teller_id = teller_id;

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "at " << cust->start_service_time << " units: teller " << teller_id << " starts serving customer " << cust->id << endl;
        }

        this_thread::sleep_for(chrono::milliseconds(cust->service_time * TIME_UNIT_MS));

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "at " << cust->end_service_time << " units: customer " << cust->id << " leaves teller " << teller_id << endl;
        }

        teller_available.V();
    }
}

void customer_arrival() {
    for (Customer& cust : customers) {
        this_thread::sleep_until(start_time + chrono::milliseconds(cust.arrival_time * TIME_UNIT_MS));

        {
            lock_guard<mutex> lock(queue_mutex);
            waiting_queue.push(&cust);
        }

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "at " << get_relative_time_unit() << " units: customer " << cust.id << " arrived and took number" << endl;
        }

        customer_ready.V();
    }

    for (int i = 0; i < num_tellers; ++i) {
        teller_available.P();
        {
            lock_guard<mutex> lock(queue_mutex);
            waiting_queue.push(nullptr);
        }
        customer_ready.V();
    }
}

void load_customers(const string& filename) {
    ifstream infile(filename);
    int id, arrival, service;
    while (infile >> id >> arrival >> service) {
        customers.push_back({id, arrival, service, -1, -1, -1});
    }
}

void write_results(const string& filename) {
    ofstream result_file(filename);
    result_file << "ID\tArrival (unit)\tStart (unit)\tEnd (unit)\tTeller" << endl;

    sort(customers.begin(), customers.end(), [](const Customer& a, const Customer& b) {
        return a.id < b.id;
    });

    for (const Customer& c : customers) {
        result_file << c.id << "\t"
                    << c.arrival_time << "\t"
                    << c.start_service_time << "\t"
                    << c.end_service_time << "\t"
                    << c.teller_id << endl;
    }

    result_file.close();
}

int main() {
    load_customers("lab1.txt");
    start_time = chrono::steady_clock::now();

    vector<thread> tellers;
    for (int i = 0; i < num_tellers; ++i) {
        tellers.emplace_back(teller_function, i + 1);
    }

    thread customer_thread(customer_arrival);

    customer_thread.join();
    for (thread& t : tellers) {
        t.join();
    }

    write_results("lab1_result.txt");
    return 0;
}

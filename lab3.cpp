#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <cstdlib>

using namespace std;

const int TOTAL_MEMORY = 1024; // KB

struct Block {
    int start;
    int size;
    int id;
    bool is_free;
    Block* next;

    Block(int s, int sz, bool f) : start(s), size(sz), is_free(f), next(nullptr), id(-1) {}
};

class MemoryManager {
    Block* head;
    Block* last_alloc_ptr;
    ofstream csv_out;
    int step;
    
public:
    enum Strategy { FIRST_FIT, NEXT_FIT, BEST_FIT, WORST_FIT } strategy;

    MemoryManager(Strategy strat) : strategy(strat), last_alloc_ptr(nullptr), step(0) {
        head = new Block(0, TOTAL_MEMORY, true);
        last_alloc_ptr = head;
        csv_out.open("lab3.csv");
        write_snapshot();
    }

    ~MemoryManager() {
        Block* cur = head;
        while (cur) {
            Block* next = cur->next;
            delete cur;
            cur = next;
        }
        csv_out.close();
    }
    

    void write_snapshot() {
        csv_out << step++;
        for (Block* cur = head; cur != nullptr; cur = cur->next) {
            csv_out << "," << cur->start << "," << cur->size << "," << (cur->is_free ? 1 : 0);
        }
        csv_out << endl;
    }

    bool allocate(int id, int size) {
        for (Block* cur = head; cur != nullptr; cur = cur->next) {
            if (!cur->is_free && cur->id == id) {
                cout << "ID already allocated: " << id << endl;
                return false;
            }
        }


        Block* best = nullptr;
        Block* prev = nullptr;
        Block* cur = head;
        Block* prev_best = nullptr;

        switch (strategy) {
            case FIRST_FIT:
                while (cur) {
                    if (cur->is_free && cur->size >= size) {
                        best = cur;
                        break;
                    }
                    cur = cur->next;
                }
                break;

            case NEXT_FIT: {
                if (!last_alloc_ptr) last_alloc_ptr = head;
            
                Block* cur = last_alloc_ptr->next ? last_alloc_ptr->next : head;
                Block* start = cur;
                bool first_iteration = true;
            
                do {
                    if (cur->is_free && cur->size >= size) {
                        best = cur;
                        break;
                    }
                    cur = cur->next ? cur->next : head;
                } while (cur != start || first_iteration);
                first_iteration = false;
                break;
            }
                
                
            case BEST_FIT:
                while (cur) {
                    if (cur->is_free && cur->size >= size) {
                        if (!best || cur->size < best->size) {
                            best = cur;
                        }
                    }
                    cur = cur->next;
                }
                break;
            case WORST_FIT:
                while (cur) {
                    if (cur->is_free && cur->size >= size) {
                        if (!best || cur->size > best->size) {
                            best = cur;
                        }
                    }
                    cur = cur->next;
                }
                break;
        }

        if (!best) {
            cout << "No suitable block found for size: " << size << endl;
            csv_out << step++ << ",-1,-1,0" << endl;
            last_alloc_ptr = head;  // Reset fallback
            return false;
        }

        if (best->size > size) {
            Block* new_block = new Block(best->start + size, best->size - size, true);
            new_block->next = best->next;
            best->next = new_block;
        }
        best->size = size;
        best->is_free = false;
        best->id = id;
        last_alloc_ptr = best; 
        write_snapshot();
        return true;
    }

    void release(int id) {
        Block* cur = head;
        while (cur) {
            if (!cur->is_free && cur->id == id) {
                cur->id = -1;
                cur->is_free = true;

                Block* prev = nullptr;
                Block* scan = head;
                while (scan && scan != cur) {
                    prev = scan;
                    scan = scan->next;
                }
            
                if (prev && prev->is_free) {
                    prev->size += cur->size;
                    prev->next = cur->next;
                    delete cur;
                    cur = prev; 
                }

                merge();
                this->last_alloc_ptr = head; 
                cur->id = -1;
                write_snapshot();
                return;
            }            
            cur = cur->next;
        }
        cout << "No block found to release with ID: " << id << endl;
    }

    void merge() {
        Block* cur = head;
        while (cur && cur->next) {
            if (cur->is_free && cur->next->is_free) {
                Block* tmp = cur->next;
                cur->size += tmp->size;
                cur->next = tmp->next;
                cur->id = -1; 
                delete tmp;
            } else {
                cur = cur->next;
            }
        }
    }
};

MemoryManager::Strategy parse_strategy(const string& s) {
    if (s == "FIRST_FIT") return MemoryManager::FIRST_FIT;
    if (s == "NEXT_FIT") return MemoryManager::NEXT_FIT;
    if (s == "BEST_FIT") return MemoryManager::BEST_FIT;
    if (s == "WORST_FIT") return MemoryManager::WORST_FIT;
    return MemoryManager::FIRST_FIT;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: ./lab3 <FIRST_FIT | NEXT_FIT | BEST_FIT | WORST_FIT>" << endl;
        return 1;
    }

    MemoryManager manager(parse_strategy(argv[1]));
    ifstream fin("lab3.txt");
    string line;
    while (getline(fin, line)) {
        stringstream ss(line);
        string cmd;
        ss >> cmd;
        if (cmd == "ALLOC") {
            int id, sz; ss >> id >> sz;
            bool success = false;
            success = manager.allocate(id, sz);
            if (!success) {
                break;
            }
        } else if (cmd == "FREE") {
            int id; ss >> id;
            manager.release(id);
        }
    }
    fin.close();

    system("python3 lab3_visualize.py");

    return 0;
}

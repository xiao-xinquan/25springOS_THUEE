#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <algorithm>
#include <climits>
#include <semaphore.h>
#include <sys/stat.h>

#define SIZE 1000000
#define THRESHOLD 10000
#define MAX_PROCESSES 20
#define INPUT_FILE "lab2.txt"
#define OUTPUT_FILE "lab2_sorted.txt"
#define SHM_NAME "/lab2_shm_tree"

// int process_count = 1;


int partition(int* arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (arr[j] <= pivot) {
            ++i;
            std::swap(arr[i], arr[j]);
        }
    }
    std::swap(arr[i + 1], arr[high]);
    return i + 1;
}

void quicksort(int* arr, int low, int high, int* process_count, sem_t* sem) {
    if (low >= high) return;
    if (high - low + 1 <= THRESHOLD) {
        std::sort(arr + low, arr + high + 1);
        return;
    }

    int p = partition(arr, low, high);

    pid_t left_pid = -1, right_pid = -1;
    bool left_forked = false, right_forked = false;
    
    // 左侧分支
    sem_wait(sem);
    if (*process_count < MAX_PROCESSES) {
        ++(*process_count);
        sem_post(sem);
        left_pid = fork();
        if (left_pid == 0) {
            quicksort(arr, low, p - 1, process_count, sem);
            exit(0);
        }
        left_forked = true;
    } else {
        sem_post(sem);
        quicksort(arr, low, p - 1, process_count, sem);
    }
    
    // 右侧分支
    sem_wait(sem);
    if (*process_count < MAX_PROCESSES) {
        ++(*process_count);
        sem_post(sem);
        right_pid = fork();
        if (right_pid == 0) {
            quicksort(arr, p + 1, high, process_count, sem);
            exit(0);
        }
        right_forked = true;
    } else {
        sem_post(sem);
        quicksort(arr, p + 1, high, process_count, sem);
    }

    std::cout << "Current process count: " << *process_count << std::endl;

    
    // 等待子进程并回收进程数
    if (left_forked) {
        waitpid(left_pid, NULL, 0);
        sem_wait(sem);
        --(*process_count);
        sem_post(sem);
    }
    if (right_forked) {
        waitpid(right_pid, NULL, 0);
        sem_wait(sem);
        --(*process_count);
        sem_post(sem);
    }
    
}

void generate_random_file() {
    std::ofstream fout(INPUT_FILE);
    if (!fout) {
        std::cerr << "Cannot create " << INPUT_FILE << std::endl;
        exit(1);
    }
    srand(time(NULL));
    for (int i = 0; i < SIZE; ++i) {
        fout << rand() % 1000000 << "\n";
    }
    fout.close();
    std::cout << "✅ Generated " << INPUT_FILE << " with " << SIZE << " random integers." << std::endl;
}

int main() {
    int shm_fd_pc = shm_open("/process_count_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd_pc, sizeof(int));
    int* process_count = (int*) mmap(NULL, sizeof(int),
        PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_pc, 0);
    *process_count = 1;

    sem_t* sem = sem_open("/process_count_sem", O_CREAT, 0666, 1);


    generate_random_file();

    std::vector<int> data(SIZE);
    std::ifstream fin(INPUT_FILE);
    if (!fin) {
        std::cerr << "Failed to open " << INPUT_FILE << std::endl;
        exit(1);
    }
    for (int i = 0; i < SIZE; ++i) fin >> data[i];
    fin.close();
    std::cout << "✅ Loaded data from " << INPUT_FILE << "." << std::endl;

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE * sizeof(int));
    int* shm_ptr = (int*)mmap(NULL, SIZE * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    std::copy(data.begin(), data.end(), shm_ptr);

    quicksort(shm_ptr, 0, SIZE - 1, process_count, sem);
    std::cout << "✅ Sorting completed." << std::endl;

    std::ofstream fout(OUTPUT_FILE);
    for (int i = 0; i < SIZE; ++i)
        fout << shm_ptr[i] << "\n";
    fout.close();
    std::cout << "✅ Sorted data written to " << OUTPUT_FILE << "." << std::endl;

    munmap(shm_ptr, SIZE * sizeof(int));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    // 释放共享内存映射
    munmap(process_count, sizeof(int));
    close(shm_fd_pc);
    shm_unlink("/process_count_shm");

    // 释放信号量
    sem_close(sem);
    sem_unlink("/process_count_sem");
    return 0;
}
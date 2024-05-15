#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <future>


void matrixVectorMultiply(const std::vector<std::vector<double>>& matrix, const std::vector<double>& vector, std::vector<double>& result, int startRow, int endRow) {
    int numRows = matrix.size();
    int numCols = matrix[0].size();
    for (int i = startRow; i < endRow; ++i) {
        double sum = 0.0;
        for (int j = 0; j < numCols; ++j) {
            sum += matrix[i][j] * vector[j];
        }
        result[i] = sum;
    }
}

void initialize(std::vector<std::vector<double>>& matrix, std::vector<double>& vector, int N) {
    
    if (matrix.size() < N || matrix[0].size() < N) {
        matrix.resize(N, std::vector<double>(N, 0.0));
    }
    if (vector.size() < N) {
        vector.resize(N, 0.0);
    }

   
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) {
                matrix[i][j] = 2.0; 
            } else {
                matrix[i][j] = 1.0; 
            }
        }
        vector[i] = N + 1.0; 
    }
}


int main() {
    const int numThreads[] = {1, 2, 4, 7, 8, 16, 20, 40};
    const int numMatrices[] = {20000, 40000};
    
    for (int n : numMatrices) {
        std::vector<std::vector<double>> matrix;
        std::vector<double> vector;
        std::vector<double> result(n);

        initialize(matrix, vector, n);
        
        std::cout << "Matrix size: " << n << "x" << n << std::endl;
        std::cout << "Number of Threads | Time (seconds)" << std::endl;
        
        for (int numThread : numThreads) {
            auto startTime = std::chrono::high_resolution_clock::now();
            
            std::vector<std::future<void>> futures;
            futures.reserve(numThread);
            
            int rowsPerThread = n / numThread;
            int remainingRows = n % numThread;
            int startRow = 0;
            int endRow = 0;
            
            for (int i = 0; i < numThread; ++i) {
                startRow = endRow;
                endRow = startRow + rowsPerThread + (i < remainingRows ? 1 : 0);
                futures.emplace_back(std::async(std::launch::async, matrixVectorMultiply, std::ref(matrix), std::ref(vector), std::ref(result), startRow, endRow));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;
            std::cout << "       " << numThread << "        |     " << duration.count() << std::endl;
        }
        std::cout << std::endl;
    }
    return 0;
}

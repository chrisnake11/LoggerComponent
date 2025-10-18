// LoggerComponent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "Logger.h"
#include <chrono>
int main() {
	
	int total = 1000000;
	int thread_count = 10;
	int per_thread_count = total / thread_count;

	std::vector<std::thread> threads;
	
	auto start = std::chrono::high_resolution_clock::now();

	for (int t = 0; t < thread_count; t++) {
		threads.emplace_back([t, per_thread_count]() {
			int start_index = t * per_thread_count;
			int end_index = start_index + per_thread_count;
			for (int i = start_index; i < end_index; i++) {
				Logger::getInstance()->log(LogLevel::INFO, "Logging message number: {}", i);
			}
		});
	}

	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	std::cout << "Logging completed in " << duration_ms << " ms" << std::endl;
	return 0;
}
//
// Created by zy on 22-7-15.
//

#ifndef THREADPOOL_STATS_H
#define THREADPOOL_STATS_H

#include <cstdint>
#include <cstdio>
#include <vector>
#include <numeric>
#include <algorithm>
#include <complex>
#include <fstream>
#include <cassert>

void log_timeInCPUCircles(int id, uint64_t time_all, uint64_t time_read, uint64_t time_write) {
    // 打印cpu时间
    printf("Thread %d, time_all: %lu, time_read: %lu, time_write: %lu\n", id, time_all, time_read, time_write);
}

void print_mean_and_std(std::vector<uint64_t> timer_all, std::string timer_type) {
    std::ofstream out("out.txt", std::ofstream::app);

    std::vector<double> diff(timer_all.size());
    double sum = std::accumulate(timer_all.begin(), timer_all.end(), 0.0);  // 注意0.0，而不是0，很关键，涉及到变量自动转化，double尽量0.0
    double mean = sum / timer_all.size();
    std::transform(timer_all.begin(), timer_all.end(), diff.begin(), [mean] (double x) {return x-mean;});
    double stddev = std::sqrt(std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0) / timer_all.size());

    uint64_t sum_uSec = sum / (2.9 * 1000000000) * 1000000;
    uint64_t mean_uSec = mean / (2.9 * 1000000000) * 1000000;
    uint64_t stddev_uSec = stddev / (2.9 * 1000000000) * 1000000;

    out << "Timer " << timer_type.c_str() << ", sum: " << sum_uSec << ", MEAN: " << (uint64_t) mean_uSec << ", STDDEV: " << stddev_uSec << std::endl;

    out.close();
}



#endif //THREADPOOL_STATS_H

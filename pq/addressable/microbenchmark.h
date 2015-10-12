#pragma once

#include <random>
#include <type_traits>
#include <vector>
#include <utility>

#include "../../common/benchmark.h"
#include "../../common/benchmark_util.h"
#include "../../common/contenders.h"

namespace pq {

namespace addressable {

template <typename PQ>
class microbenchmark {
public:
    using Configuration = std::pair<size_t, size_t>;
    using Benchmark = common::benchmark<PQ, Configuration>;
    using BenchmarkFactory = common::contender_factory<Benchmark>;
    using T = typename PQ::value_type;

    static void* fill_data_permutation(PQ&, Configuration config, void*) {
        return common::util::fill_data_permutation<T>(
            config.first, config.second);
    }

    template <int factor=1>
    static void* fill_data_random(PQ&, Configuration config, void*) {
        return common::util::fill_data_random<T>(
            factor*config.first, config.second);
    }

    template <int factor = 1>
    static void* fill_heap_random(PQ& queue, Configuration config, void*) {
        std::mt19937 random{config.second};
        size_t size = factor * config.first;
        for (size_t i = 0; i < size; ++i)
            queue.push(random());
        return nullptr;
    }

    struct heap_data
    {
        std::vector<void*> handles;
        T* data;
    };

    template <int factor = 1, int pops = 1>
    static void* fill_heap_random(PQ& queue, Configuration config, void*) {
        auto* data = new heap_data();

        std::mt19937 random{config.second};
        size_t size = factor * config.first;
        for (size_t i = 0; i < size; ++i)
        {
            data->handles.push_back(queue.push(random()));
        }
        return common::util::fill_data_random<T>(
            factor*config.first, config.second);
        return static_cast<void*>(data);
    }

    template <int factor = 1, int pops = 1>
    static void* fill_heap_random_and_pop(PQ& queue, Configuration config, void* cb)
    {
        void* data = fill_both_random(queue, config, cb);

        for (size_t i = 0; i < pops; ++i)
        {
            assert(queue.top() + 1 > queue.top());
            queue.push(queue.top() + 1);
            queue.pop();
        }

        return data;
    }

    static void clear_data(PQ&, Configuration, void* data) {
        delete static_cast<heap_data*>(data);
    }

    static void register_benchmarks(common::contender_list<Benchmark> &benchmarks) {
        const std::vector<Configuration> configs{
            std::make_pair(1<<6, 0xDECAF),
            std::make_pair(1<<16, 0xDECAF),
            std::make_pair(1<<18, 0xBEEF),
            std::make_pair(1<<20, 0xC0FFEE),
            //std::make_pair(1<<22, 0xF005BA11),
            //std::make_pair(1<<24, 0xBA5EBA11),
            //std::make_pair(1<<26, 0xCA55E77E)
        };

        common::register_benchmark("push^n modify^n", "p^n m^n",
            microbenchmark::fill_data_random<2>,
            [](PQ &queue, Configuration config, void* ptr) {
                T* data = static_cast<T*>(ptr);
                size_t size = config.first;
                std::vector<typename PQ::handle_type> handles(size);
                for (size_t i = 0; i < size; ++i) {
                    handles[i] = queue.push(data[i]);
                }
                for (size_t i = 0; i < size; ++i) {
                    queue.modify(handles[i], data[size + i]);
                }
            }, microbenchmark::clear_data, configs, benchmarks);

        common::register_benchmark("modify^n on filled heap", "filled-m^n",
            microbenchmark::fill_heap_random<1>,
            [](PQ &queue, Configuration config, void* ptr) {
                heap_data* data = static_cast<heap_data*>(ptr);
                size_t size = config.first;

                for (size_t i = 0; i < size; ++i) {
                    queue.modify(data->handles[i], data->data[i]);
                }
            }, microbenchmark::clear_data, configs, benchmarks);
    }
};
}
}

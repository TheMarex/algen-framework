#pragma once

#include <random>
#include <type_traits>
#include <vector>
#include <utility>

#include "../../common/benchmark.h"
#include "../../common/benchmark_util.h"
#include "../../common/contenders.h"

#include "static_graph.hpp"

namespace pq {

namespace addressable {

struct DijkstraNodeKey
{
    unsigned node_id;
};

std::ostream& operator<<(std::ostream& out, DijkstraNodeKey key)
{
    out << key.node_id;
    return out;
}

template<typename T>
struct DijkstraMetric
{
    using value_type = unsigned;
    using index_type = decltype(T().node_id);

    void reset(unsigned num_nodes)
    {
        distances.resize(num_nodes, std::numeric_limits<value_type>::max());
    }

    // we need to implement std::greater here because we want a min-heap
    bool operator()(T lhs, T rhs) const
    {
        return distances[lhs.node_id] > distances[rhs.node_id];
    }

    value_type operator[](T key) const
    {
        return distances[key.node_id];
    }

    value_type& operator[](T key)
    {
        return distances[key.node_id];
    }

    value_type& operator[](index_type idx)
    {
        return distances[idx];
    }

    std::vector<value_type> distances;
};


template <typename PQ>
class dijkstra_benchmark {
public:
    struct TestEdgeData { unsigned weight; };
    using TestGraph = StaticGraph<TestEdgeData>;
    using Key = DijkstraNodeKey;
    using Metric = DijkstraMetric<Key>;
    using Configuration = std::pair<size_t, size_t>;
    using Benchmark = common::benchmark<PQ, Configuration>;
    using BenchmarkFactory = common::contender_factory<Benchmark>;
    // we assume T is the node id and its sorted by a custom comparator
    using T = typename PQ::value_type;

    template <int factor=1>
    static void* fill_data_random(PQ&, Configuration config, void*) {
        return common::util::fill_data_random<T>(
            factor*config.first, config.second);
    }

    /// create a random undirected unit graph with given maximal degree
    template <int max_degree=5>
    static void* fill_graph_random(PQ&, Configuration config, void*) {
        static_assert(max_degree >= 2, "A connected graph needs at least degree 2");

        std::mt19937 random{config.second};
        size_t num_nodes = config.first;

        // this vector needs to be sorted lexicographical by (source, target)
        using EdgeT = typename TestGraph::InputEdge;
        std::vector<EdgeT> input_edges;

        // create a random spanning tree
        std::vector<unsigned> nodes(num_nodes);
        std::iota(nodes.begin(), nodes.end(), 0);
        std::random_shuffle(nodes.begin(), nodes.end());
        auto current = nodes.begin();
        while (current != nodes.end() && std::next(current) != nodes.end())
        {
            auto next = std::next(current);
            input_edges.emplace_back(*current, *next, TestEdgeData { 1 });
            input_edges.emplace_back(*next, *current, TestEdgeData { 1 });
            current = next;
        }

        std::uniform_int_distribution<int> target_distribution(0, num_nodes-1);
        std::normal_distribution<double> degree_distribution(2, max_degree);
        for (size_t source = 0; source < num_nodes; ++source)
        {
            auto degree = std::max(std::min(1, static_cast<int>(degree_distribution(random))), max_degree);
            std::vector<unsigned> targets;
            // degree for each node is already at least 2
            // because the graph is already a spanning tree
            auto current_degree = source == nodes.front() || source == nodes.back() ? 1 : 2;
            for (int j = current_degree; j <= degree; ++j)
            {
                auto target = target_distribution(random);
                input_edges.emplace_back(source, target, TestEdgeData { 1 });
                input_edges.emplace_back(target, source, TestEdgeData { 1 });
            }
        }

        std::sort(input_edges.begin(), input_edges.end());
        auto new_end = std::remove_if(input_edges.begin(), input_edges.end(), [](const EdgeT& edge) { return edge.source == edge.target; });
        input_edges.erase(new_end, input_edges.end());
        new_end = std::unique(input_edges.begin(), input_edges.end(), [](const EdgeT& lhs, const EdgeT& rhs) { return lhs.source == rhs.source && lhs.target == rhs.target; });
        input_edges.erase(new_end, input_edges.end());

        auto* data = new TestGraph(num_nodes, input_edges);
        return static_cast<void*>(data);
    }

    static void clear_data(PQ&, Configuration, void* data) {
        delete static_cast<TestGraph*>(data);
    }

    static void register_benchmarks(common::contender_list<Benchmark> &benchmarks) {
        const std::vector<Configuration> configs{
            std::make_pair(1<<2, 0xDECAF),
            std::make_pair(1<<6, 0xDECAF),
            std::make_pair(1<<16, 0xDECAF),
            std::make_pair(1<<18, 0xBEEF),
            std::make_pair(1<<20, 0xC0FFEE),
            std::make_pair(1<<22, 0xF005BA11),
        };

        common::register_benchmark("Dijkstra on random graph", "dijkstra",
            dijkstra_benchmark::fill_graph_random<5>,
            [](PQ &queue, Configuration config, void* ptr) {
                auto* graph = static_cast<TestGraph*>(ptr);
                size_t num_nodes = config.first;
                std::vector<void*> handles(num_nodes, nullptr);

                // Bad hack to get acces to the distance array
                auto& metric = *static_cast<typename PQ::comparator_type*>(queue.get_comparator());
                static_assert(std::is_same<typename PQ::comparator_type, DijkstraMetric<DijkstraNodeKey>>(), "This only works with our custom comparator");
                metric.reset(num_nodes);

                // start search at node 0
                metric[0] = 0;
                handles[0] = queue.push(Key{0});
                while (queue.size() > 0)
                {
                    auto source = queue.top().node_id;
                    queue.pop();

                    for (auto edge_id = graph->BeginEdges(source); edge_id < graph->EndEdges(source); ++edge_id)
                    {
                        auto target = graph->GetTarget(edge_id);
                        const auto& data = graph->GetEdgeData(edge_id);
                        auto new_dist = metric[source] + data.weight;
                        if (handles[target] == nullptr)
                        {
                            // Note that we actually don't modify the key, we modify the comparator
                            metric[target] = new_dist;
                            handles[target] = queue.push(Key {target});
                        }
                        else if (new_dist < metric[target])
                        {
                            // Note that we actually don't modify the key, we modify the comparator
                            metric[target] = new_dist;
                            queue.modify_up(handles[target], Key {target});
                        }
                    }
                }
            }, dijkstra_benchmark::clear_data, configs, benchmarks);
    }
};
}
}

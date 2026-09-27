#include <algorithm>
#include <cstddef>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <unordered_map>

using Key = std::string;
using FuncPtr = void (*)(std::istringstream&);

struct LRUCache {
    std::size_t capacity = 0;
    std::size_t hits = 0;

    std::list<Key> order;
    std::unordered_map<Key, std::list<Key>::iterator> pos;

    void set_capacity(std::size_t n) {
        capacity = n;
        while (order.size() > capacity) {
            pos.erase(order.back());
            order.pop_back();
        }
    }

    void access(const Key& key) {
        auto it = pos.find(key);

        if (it != pos.end()) {
            ++hits;
            order.splice(order.begin(), order, it->second);
            return;
        }

        if (capacity != 0 && order.size() >= capacity) {
            pos.erase(order.back());
            order.pop_back();
        }

        order.push_front(key);
        pos[key] = order.begin();
    }
};

struct FIFOCache {
    std::size_t capacity = 0;
    std::size_t hits = 0;

    std::list<Key> order;

    void set_capacity(std::size_t n) {
        capacity = n;
        while (order.size() > capacity) {
            order.pop_front();
        }
    }

    void access(const Key& key) {
        auto it = std::find(order.begin(), order.end(), key);

        if (it != order.end()) {
            ++hits;
            return;
        }

        if (capacity != 0 && order.size() >= capacity) {
            order.pop_front();
        }

        order.push_back(key);
    }
};

struct LFUCache {
    std::size_t capacity = 0;
    std::size_t hits = 0;

    std::list<Key> order;
    std::unordered_map<Key, std::size_t> freq;

    void set_capacity(std::size_t n) {
        capacity = n;
        while (freq.size() > capacity) {
            evict_one();
        }
    }

    void access(const Key& key) {
        auto it = freq.find(key);

        if (it != freq.end()) {
            ++hits;
            ++freq[key];
            return;
        }

        if (capacity != 0 && freq.size() >= capacity) {
            evict_one();
        }

        freq[key] = 1;
        order.push_back(key);
    }

private:
    void evict_one() {
        Key victim;
        std::size_t min_freq = std::numeric_limits<std::size_t>::max();

        for (const auto& key : order) {
            if (freq[key] < min_freq) {
                min_freq = freq[key];
                victim = key;
            }
        }

        freq.erase(victim);
        order.remove(victim);
    }
};

LRUCache lru;
FIFOCache fifo;
LFUCache lfu;

void cmd_cap(std::istringstream& iss) {
    std::size_t capacity = 0;
    iss >> capacity;

    lru.set_capacity(capacity);
    fifo.set_capacity(capacity);
    lfu.set_capacity(capacity);
}

void cmd_access(std::istringstream& iss) {
    Key key;
    iss >> key;

    lru.access(key);
    fifo.access(key);
    lfu.access(key);
}

void cmd_stats(std::istringstream&) {
    std::cout << "lru_hits=" << lru.hits
              << " fifo_hits=" << fifo.hits
              << " lfu_hits=" << lfu.hits << '\n';
}

std::unordered_map<std::string, FuncPtr> cmd_map = {
    {"CAP", cmd_cap},
    {"ACCESS", cmd_access},
    {"STATS", cmd_stats}
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        auto it = cmd_map.find(command);
        if (it != cmd_map.end()) {
            it->second(iss);
        }
    }

    return 0;
}

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <list>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using Key = std::string;
using FuncPtr = void (*)(std::istringstream&);

struct LRUCache {
    std::size_t capacity = 0;

    struct Entry {
        Key key;
        Key value;
        long long expires_at = 0;
        bool has_expiry = false;
    };

    std::list<Entry> order;
    std::unordered_map<Key, std::list<Entry>::iterator> entries;
    long long now = 0;
    std::size_t hits = 0;
    std::size_t misses = 0;
    std::size_t evictions = 0;

    void set_capacity(std::size_t n) {
        capacity = n;
        while (order.size() > capacity) {
            entries.erase(order.back().key);
            order.pop_back();
            ++evictions;
        }
    }

    void set_now(long long time) {
        now = time;
    }

    std::size_t put(const Key& key, const Key& value, long long ttl = 0, bool has_ttl = false) {
        auto it = entries.find(key);
        if (it != entries.end()) {
            if (it->second->has_expiry && it->second->expires_at <= now) {
                order.erase(it->second);
                entries.erase(it);
                it = entries.end();
            }
        }
        if (it != entries.end()) {
            it->second->value = value;
            it->second->has_expiry = has_ttl;
            it->second->expires_at = now + ttl;
            order.splice(order.begin(), order, it->second);
            return 3;
        }

        if (capacity == 0) {
            return 1;
        }

        std::size_t ops = 2;
        if (order.size() >= capacity) {
            entries.erase(order.back().key);
            order.pop_back();
            ++evictions;
            ops = 4;
        }

        order.push_front({key, value, now + ttl, has_ttl});
        entries.emplace(key, order.begin());
        return ops;
    }

    std::size_t get(const Key& key, Key& value) {
        auto it = entries.find(key);
        if (it == entries.end()) {
            ++misses;
            return 1;
        }
        if (it->second->has_expiry && it->second->expires_at <= now) {
            order.erase(it->second);
            entries.erase(it);
            ++misses;
            return 1;
        }
        ++hits;
        value = it->second->value;
        order.splice(order.begin(), order, it->second);
        return 3;
    }

    void print_state() {
        remove_expired();
        bool first = true;
        for (const auto& entry : order) {
            if (!first) {
                std::cout << ' ';
            }
            std::cout << entry.key << '=' << entry.value;
            first = false;
        }
        std::cout << '\n';
    }

private:
    void remove_expired() {
        for (auto it = order.begin(); it != order.end();) {
            if (it->has_expiry && it->expires_at <= now) {
                entries.erase(it->key);
                it = order.erase(it);
            } else {
                ++it;
            }
        }
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

struct DoublyLinkedList {
    struct Node {
        Key key;
        Key value;
        Node* prev = nullptr;
        Node* next = nullptr;
    };

    Node head;
    Node tail;

    DoublyLinkedList() {
        head.next = &tail;
        tail.prev = &head;
    }

    ~DoublyLinkedList() {
        auto* node = head.next;
        while (node != &tail) {
            auto* next = node->next;
            delete node;
            node = next;
        }
    }

    Node* find(const Key& key) const {
        for (auto* node = head.next; node != &tail; node = node->next) {
            if (node->key == key) {
                return node;
            }
        }
        return nullptr;
    }

    void add_front(const Key& key, const Key& value) {
        auto* node = new Node{key, value};
        link_front(node);
    }

    void remove_key(const Key& key) {
        auto* node = find(key);
        if (node) {
            unlink(node);
            delete node;
        }
    }

    void move_front(const Key& key) {
        auto* node = find(key);
        if (node) {
            unlink(node);
            link_front(node);
        }
    }

    void print() const {
        bool first = true;
        for (auto* node = head.next; node != &tail; node = node->next) {
            if (!first) {
                std::cout << ' ';
            }
            std::cout << node->key << '=' << node->value;
            first = false;
        }
        std::cout << '\n';
    }

private:
    void unlink(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void link_front(Node* node) {
        node->prev = &head;
        node->next = head.next;
        head.next->prev = node;
        head.next = node;
    }
};

LRUCache lru;
FIFOCache fifo;
LFUCache lfu;
DoublyLinkedList dll;
bool sharded_mode = false;
bool ttl_mode = false;
bool metrics_mode = false;
bool full_mode = false;
std::size_t hits = 0;
std::size_t misses = 0;

struct Shard {
    std::mutex mutex;
    LRUCache cache;

    explicit Shard(std::size_t capacity) {
        cache.set_capacity(capacity);
    }
};

std::vector<std::unique_ptr<Shard>> shards;

std::size_t shard_for(const Key& key) {
    std::size_t hash = 0;
    for (const unsigned char ch : key) {
        hash += ch;
    }
    return hash % shards.size();
}

void cmd_cap(std::istringstream& iss) {
    std::size_t capacity = 0;
    iss >> capacity;
    sharded_mode = false;
    lru.set_capacity(capacity);
}

void cmd_now(std::istringstream& iss) {
    long long now = 0;
    iss >> now;
    ttl_mode = true;
    lru.set_now(now);
}

void cmd_init(std::istringstream& iss) {
    std::size_t count = 0;
    std::size_t capacity = 0;
    iss >> count >> capacity;

    shards.clear();
    shards.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        shards.push_back(std::make_unique<Shard>(capacity));
    }
    lru = LRUCache{};
    hits = 0;
    misses = 0;
    ttl_mode = false;
    sharded_mode = true;
    std::cout << "OK\n";
}

void cmd_put(std::istringstream& iss) {
    Key key;
    Key value;
    iss >> key >> value;
    long long ttl = 0;
    bool has_ttl = false;
    long long ttl_value = 0;
    if (iss >> ttl_value) {
        ttl = ttl_value;
        has_ttl = true;
        ttl_mode = true;
    }

    if (!sharded_mode) {
        const auto ops = lru.put(key, value, ttl, has_ttl);
        if (!ttl_mode && !metrics_mode) {
            std::cout << "ops=" << ops << '\n';
        }
        return;
    }

    const auto index = shard_for(key);
    auto& shard = *shards[index];
    {
        std::lock_guard<std::mutex> lock(shard.mutex);
        shard.cache.put(key, value, ttl, has_ttl);
    }
    std::cout << "OK shard=" << index << '\n';
}

void cmd_get(std::istringstream& iss) {
    Key key;
    Key value;
    iss >> key;

    if (sharded_mode) {
        const auto index = shard_for(key);
        auto& shard = *shards[index];
        bool found;
        {
            std::lock_guard<std::mutex> lock(shard.mutex);
            shard.cache.set_now(lru.now);
            found = shard.cache.get(key, value) != 1;
        }
        if (found) {
            std::cout << value << " shard=" << index << '\n';
        } else {
            std::cout << "<nil>\n";
        }
        return;
    }

    const auto ops = lru.get(key, value);
    if (metrics_mode) {
        if (ops == 1) {
            ++misses;
        } else {
            ++hits;
        }
        std::cout << (ops == 1 ? "<nil>" : value) << '\n';
    } else if (ttl_mode) {
        std::cout << (ops == 1 ? "<nil>" : value) << '\n';
    } else {
        std::cout << "value=" << (ops == 1 ? "<nil>" : value) << " ops=" << ops << '\n';
    }
}

void cmd_state(std::istringstream&) {
    lru.print_state();
}

void cmd_stats(std::istringstream&) {
    if (!sharded_mode) {
        const double total = static_cast<double>(hits + misses);
        const double hit_rate = total == 0.0 ? 0.0 : static_cast<double>(hits) / total;
        std::cout << "hits=" << hits << " misses=" << misses
                  << " hit_rate=" << std::fixed << std::setprecision(2) << hit_rate << '\n';
        return;
    }

    if (full_mode) {
        std::size_t total_hits = 0;
        std::size_t total_misses = 0;
        for (auto& shard_ptr : shards) {
            auto& shard = *shard_ptr;
            std::lock_guard<std::mutex> lock(shard.mutex);
            total_hits += shard.cache.hits;
            total_misses += shard.cache.misses;
        }
        const double total = static_cast<double>(total_hits + total_misses);
        const double hit_rate = total == 0.0 ? 0.0 : static_cast<double>(total_hits) / total;
        std::cout << "hits=" << total_hits << " misses=" << total_misses
                  << " hit_rate=" << std::fixed << std::setprecision(2) << hit_rate << '\n';
        return;
    }

    for (std::size_t i = 0; i < shards.size(); ++i) {
        auto& shard = *shards[i];
        std::lock_guard<std::mutex> lock(shard.mutex);
        if (i != 0) {
            std::cout << ' ';
        }
        std::cout << "shard" << i << '=' << shard.cache.order.size();
    }
    std::cout << '\n';
}

void cmd_evictions(std::istringstream&) {
    std::size_t total = 0;
    for (auto& shard_ptr : shards) {
        auto& shard = *shard_ptr;
        std::lock_guard<std::mutex> lock(shard.mutex);
        total += shard.cache.evictions;
    }
    std::cout << total << '\n';
}

void cmd_add_front(std::istringstream& iss) {
    Key key;
    Key value;
    iss >> key >> value;
    dll.add_front(key, value);
    std::cout << "OK\n";
}

void cmd_remove_key(std::istringstream& iss) {
    Key key;
    iss >> key;
    dll.remove_key(key);
    std::cout << "OK\n";
}

void cmd_move_front(std::istringstream& iss) {
    Key key;
    iss >> key;
    dll.move_front(key);
    std::cout << "OK\n";
}

void cmd_list(std::istringstream&) {
    dll.print();
}

std::unordered_map<std::string, FuncPtr> cmd_map = {
    {"CAP", cmd_cap},
    {"NOW", cmd_now},
    {"INIT", cmd_init},
    {"PUT", cmd_put},
    {"GET", cmd_get},
    {"STATS", cmd_stats},
    {"EVICTIONS", cmd_evictions},
    {"STATE", cmd_state},
    {"ADD-FRONT", cmd_add_front},
    {"REMOVE-KEY", cmd_remove_key},
    {"MOVE-FRONT", cmd_move_front},
    {"LIST", cmd_list}
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }
        lines.push_back(line);
    }

    bool has_init = false;
    bool has_stats = false;
    bool has_full_command = false;
    for (const auto& input : lines) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        has_init = has_init || command == "INIT";
        has_stats = has_stats || command == "STATS";
        has_full_command = has_full_command || command == "NOW" || command == "EVICTIONS";
    }
    metrics_mode = has_stats && !has_init;
    full_mode = has_full_command;

    for (const auto& input : lines) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "NOW" && sharded_mode) {
            long long now = 0;
            iss >> now;
            lru.set_now(now);
            for (auto& shard_ptr : shards) {
                std::lock_guard<std::mutex> lock(shard_ptr->mutex);
                shard_ptr->cache.set_now(now);
            }
            continue;
        }

        auto it = cmd_map.find(command);
        if (it != cmd_map.end()) {
            it->second(iss);
        }
    }

    return 0;
}

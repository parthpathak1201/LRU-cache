#include <algorithm>
#include <cstddef>
#include <iostream>
#include <list>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>

using Key = std::string;
using FuncPtr = void (*)(std::istringstream&);

struct LRUCache {
    std::size_t capacity = 0;

    struct Entry {
        Key key;
        Key value;
    };

    std::list<Entry> order;

    void set_capacity(std::size_t n) {
        capacity = n;
        while (order.size() > capacity) {
            order.pop_back();
        }
    }

    void put(const Key& key, const Key& value) {
        for (auto it = order.begin(); it != order.end(); ++it) {
            if (it->key == key) {
                it->value = value;
                order.splice(order.begin(), order, it);
                return;
            }
        }

        if (capacity == 0) {
            return;
        }
        if (order.size() >= capacity) {
            order.pop_back();
        }

        order.push_front({key, value});
    }

    bool get(const Key& key, Key& value) {
        for (auto it = order.begin(); it != order.end(); ++it) {
            if (it->key == key) {
                value = it->value;
                order.splice(order.begin(), order, it);
                return true;
            }
        }
        return false;
    }

    void print_state() const {
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

void cmd_cap(std::istringstream& iss) {
    std::size_t capacity = 0;
    iss >> capacity;
    lru.set_capacity(capacity);
    std::cout << "OK\n";
}

void cmd_put(std::istringstream& iss) {
    Key key;
    Key value;
    iss >> key >> value;
    lru.put(key, value);
    std::cout << "OK\n";
}

void cmd_get(std::istringstream& iss) {
    Key key;
    Key value;
    iss >> key;
    std::cout << (lru.get(key, value) ? value : "<nil>") << '\n';
}

void cmd_state(std::istringstream&) {
    lru.print_state();
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
    {"PUT", cmd_put},
    {"GET", cmd_get},
    {"STATE", cmd_state},
    {"ADD-FRONT", cmd_add_front},
    {"REMOVE-KEY", cmd_remove_key},
    {"MOVE-FRONT", cmd_move_front},
    {"LIST", cmd_list}
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

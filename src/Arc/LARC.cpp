#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <list>
#include <algorithm>

class LARC_Cache {
private:
    size_t capacity;
    size_t ghost_capacity;
    std::list<int> cache;
    std::list<int> ghost_cache;
    std::unordered_map<int, std::list<int>::iterator> cache_map;
    std::unordered_map<int, std::list<int>::iterator> ghost_map;
    
    // Statistics
    int total_hits = 0;
    int total_misses = 0;
    int total_read_hits = 0;
    int total_write_hits = 0;
    int total_read_misses = 0;
    int total_write_misses = 0;

    void replace(int key) {
        if (cache.size() >= capacity) {
            int evicted = cache.back();
            cache.pop_back();
            cache_map.erase(evicted);
        }
    }

public:
    LARC_Cache(size_t cap)
        : capacity(cap), ghost_capacity(cap / 2) {}

    void access(int key, const std::string& request_type) {
        bool is_hit = false;

        if (cache_map.find(key) != cache_map.end()) {
            cache.splice(cache.begin(), cache, cache_map[key]);
            is_hit = true;
        } else if (ghost_map.find(key) != ghost_map.end()) {
            ghost_cache.erase(ghost_map[key]);
            ghost_map.erase(key);
            replace(key);
            cache.push_front(key);
            cache_map[key] = cache.begin();
        } else {
            if (cache.size() < capacity) {
                cache.push_front(key);
                cache_map[key] = cache.begin();
            } else {
                replace(key);
                cache.push_front(key);
                cache_map[key] = cache.begin();
            }
            ghost_cache.push_front(key);
            ghost_map[key] = ghost_cache.begin();
            if (ghost_cache.size() > ghost_capacity) {
                int old = ghost_cache.back();
                ghost_cache.pop_back();
                ghost_map.erase(old);
            }
        }

        if (is_hit) {
            total_hits++;
            if (request_type == "Read") total_read_hits++;
            else if (request_type == "Write") total_write_hits++;
        } else {
            total_misses++;
            if (request_type == "Read") total_read_misses++;
            else if (request_type == "Write") total_write_misses++;
        }
    }

    int getTotalHits() const { return total_hits; }
    int getTotalMisses() const { return total_misses; }
    int getTotalReadHits() const { return total_read_hits; }
    int getTotalWriteHits() const { return total_write_hits; }
    int getTotalReadMisses() const { return total_read_misses; }
    int getTotalWriteMisses() const { return total_write_misses; }
};

void processTraceFile(const std::string &filename, int cache_size, long long start_time, long long end_time) {
    start_time *= 1000000000;
    end_time *= 1000000000;
    LARC_Cache cache(cache_size);
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    int total_requests = 0;
    bool first_line = true;

    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        long long timestamp = 0, offset = 0;
        std::string request_type;
        char comma;

        if (!(iss >> timestamp >> comma)) continue;
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ',');
        if (!(iss >> offset >> comma)) continue;
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ',');
        if (!(std::getline(iss, request_type, ','))) continue;
        iss.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (first_line) {
            start_time += timestamp;
            end_time += timestamp;
            first_line = false;
        }

        if (timestamp < start_time) continue;
        if (timestamp > end_time) break;

        total_requests++;
        cache.access(offset, request_type);
    }

    infile.close();
    
    std::cout << "Total Requests: " << total_requests << std::endl;
    std::cout << "Total Hits: " << cache.getTotalHits() << std::endl;
    std::cout << "Total Misses: " << cache.getTotalMisses() << std::endl;
    std::cout << "Total Read Hits: " << cache.getTotalReadHits() << std::endl;
    std::cout << "Total Write Hits: " << cache.getTotalWriteHits() << std::endl;
    std::cout << "Total Read Misses: " << cache.getTotalReadMisses() << std::endl;
    std::cout << "Total Write Misses: " << cache.getTotalWriteMisses() << std::endl;
    std::cout << "Hit Rate: " << (total_requests > 0 ? (100.0 * cache.getTotalHits() / total_requests) : 0) << "%" << std::endl;
}

int main() {
    std::string trace_file;
    int cache_size;
    long long start_time, end_time;

    std::cout << "Enter trace file path: ";
    std::cin >> trace_file;
    std::cout << "Enter cache size: ";
    std::cin >> cache_size;
    std::cout << "Enter start time: ";
    std::cin >> start_time;
    std::cout << "Enter end time: ";
    std::cin >> end_time;

    processTraceFile(trace_file, cache_size, start_time, end_time);
    return 0;
}
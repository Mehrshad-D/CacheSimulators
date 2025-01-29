#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <list>
#include <algorithm>

class ARC_Cache {
private:
    size_t capacity;
    size_t p = 0; // Adaptive parameter

    // Main cache lists
    std::list<int> T1; // Recently accessed items
    std::list<int> T2; // Frequently accessed items

    // Ghost lists
    std::list<int> B1; // Evicted from T1
    std::list<int> B2; // Evicted from T2

    // Maps to store iterators for O(1) access
    std::unordered_map<int, std::list<int>::iterator> T1_map;
    std::unordered_map<int, std::list<int>::iterator> T2_map;
    std::unordered_map<int, std::list<int>::iterator> B1_map;
    std::unordered_map<int, std::list<int>::iterator> B2_map;

   // Statistics
    int total_hits;
    int total_misses;
    int total_read_hits;
    int total_write_hits;
    int total_read_misses;
    int total_write_misses;

    // Move an item from T1 to T2
    void moveToT2(int key) {
        T1_map.erase(key);
        T1.remove(key);
        T2.push_front(key);
        T2_map[key] = T2.begin();
    }

    // Replace an item in the cache
    void replace(int key) {
        if (!T1.empty() && (T1.size() > p || (!B2.empty() && T1.size() == p))) {
            // Evict from T1
            int evicted = T1.back();
            T1.pop_back();
            T1_map.erase(evicted);
            B1.push_front(evicted);
            B1_map[evicted] = B1.begin();
        } else {
            // Evict from T2
            int evicted = T2.back();
            T2.pop_back();
            T2_map.erase(evicted);
            B2.push_front(evicted);
            B2_map[evicted] = B2.begin();
        }
    }

public:
    ARC_Cache(size_t cap) 
        : capacity(cap), p(0), total_hits(0), total_misses(0), 
          total_read_hits(0), total_write_hits(0), 
          total_read_misses(0), total_write_misses(0) {}

    // Access an item in the cache
    void access(int key, const std::string& request_type) {
        bool is_hit = false;

        // If the key is in T1, move it to T2
        if (T1_map.find(key) != T1_map.end()) {
            moveToT2(key);
            is_hit = true;
        }
        // If the key is in T2, move it to the front of T2
        else if (T2_map.find(key) != T2_map.end()) {
            T2.erase(T2_map[key]);
            T2.push_front(key);
            T2_map[key] = T2.begin();
            is_hit = true;
        }
        // If the key is in B1, increase p and replace
        else if (B1_map.find(key) != B1_map.end()) {
            p = std::min(p + std::max(size_t(1), B2.size() / B1.size()), capacity);
            replace(key);
            B1_map.erase(key);
            B1.remove(key);
            T2.push_front(key);
            T2_map[key] = T2.begin();
        }
        // If the key is in B2, decrease p and replace
        else if (B2_map.find(key) != B2_map.end()) {
            p = std::max(p - std::max(size_t(1), B1.size() / B2.size()), size_t(0));
            replace(key);
            B2_map.erase(key);
            B2.remove(key);
            T2.push_front(key);
            T2_map[key] = T2.begin();
        }
        // If the key is not in any list, add it to T1
        else {
            if (T1.size() + T2.size() == capacity) {
                replace(key);
            }
            T1.push_front(key);
            T1_map[key] = T1.begin();
        }

        // Update statistics
        if (is_hit) {
            total_hits++;
            if (request_type == "Read") {
                total_read_hits++;
            } else if (request_type == "Write") {
                total_write_hits++;
            }
        } else {
            total_misses++;
            if (request_type == "Read") {
                total_read_misses++;
            } else if (request_type == "Write") {
                total_write_misses++;
            }
        }
    }

    // Getter methods for statistics
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
    ARC_Cache cache(cache_size);
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    int total_requests = 0, skipped_lines = 0;
    bool first_line = true;

    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        long long timestamp = 0, offset = 0;
        std::string request_type;
        char comma;

        // Parse timestamp
        if (!(iss >> timestamp >> comma)) {
            skipped_lines++;
            continue;
        }

        // Skip "Response Time"
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ',');

        // Parse offset
        if (!(iss >> offset >> comma)) {
            skipped_lines++;
            continue;
        }

        // Parse request size and skip it
        iss.ignore(std::numeric_limits<std::streamsize>::max(), ',');

        // Parse request type
        if (!(std::getline(iss, request_type, ','))) {
            skipped_lines++;
            continue;
        }

        // Skip remaining parameters
        iss.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Initialize start and end times based on the first line's timestamp
        if (first_line) {
            start_time += timestamp;
            end_time += timestamp;
            first_line = false;
        }

        // Filter requests based on the time range
        if (timestamp < start_time) {
            continue;
        }
        if (timestamp > end_time) break;

        total_requests++;
        cache.access(offset, request_type);
    }

    infile.close();

    // Print results
    std::cout << "Total Requests: " << total_requests << std::endl;
    std::cout << "Skipped Lines: " << skipped_lines << std::endl;
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
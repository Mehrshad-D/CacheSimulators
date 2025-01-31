#include <iostream>
#include <fstream>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>

using namespace std;

// Struct to store cache statistics
struct CacheStatistics {
    int total_hits = 0;
    int total_misses = 0;
    int cold_misses = 0;
    int total_reads = 0;
    int total_writes = 0;
    int total_read_hits = 0;
    int total_read_misses = 0;
    int total_write_hits = 0;
    int total_write_misses = 0;
};

// Function to get the first timestamp from the CSV file
long long int get_first_timestamp(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Could not open the file." << endl;
        exit(EXIT_FAILURE);
    }
    string line;
    if (getline(file, line)) {
        stringstream ss(line);
        string time_stamp;
        if (getline(ss, time_stamp, ',')) {
            try {
                return stoll(time_stamp);
            } catch (const exception&) {
                cerr << "Error reading the first timestamp." << endl;
                exit(EXIT_FAILURE);
            }
        }
    }
    cerr << "Error: Empty file or malformed data." << endl;
    exit(EXIT_FAILURE);
}

// Function to simulate LRU cache replacement while reading the file line by line
void lru_cache_simulation(
        int cache_size,
        const string& filename,
        long long int start_time,
        long long int end_time,
        CacheStatistics& cache_stats) {
    unordered_map<long long int, list<long long int>::iterator> cache_map;
    list<long long int> cache;
    unordered_set<long long int> seen_offsets;

    ifstream file(filename);
    if (!file) {
        cerr << "Error: Could not open the file." << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string time_stamp, offset, temp, request_type;

        if (getline(ss, time_stamp, ',') &&
            getline(ss, temp, ',') &&
            getline(ss, offset, ',') &&
            getline(ss, temp, ',') &&
            getline(ss, request_type, ',')) {
            try {
                long long int timestamp_val = stoll(time_stamp);
                if (timestamp_val >= start_time && timestamp_val <= end_time) {
                    long long int offset_val = stoll(offset);
                    bool is_read = (request_type == "Read");

                    // Track total reads/writes
                    if (is_read) {
                        cache_stats.total_reads++;
                    } else {
                        cache_stats.total_writes++;
                    }

                    // Check if offset is in cache
                    if (cache_map.find(offset_val) != cache_map.end()) {
                        // Cache hit
                        cache.erase(cache_map[offset_val]);
                        cache.push_front(offset_val);
                        cache_map[offset_val] = cache.begin();
                        cache_stats.total_hits++;

                        if (is_read) {
                            cache_stats.total_read_hits++;
                        } else {
                            cache_stats.total_write_hits++;
                        }
                    } else {
                        // Cache miss
                        cache_stats.total_misses++;

                        // Cold miss check
                        if (seen_offsets.find(offset_val) == seen_offsets.end()) {
                            cache_stats.cold_misses++;
                            seen_offsets.insert(offset_val);
                        }

                        if (is_read) {
                            cache_stats.total_read_misses++;
                        } else {
                            cache_stats.total_write_misses++;
                        }

                        // Handle eviction if cache is full
                        if (cache.size() == cache_size) {
                            long long int evicted_offset = cache.back();
                            cache_map.erase(evicted_offset);
                            cache.pop_back();
                        }

                        // Insert new item
                        cache.push_front(offset_val);
                        cache_map[offset_val] = cache.begin();
                    }
                }
            } catch (const exception&) {
                cerr << "Warning: Skipping malformed row: " << line << endl;
            }
        }
    }
}

int main() {
    string filename;
    int cache_size;
    long long int start_time_sec, end_time_sec;

    // Input
    std::cout << "Enter CSV filename: ";
    std::cin >> filename;

    std::cout << "Enter cache size: ";
    std::cin >> cache_size;

    std::cout << "Enter start time (relative, in seconds): ";
    std::cin >> start_time_sec;

    std::cout << "Enter end time (relative, in seconds): ";
    std::cin >> end_time_sec;

    // Convert times to nanoseconds
    long long int start_time_ns = start_time_sec * 1000000000;
    long long int end_time_ns = end_time_sec * 1000000000;

    filename = "D:\\code\\os_project\\A669.csv";
    // Get the first timestamp and adjust time range
    long long int first_timestamp = get_first_timestamp(filename);
    start_time_ns += first_timestamp;
    end_time_ns += first_timestamp;

    // Cache statistics
    CacheStatistics cache_stats;

    // Run LRU cache simulation while reading the file line by line
    lru_cache_simulation(cache_size, filename, start_time_ns, end_time_ns, cache_stats);

    // Output
    int total_requests = cache_stats.total_hits + cache_stats.total_misses;
    cout << "Total Requests: " << total_requests << endl;
    cout << "Total Hits: " << cache_stats.total_hits << endl;
    cout << "Total Misses: " << cache_stats.total_misses << endl;
    cout << "Cold Misses: " << cache_stats.cold_misses << endl;
    cout << "Total Reads: " << cache_stats.total_reads << endl;
    cout << "Total Writes: " << cache_stats.total_writes << endl;
    cout << "Total Read Hits: " << cache_stats.total_read_hits << endl;
    cout << "Total Read Misses: " << cache_stats.total_read_misses << endl;
    cout << "Total Write Hits: " << cache_stats.total_write_hits << endl;
    cout << "Total Write Misses: " << cache_stats.total_write_misses << endl;
    cout << "Hit Rate: " << (total_requests > 0 ? (100.0 * cache_stats.total_hits / total_requests) : 0) << "%" << endl;

    return 0;
}
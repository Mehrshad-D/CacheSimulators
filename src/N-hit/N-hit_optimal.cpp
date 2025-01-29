#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

struct Request {
    long long timestamp;
    std::string logical_address;
    std::string request_type;
};

struct CacheItem {
    std::string logical_address;
    int access_count;
    long long insertion_time;

    // Comparator for std::set
    bool operator<(const CacheItem& other) const {
        if (access_count == other.access_count) {
            return insertion_time < other.insertion_time; // FIFO if access counts are equal
        }
        return access_count < other.access_count; // Evict least accessed first
    }
};

class NHitCache {
private:
    int cache_size;
    int insertion_threshold;
    long long start_time, end_time;
    std::unordered_map<std::string, CacheItem> cache;
    std::unordered_map<std::string, int> access_counts;
    
    std::set<CacheItem> eviction_set;

    // Metrics
    int total_read_hit = 0, total_read_miss = 0;
    int total_write_hit = 0, total_write_miss = 0;
    int total_read_requests = 0, total_write_requests = 0;
    int total_cold_miss = 0;

public:
    NHitCache(int size, int threshold, long long start, long long end)
        : cache_size(size), insertion_threshold(threshold), start_time(start), end_time(end) {}

    void process_request(const Request& request) {
        if (request.timestamp < start_time || request.timestamp > end_time) {
            return;
        }

        if (request.request_type == "Read") {
            total_read_requests++;
        } else if (request.request_type == "Write") {
            total_write_requests++;
        }

        auto it = cache.find(request.logical_address);

        if (it != cache.end()) {
            // Cache hit
            access_counts[it->first]++;

            CacheItem old_item = it->second;
            eviction_set.erase(old_item); // Remove old item from the set

            it->second.access_count++; // Update access count
            CacheItem updated_item = it->second;
            eviction_set.insert(updated_item); // Insert updated item into the set


            if (request.request_type == "Read") {
                total_read_hit++;
            } else {
                total_write_hit++;
            }
        } else {
            // Cache miss
            if (request.request_type == "Read") {
                total_read_miss++;
            } else {
                total_write_miss++;
            }

            if(access_counts[request.logical_address] == 0){ // first miss
                total_cold_miss += 1;
            }
            access_counts[request.logical_address]++;

            if (access_counts[request.logical_address] >= insertion_threshold) {
                if (cache.size() >= static_cast<size_t>(cache_size)) {
                    evict();
                }
                CacheItem item = { request.logical_address, access_counts[request.logical_address], request.timestamp };
                cache[request.logical_address] = item;
                eviction_set.insert(item);
            }
            
        }
    }

    void evict() {
        // Evict the least accessed item (minimum item in the set)
        if (!eviction_set.empty()) {
            CacheItem to_evict = *eviction_set.begin(); // Get the minimum item
            eviction_set.erase(eviction_set.begin()); // Remove it from the set
            cache.erase(to_evict.logical_address);   // Remove it from the cache
        }
    }

    void print_metrics() const {
        int total_requests = total_read_requests + total_write_requests;
        int total_cache_hit = total_read_hit + total_write_hit;
        int total_cache_miss = total_read_miss + total_write_miss;

        std::cout << "Total Read Hit: " << total_read_hit << "\n";
        std::cout << "Total Read Miss: " << total_read_miss << "\n";
        std::cout << "Total Write Hit: " << total_write_hit << "\n";
        std::cout << "Total Write Miss: " << total_write_miss << "\n";
        std::cout << "Total Read Requests: " << total_read_requests << "\n";
        std::cout << "Total Write Requests: " << total_write_requests << "\n";
        std::cout << "Total Requests: " << total_requests << "\n";
        std::cout << "Total Cache Hit: " << total_cache_hit << "\n";
        std::cout << "Total Cache Miss: " << total_cache_miss << "\n";
        std::cout << "Total Cold Miss: " << total_cold_miss << "\n";

        double hit_ratio = total_requests > 0 ? static_cast<double>(total_cache_hit) / total_requests : 0.0;
        double miss_ratio = total_requests > 0 ? static_cast<double>(total_cache_miss) / total_requests : 0.0;

        std::cout << "Hit Ratio: " << hit_ratio << "\n";
        std::cout << "Miss Ratio: " << miss_ratio << "\n";
    }
};

void process_csv(const std::string& filename, NHitCache& cache) {
    std::ifstream file(filename);
    std::string line;

    long long first_request_time = -1;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;

        Request req;
        std::getline(ss, field, ',');
        req.timestamp = std::stoll(field);

        if (first_request_time == -1) {
            first_request_time = req.timestamp;
        }

        req.timestamp -= first_request_time; // Normalize timestamp relative to the first request

        std::getline(ss, field, ','); // Skip second field
        std::getline(ss, field, ',');
        req.logical_address = field;

        std::getline(ss, field, ','); // Skip fourth field
        std::getline(ss, field, ',');
        req.request_type = field;

        cache.process_request(req);
    }
}

int main() {
    std::string filename;
    int cache_size, insertion_threshold;
    long long start_time, end_time;

    std::cout << "Enter CSV filename: ";
    std::cin >> filename;

    std::cout << "Enter cache size: ";
    std::cin >> cache_size;

    std::cout << "Enter insertion threshold (N-hit): ";
    std::cin >> insertion_threshold;

    std::cout << "Enter start time (relative, in seconds): ";
    std::cin >> start_time;

    std::cout << "Enter end time (relative, in seconds): ";
    std::cin >> end_time;

    start_time *= 1000000000; // Convert to nanoseconds
    end_time *= 1000000000;

    NHitCache cache(cache_size, insertion_threshold, start_time, end_time);
    process_csv(filename, cache);

    cache.print_metrics();

    return 0;
}

#include <iostream>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <set>
#include <climits>
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

// Custom comparator to sort cache by decreasing "next use" time
struct CompareNextUse {
    bool operator()(const pair<int, long long int> &a, const pair<int, long long int> &b) const {
        return a.first > b.first;  // Sort by "next use" time in descending order
    }
};

// Function to preprocess future occurrences of pages
unordered_map<long long int, vector<int> > preprocess_future_occurrences(const vector<tuple<long long int, string> > &sequence, int piece_count, int piece_num) {
    unordered_map<long long int, vector<int> > future_occurrences;
    int size = sequence.size();
    int ceill = (size + piece_count - 1) / piece_count;
    int start = ceill * piece_num;
    int end = min(ceill * (piece_num + 1), size) - 1;

    for (int i = end; i >= start; --i) {
        long long int page = get<0>(sequence[i]);
        if (future_occurrences.find(page) == future_occurrences.end()) {
            future_occurrences[page].push_back(0);  // Dummy value for cold miss detection
        }
        future_occurrences[page].push_back(i);  // Store future occurrence index
    }
    return future_occurrences;
}

// Function to simulate the optimal cache replacement algorithm using std::set
void optimal_cache_replacement_with_set(int cache_size, const vector<tuple<long long int, string> > &sequence, int piece_count) {
    multiset<pair<int, long long int>, CompareNextUse> cache;  // Cache uses a custom comparator for decreasing order
    unordered_map<long long int, multiset<pair<int, long long int> >::iterator> cache_map;  // Map storing iterators to set elements
    int total_misses = 0, out_misses = 0, cold_misses = 0, total_write_misses = 0, total_read_misses = 0;
    int total_hits = 0, total_write_hits = 0, total_read_hits = 0;
    int total_reads = 0, total_writes = 0;
    int total_requests = sequence.size();

    for (int j = 0; j < piece_count; j++) {
        // Preprocess future occurrences of pages in the sequence
        unordered_map<long long int, vector<int> > future_occurrences = preprocess_future_occurrences(sequence, piece_count, j);

        // Simulate page requests
        for (int i = 0; i < sequence.size() / piece_count; ++i) {
            long long int page = get<0>(sequence[(sequence.size() / piece_count) * j + i]);
            string request_type = get<1>(sequence[(sequence.size() / piece_count) * j + i]);
            future_occurrences[page].pop_back();

            int next_use = future_occurrences[page].size() > 1 ? future_occurrences[page].back() : INT_MAX;

            if (cache_map.find(page) == cache_map.end()) {
                // Cache miss
                total_misses++;
                if (request_type == "Read") {
                    total_read_misses++;
                    total_reads++;
                } else {
                    total_write_misses++;
                    total_writes++;
                }

                if (cache.size() == cache_size) {
                    // Evict the page with the biggest "next use" (i.e., the one at the beginning)
                    auto it = cache.begin();
                    if (it->first > next_use) {
                        cache_map.erase(it->second);  // Remove from cache_map
                        cache.erase(it);  // Remove from cache set

                        if (future_occurrences[page][0] == 0) {
                            cold_misses++;  // Detect cold miss
                            future_occurrences[page][0] = 1;
                        }

                        auto insert_it = cache.insert({next_use, page});  // Insert new page into cache
                        cache_map[page] = insert_it;  // Store iterator in cache_map
                    } else {
                        out_misses++;  // Page can't be inserted because its next use is too far
                    }
                } else {
                    // Cache is not full, insert page directly
                    if (future_occurrences[page][0] == 0) {
                        cold_misses++;  // Detect cold miss
                        future_occurrences[page][0] = 1;
                    }
                    auto insert_it = cache.insert({next_use, page});  // Insert new page into cache
                    cache_map[page] = insert_it;  // Store iterator in cache_map
                }
            } else {
                // Page hit, update next use in the cache
                total_hits++;
                if (request_type == "Read") {
                    total_read_hits++;
                    total_reads++;
                } else {
                    total_write_hits++;
                    total_writes++;
                }
                auto old_it = cache_map[page];  // Retrieve the iterator for the current page
                cache.erase(old_it);  // Erase the old entry using the iterator
                auto insert_it = cache.insert({next_use, page});  // Re-insert with updated next_use
                // cout << page << " : " << insert_it->second << endl;
                cache_map[page] = insert_it;  // Update the iterator in cache_map
            }
        }
    }

    // Output
    cout << "Total Requests: " << total_requests << endl;
    cout << "Total Hits: " << total_hits << endl;
    cout << "Total Misses: " << total_misses << endl;
    cout << "Cold Misses: " << cold_misses << endl;
    cout << "Total Reads: " << total_reads << endl;
    cout << "Total Writes: " << total_writes << endl;
    cout << "Total Read Hits: " << total_read_hits << endl;
    cout << "Total Read Misses: " << total_read_misses << endl;
    cout << "Total Write Hits: " << total_write_hits << endl;
    cout << "Total Write Misses: " << total_write_misses << endl;
    cout << "Hit Rate: " << (total_requests > 0 ? (100.0 * total_hits / total_requests) : 0) << "%" << endl;
}

// Function to read the offsets & request types from the CSV file
vector<tuple<long long int, string> > read_sequence(const string& filename, long long int start, long long int end) {
    long long int first_time;
    vector<tuple<long long int, string> > data;
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Could not open the file." << endl;
        return data;
    }

    string line;
    int first_line = 1;
    while (getline(file, line)) {
    
        stringstream ss(line);
        string time_stamp, offset, temp, request_type;

        if (getline(ss, time_stamp, ',') &&
            getline(ss, temp, ',') &&
            getline(ss, offset, ',') &&
            getline(ss, temp, ',') &&
            getline(ss, request_type, ',')) {
            try {
                long long int time = stoll(time_stamp);
                if (first_line == 1)
                {
                    first_line = 0;
                    first_time = time;
                }
                
                if (time >= first_time + start && time <= first_time + end)
                    data.emplace_back(stoll(offset), request_type);
            } catch (const invalid_argument&) {
                cerr << "Warning: Non-integer value encountered and skipped: " << line << endl;
            } catch (const out_of_range&) {
                cerr << "Warning: Value out of range encountered and skipped: " << line << endl;
            }
        }
    }
    file.close();
    return data;
}

int main() {

    string filename;
    int piece_count;
    int cache_size;
    long long start_time, end_time;

    std::cout << "Enter CSV filename: ";
    std::cin >> filename;

    std::cout << "Enter cache size: ";
    std::cin >> cache_size;

    std::cout << "Enter piece number (Blady): ";
    std::cin >> piece_count;

    std::cout << "Enter start time (relative, in seconds): ";
    std::cin >> start_time;

    std::cout << "Enter end time (relative, in seconds): ";
    std::cin >> end_time;

    start_time *= 1000000000;
    end_time *= 1000000000;

    vector<tuple<long long int, string> > sequence = read_sequence(filename, start_time, end_time);

    if (sequence.empty()) {
        cerr << "No valid data found in the first column." << endl;
        return 1;
    }



    optimal_cache_replacement_with_set(cache_size, sequence, piece_count);

    return 0;
}
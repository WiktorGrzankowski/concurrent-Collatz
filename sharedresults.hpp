#ifndef SHAREDRESULTS_HPP
#define SHAREDRESULTS_HPP

#include <mutex>
#include <shared_mutex>
#include <map>
class SharedResults {
private:
    std::map<InfInt, uint64_t> collatz_map;
    std::shared_mutex s_mutex;
public:
    inline uint64_t sharedCalcCollatz(InfInt n) {
        if (collatz_map.find(n) != collatz_map.end()) {
            return collatz_map.at(n); // calculated already
        }

        uint64_t count = 0;
        assert(n > 0);
        InfInt starting_n = n;

        while (n != 1) {
            ++count;
            if (n % 2 == 1) {
                n *= 3;
                n += 1;
            } else {
                n /= 2;
            }
        }

        std::unique_lock<std::shared_mutex> u_lock(s_mutex);
        collatz_map.insert(std::pair<InfInt, uint64_t>(starting_n, count));
        return count;
    }
};

#endif

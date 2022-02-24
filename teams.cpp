#include <utility>
#include <deque>
#include <future>

#include "teams.hpp"
#include "contest.hpp"
#include "collatz.hpp"

ContestResult TeamNewThreads::runContestImpl(ContestInput const &contestInput) {
    ContestResult result;

    uint64_t MAX_THREAD_COUNT = this->getSize();
    uint64_t CONTEST_SIZE = contestInput.size();

    std::deque<std::thread> threads;
    std::promise<uint64_t> promises[CONTEST_SIZE];
    std::future<uint64_t> futures[CONTEST_SIZE];

    for (uint64_t i = 0; i < CONTEST_SIZE; ++i) {
        std::promise<uint64_t> prom;
        promises[i] = std::move(prom);
        futures[i] = promises[i].get_future();
    }

    uint64_t oldest_id = 0, current_id = 0;
    while (current_id < CONTEST_SIZE) {
        auto contest = contestInput[current_id];
        if (threads.size() == MAX_THREAD_COUNT) { // no free thread spots
            result.push_back(futures[oldest_id].get());
            // make sure the oldest thread finishes its job
            threads.front().join();
            threads.pop_front();
            ++oldest_id;
        }
        std::thread t = createThread(
                [&, t_contest = contest,
                        &t_promise = promises[current_id]] {
                    if (this->getSharedResults())
                        t_promise.set_value(this->getSharedResults()
                                                    ->sharedCalcCollatz(
                                                            t_contest));
                    else
                        t_promise.set_value(calcCollatz(t_contest));
                });
        threads.push_back(std::move(t));
        ++current_id;
    }

    for (auto &thread : threads) {
        result.push_back(futures[oldest_id].get());
        thread.join();
        ++oldest_id;
    }

    return result;
}

ContestResult
TeamConstThreads::runContestImpl(ContestInput const &contestInput) {
    ContestResult result;

    uint64_t THREAD_COUNT = this->getSize();
    uint64_t CONTEST_SIZE = contestInput.size();

    std::vector<std::promise<uint64_t>> promises[THREAD_COUNT];
    std::vector<std::future<uint64_t>> futures(CONTEST_SIZE);
    std::vector<InfInt> assigned_contests[THREAD_COUNT];

    for (uint64_t i = 0; i < CONTEST_SIZE; ++i) {
        std::promise<uint64_t> prom;
        promises[i % THREAD_COUNT].push_back(std::move(prom));
        // get the latest promise
        futures[i] = promises[i % THREAD_COUNT].back().get_future();
        // (i * THREAD_COUNT)-th thread gets i-th contest assigned
        assigned_contests[i % THREAD_COUNT].push_back(contestInput[i]);
    }

    for (uint64_t i = 0; i < THREAD_COUNT; ++i) {
        std::thread t = createThread(
                [&, &contests = assigned_contests[i],
                        &proms = promises[i]] {
                    if (this->getSharedResults()) {
                        for (uint64_t i = 0; i < contests.size(); ++i)
                            proms[i].set_value(this->getSharedResults()
                                                       ->sharedCalcCollatz(
                                                               contests[i]));
                    } else {
                        for (uint64_t i = 0; i < contests.size(); ++i)
                            proms[i].set_value(calcCollatz(contests[i]));
                    }
                });
        t.detach();
    }

    for (auto &future : futures)
        result.push_back(future.get());

    return result;
}

ContestResult TeamPool::runContest(ContestInput const &contestInput) {
    ContestResult result;

    uint64_t CONTEST_SIZE = contestInput.size();
    std::vector<std::future<uint64_t>> futures;

    auto pool_collatz =
            [this](const InfInt &contest) {
                if (this->getSharedResults())
                    return this->getSharedResults()->sharedCalcCollatz(
                            contest);
                // not shared
                return calcCollatz(contest);
            };

    for (uint64_t i = 0; i < CONTEST_SIZE; ++i)
        futures.push_back(pool.push(pool_collatz, contestInput[i]));

    result = cxxpool::get(futures.begin(), futures.end());
    return result;
}

ContestResult TeamNewProcesses::runContest(ContestInput const &contestInput) {
    ContestResult result;
    std::string input[contestInput.size()];
    std::string output[contestInput.size()];
    for (int i = 0; i < contestInput.size(); ++i) {
        input[i] = "/tmp/input" + std::to_string(i);
        output[i] = "/tmp/output" + std::to_string(i);
    }
    int desc[contestInput.size()];
    int desc_output[contestInput.size()];
    int process_count = 0;
    int finished_count = 0;
    for (int i = 0; i < contestInput.size(); ++i) {
        try {
            std::filesystem::remove(input[i]);
            std::filesystem::remove(output[i]);
        } catch (const std::filesystem::filesystem_error &e) {
            exit(-1);
        }
        const char *in = input[i].c_str();
        if (mkfifo(in, 0755) == -1)
            exit(1);
        const char *out = output[i].c_str();
        if (mkfifo(out, 0755) == -1)
            exit(1);
        if (process_count == this->getSize()) {
            desc_output[finished_count] = open(
                    output[finished_count].c_str(), O_RDONLY);
            if (desc_output[finished_count] == -1) {
                std::cout << "OPEN FAILURE\n";
                exit(1);
            }
            uint64_t partial_result;
            if (read(desc_output[finished_count], &partial_result,
                     sizeof(uint64_t)) < 0) {
                std::cout << "READ FAILURE\n";
                exit(1);
            }
            result.push_back(partial_result);
            if (close(desc_output[finished_count])) {
                std::cout << "CLOSE FAILURE\n";
                exit(1);
            }
            if (wait(0) == -1)
                exit(1);


            finished_count++;
            process_count--;
        }
        process_count++;
        std::string cnt = "1";
        switch (fork()) {
            case -1:
                exit(1);

            case 0:
                execlp("./new_process", "./new_process",
                       input[i].c_str(), output[i].c_str(),
                       cnt.c_str(), NULL);
                break;

            default:
                desc[i] = open(input[i].c_str(), O_WRONLY);
                if (desc[i] == -1) {
                    std::cout << "OPEN FAILURE\n";
                    exit(1);
                }
                size_t len = contestInput[i].numberOfDigits();
                if (write(desc[i], &len,
                          sizeof(size_t)) < 0)
                    exit(1);
                if (write(desc[i], contestInput[i].toString().c_str(),
                          len) < 0)
                    exit(1);
                if (close(desc[i])) {
                    std::cout << "CLOSE FAILURE\n";
                    exit(1);
                }
                break;
        }
    }

    for (int i = finished_count; i < contestInput.size(); ++i) {
        desc_output[i] = open(output[i].c_str(), O_RDONLY);
        if (desc_output[i] == -1) {
            std::cout << "OPEN FAILURE\n";
            exit(1);
        }

        uint64_t partial_result;
        if (read(desc_output[i], &partial_result, sizeof(uint64_t)) < 0) {
            std::cout << "READ FAILURE\n";
            exit(1);
        }
        result.push_back(partial_result);
        if (close(desc_output[i])) {
            std::cout << "CLOSE FAILURE\n";
            exit(1);
        }
    }

    while (process_count--) {
        if (wait(0) == -1)
            exit(1);
    }

    for (int i = 0; i < contestInput.size(); ++i) {
        if (unlink(input[i].c_str()))
            exit(1);
        if (unlink(output[i].c_str()))
            exit(1);
    }

    return result;
}


ContestResult TeamAsync::runContest(ContestInput const &contestInput) {
    ContestResult result;

    std::vector<std::future<uint64_t>> futures;

    for (auto &contest : contestInput) {
        futures.push_back(std::async([&] {
            if (this->getSharedResults())
                return this->getSharedResults()->sharedCalcCollatz(contest);
            // not shared
            return calcCollatz(contest);
        }));
    }

    for (auto &future : futures)
        result.push_back(future.get());

    return result;
}

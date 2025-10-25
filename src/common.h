#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

static inline uint64_t now_ns() {
    /**
     * Get current time in nanoseconds
     */
    return std::chrono::duration_caststd::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline std::vectorstd::string split(const std::string& s, char delim=',') {
    /**
     * split string via deliminator
     */
    std::vector<std::string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        out.emplace_back(s.substr(start, i - start));
        start = i + 1;
    }

    return out;

}
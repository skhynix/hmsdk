/* Copyright (c) 2025 SK hynix, Inc. */
/* SPDX-License-Identifier: BSD 2-Clause */

#include <algorithm>
#include <argp.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "pcm/src/cpucounters.h"

namespace {

// Command types
enum class CommandType {
    RECORD,
    TOP,
    DUMP,
    REPORT,
    INFO,
    HELP, // New help command
    INVALID
};

// Configuration structure
struct Config {
    CommandType command = CommandType::HELP; // Default to help mode
    int interval = 2;
    int socket = -1;
    std::string data_dir = "bwprof.data";
    std::vector<std::string> command_args;
    bool show_realtime = false; // For --top option in record mode
};

static struct argp_option options[] = {
    {"help", 'h', nullptr, 0, "Give this help list", 0},
    {"interval", 'i', "seconds", 0, "Bandwidth monitoring interval (default: 2)", 0},
    {"socket", 's', "socket number", 0, "Bandwidth monitoring for the given socket only", 0},
    {"data-dir", 'd', "directory", 0, "Data directory name (default: bwprof.data)", 0},
    {"top", 0, nullptr, 0, "Show real-time output in record mode", 0}, // Removed short option
    {nullptr, 0, nullptr, 0, nullptr, 0},
};

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    auto *config = static_cast<Config *>(state->input);

    try {
        switch (key) {
        case 'h':
            argp_state_help(state, state->out_stream, ARGP_HELP_STD_HELP);
            break;

        case 'i':
            config->interval = std::stoi(arg);
            if (config->interval <= 0) {
                argp_error(state, "Interval must be positive");
            }
            break;

        case 's':
            config->socket = std::stoi(arg);
            if (config->socket < 0) {
                argp_error(state, "Socket number must be non-negative");
            }
            break;

        case 'd':
            config->data_dir = arg;
            if (config->data_dir.empty()) {
                argp_error(state, "Data directory name cannot be empty");
            }
            break;

        case ARGP_KEY_ARG:
            // First argument is command type
            if (state->arg_num == 0) {
                std::string cmd(arg);
                if (cmd == "record") {
                    config->command = CommandType::RECORD;
                } else if (cmd == "top") {
                    config->command = CommandType::TOP;
                } else if (cmd == "dump") {
                    config->command = CommandType::DUMP;
                } else if (cmd == "report") {
                    config->command = CommandType::REPORT;
                } else if (cmd == "info") {
                    config->command = CommandType::INFO;
                } else if (cmd == "help") {
                    config->command = CommandType::HELP;
                } else {
                    argp_error(state, "Unknown command: %s", arg);
                }
            } else {
                // Collect remaining arguments as command to execute
                config->command_args.push_back(arg);
            }
            break;

        case ARGP_KEY_END:
            // Validate command-specific requirements
            if (config->command == CommandType::DUMP || config->command == CommandType::REPORT ||
                config->command == CommandType::INFO || config->command == CommandType::HELP) {
                if (!config->command_args.empty()) {
                    argp_error(state, "No command expected for dump/report/info/help mode");
                }
            }
            // For RECORD and TOP modes, command is optional
            break;

        default:
            return ARGP_ERR_UNKNOWN;
        }
    } catch (const std::exception &) {
        argp_error(state, "Invalid argument for option '%c'", key);
    }
    return 0;
}

// Data structures
struct SocketMemoryData {
    uint64_t reads = 0;
    uint64_t writes = 0;
    uint64_t cxl_reads = 0;
    uint64_t cxl_writes = 0;

    void reset() {
        reads = writes = cxl_reads = cxl_writes = 0;
    }

    void accumulate(const SocketMemoryData &other) {
        reads += other.reads;
        writes += other.writes;
        cxl_reads += other.cxl_reads;
        cxl_writes += other.cxl_writes;
    }
};

// Statistics data structure
struct BWStats {
    double dram_read_bw{}, dram_write_bw{}, dram_total_bw{};
    double cxl_read_bw{}, cxl_write_bw{}, cxl_total_bw{};
    double dram_read_sz{}, dram_write_sz{}, dram_total_sz{};
    double cxl_read_sz{}, cxl_write_sz{}, cxl_total_sz{};
    double dram_read_ratio{}, dram_write_ratio{};
    double cxl_read_ratio{}, cxl_write_ratio{};
    double dram_ratio{}, cxl_ratio{};
};

// Utility functions
namespace utils {
inline double toBW(uint64_t events, uint64_t elapsed_ms) {
    if (elapsed_ms == 0)
        return 0.0;
    // Events are counted in cache line units (64 bytes per event on x86)
    constexpr double bytes_per_event = 64.0;
    constexpr double mb_divisor = 1000000.0;
    return (static_cast<double>(events) * bytes_per_event) / mb_divisor /
           (static_cast<double>(elapsed_ms) / 1000.0);
}

inline double toSizeGB(uint64_t events) {
    // Events are counted in cache line units (64 bytes per event on x86)
    constexpr double bytes_per_event = 64.0;
    constexpr double gb_divisor = 1000000000.0;
    return (static_cast<double>(events) * bytes_per_event) / gb_divisor;
}

inline double calculateRatio(double numerator, double denominator) {
    if (denominator == 0.0)
        return 0.0;
    return (numerator / denominator) * 100.0;
}

// Get monotonic timestamp using std::chrono::steady_clock
inline double getMonotonicTimestamp() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto nanosecs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);
    return static_cast<double>(secs.count()) + static_cast<double>(nanosecs.count()) / 1000000000.0;
}

// Check if file exists using filesystem
inline bool fileExists(const std::string &filename) {
    std::error_code ec;
    bool exists = std::filesystem::exists(filename, ec);
    bool is_regular = std::filesystem::is_regular_file(filename, ec);
    return exists && is_regular && !ec;
}

// Check if directory exists
inline bool dirExists(const std::string &dirname) {
    std::error_code ec;
    bool exists = std::filesystem::exists(dirname, ec);
    bool is_directory = std::filesystem::is_directory(dirname, ec);
    return exists && is_directory && !ec;
}

// Create directory
inline bool createDir(const std::string &dirname) {
    std::error_code ec;
    bool result = std::filesystem::create_directories(dirname, ec);
    return result && !ec;
}

// Rename file/directory using filesystem
inline bool renamePath(const std::string &oldname, const std::string &newname) {
    std::error_code ec;
    std::filesystem::rename(oldname, newname, ec);
    return !ec;
}

// Remove directory recursively using filesystem
inline bool removePathRecursive(const std::string &path) {
    std::error_code ec;
    std::uintmax_t result = std::filesystem::remove_all(path, ec);
    return !ec && result != static_cast<std::uintmax_t>(-1);
}

// Split string by delimiter
inline std::vector<std::string> splitString(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

// Static function to print formatted output for a socket
inline void printFormattedOutput(size_t socket_id, const BWStats &stats) {
    printf("    Socket%zu        Throughput   AccessTotal    MemAccess  MediaAccess\n", socket_id);
    printf("                         MB/s            GB        Ratio        Ratio\n");
    printf("    DRAM   Read   %11.2f  %12.2f       %5.1f%%            -\n", stats.dram_read_bw,
           stats.dram_read_sz, stats.dram_read_ratio);
    printf("           Write  %11.2f  %12.2f       %5.1f%%            -\n", stats.dram_write_bw,
           stats.dram_write_sz, stats.dram_write_ratio);
    printf("           Total  %11.2f  %12.2f            -       %5.1f%%\n", stats.dram_total_bw,
           stats.dram_total_sz, stats.dram_ratio);

    printf("    CXL    Read   %11.2f  %12.2f       %5.1f%%            -\n", stats.cxl_read_bw,
           stats.cxl_read_sz, stats.cxl_read_ratio);
    printf("           Write  %11.2f  %12.2f       %5.1f%%            -\n", stats.cxl_write_bw,
           stats.cxl_write_sz, stats.cxl_write_ratio);
    printf("           Total  %11.2f  %12.2f            -       %5.1f%%\n\n", stats.cxl_total_bw,
           stats.cxl_total_sz, stats.cxl_ratio);
}

// Dump mode function to print formatted output (same as original report)
inline void printDump(const std::string &data_dir) {
    std::string filename = data_dir + "/bwprof.csv";
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + filename + " for reading");
    }

    std::string line;
    bool is_header = true;
    double first_timestamp = -1.0;

    while (std::getline(file, line)) {
        // Parse CSV line
        auto values = splitString(line, ',');

        if (is_header) {
            // Print header with proper formatting, replacing first column with "Time(s)"
            printf("%11s", "Time(s)");
            for (size_t i = 1; i < values.size(); ++i) {
                printf("  "); // Two spaces instead of comma
                printf("%11s", values[i].c_str());
            }
            printf("\n");
            is_header = false;
        } else {
            // Convert first column (timestamp) to elapsed time and print with %11.2f formatting
            try {
                double timestamp = std::stod(values[0]);

                // Store first timestamp as reference
                if (first_timestamp < 0) {
                    first_timestamp = timestamp;
                }

                double elapsed_time = timestamp - first_timestamp;
                printf("%11.2f", elapsed_time);
            } catch (const std::exception &) {
                // If conversion fails, print first column as 0.00
                if (first_timestamp < 0) {
                    first_timestamp = 0.0;
                }
                printf("%11.2f", 0.0);
            }

            // Print remaining data values with %11.2f formatting
            for (size_t i = 1; i < values.size(); ++i) {
                printf("  "); // Two spaces instead of comma

                try {
                    double value = std::stod(values[i]);
                    printf("%11.2f", value);
                } catch (const std::exception &) {
                    // If conversion fails, print as string
                    printf("%11s", values[i].c_str());
                }
            }
            printf("\n");
        }
    }
    file.close();
}

// Report mode function to calculate and print aggregated statistics
inline void printReport(const std::string &data_dir) {
    std::string filename = data_dir + "/bwprof.csv";
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + filename + " for reading");
    }

    std::string line;
    bool is_header = true;
    std::vector<std::vector<double>> all_values;

    // Read all data
    while (std::getline(file, line)) {
        // Skip header
        if (is_header) {
            is_header = false;
            continue;
        }

        // Parse CSV line
        auto values = splitString(line, ',');
        std::vector<double> row_values;

        for (const auto &val : values) {
            try {
                row_values.push_back(std::stod(val));
            } catch (const std::exception &) {
                row_values.push_back(0.0);
            }
        }
        all_values.push_back(row_values);
    }
    file.close();

    if (all_values.empty()) {
        std::cout << "No data found in " << filename << std::endl;
        return;
    }

    // Calculate statistics
    const size_t num_columns = all_values[0].size();
    const size_t num_rows = all_values.size();

    // Initialize accumulators
    std::vector<double> sum_values(num_columns, 0.0);
    std::vector<double> avg_values(num_columns, 0.0);

    // Calculate sums and averages for all values
    for (const auto &row : all_values) {
        for (size_t col = 0; col < std::min(num_columns, row.size()); ++col) {
            sum_values[col] += row[col];
        }
    }

    // Calculate averages for all values (column-wise averages)
    for (size_t col = 0; col < num_columns; ++col) {
        avg_values[col] = sum_values[col] / static_cast<double>(num_rows);
    }

    // Determine number of sockets from column count (6 columns per socket, plus timestamp column)
    const size_t num_sockets = (num_columns - 1) / 6;

    // Print results for each socket
    for (size_t skt = 0; skt < num_sockets; ++skt) {
        const size_t base_col = 1 + (skt * 6); // Skip timestamp column (index 0)

        // Create BWStats object for this socket
        BWStats stats{};

        // Set average throughput values for this socket (MB/s)
        stats.dram_read_bw = avg_values[base_col + 0];  // DRAM Read Throughput average
        stats.dram_write_bw = avg_values[base_col + 1]; // DRAM Write Throughput average
        stats.dram_total_bw = avg_values[base_col + 2]; // DRAM Total Throughput average
        stats.cxl_read_bw = avg_values[base_col + 3];   // CXL Read Throughput average
        stats.cxl_write_bw = avg_values[base_col + 4];  // CXL Write Throughput average
        stats.cxl_total_bw = avg_values[base_col + 5];  // CXL Total Throughput average

        // Set accumulated values for this socket and convert to GB
        // Sum values are in MB*seconds, convert to GB by dividing by 1000
        stats.dram_read_sz = sum_values[base_col + 0] / 1000.0;  // DRAM Read Total Size in GB
        stats.dram_write_sz = sum_values[base_col + 1] / 1000.0; // DRAM Write Total Size in GB
        stats.dram_total_sz = stats.dram_read_sz + stats.dram_write_sz; // DRAM Total Size in GB

        stats.cxl_read_sz = sum_values[base_col + 3] / 1000.0;       // CXL Read Total Size in GB
        stats.cxl_write_sz = sum_values[base_col + 4] / 1000.0;      // CXL Write Total Size in GB
        stats.cxl_total_sz = stats.cxl_read_sz + stats.cxl_write_sz; // CXL Total Size in GB

        // Calculate ratios based on accumulated sizes
        stats.dram_read_ratio = calculateRatio(stats.dram_read_sz, stats.dram_total_sz);
        stats.dram_write_ratio = calculateRatio(stats.dram_write_sz, stats.dram_total_sz);
        stats.cxl_read_ratio = calculateRatio(stats.cxl_read_sz, stats.cxl_total_sz);
        stats.cxl_write_ratio = calculateRatio(stats.cxl_write_sz, stats.cxl_total_sz);

        const double total_sz = stats.dram_total_sz + stats.cxl_total_sz;
        stats.dram_ratio = calculateRatio(stats.dram_total_sz, total_sz);
        stats.cxl_ratio = calculateRatio(stats.cxl_total_sz, total_sz);

        // Print formatted output using the shared function
        printFormattedOutput(skt, stats);
    }
}

// Info mode function to display system information
inline void printInfo(const std::string &data_dir) {
    std::string filename = data_dir + "/info.txt";
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + filename + " for reading");
    }

    std::map<std::string, std::string> info_map;
    std::string line;

    // Parse the info file
    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        if (line.find(':') != std::string::npos) {
            size_t colon_pos = line.find(':');
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);

            if (key == "data_version" || key == "cmdline") {
                info_map[key] = value;
            } else if (key == "recorded_time") {
                // Remove trailing newline if present
                if (!value.empty() && value.back() == '\n') {
                    value.pop_back();
                }
                info_map[key] = value;
            } else if (key == "cpuinfo") {
                if (value.find("lines=") == 0) {
                    // This is a header line indicating how many lines follow
                    try {
                        std::string lines_str = value.substr(6); // Skip "lines="
                        int lines = std::stoi(lines_str);
                        // Read the specified number of lines
                        for (int i = 0; i < lines; ++i) {
                            if (std::getline(file, line)) {
                                if (line.find(':') != std::string::npos) {
                                    size_t inner_colon_pos = line.find(':');
                                    std::string inner_key = line.substr(0, inner_colon_pos);
                                    std::string inner_value = line.substr(inner_colon_pos + 1);
                                    info_map[inner_key] = inner_value;
                                }
                            }
                        }
                    } catch (const std::exception &) {
                        // Ignore parsing errors
                    }
                } else {
                    // Direct value
                    info_map[key] = value;
                }
            } else if (key == "osinfo") {
                if (value.find("lines=") == 0) {
                    // This is a header line indicating how many lines follow
                    try {
                        std::string lines_str = value.substr(6); // Skip "lines="
                        int lines = std::stoi(lines_str);
                        // Read the specified number of lines
                        for (int i = 0; i < lines; ++i) {
                            if (std::getline(file, line)) {
                                if (line.find(':') != std::string::npos) {
                                    size_t inner_colon_pos = line.find(':');
                                    std::string inner_key = line.substr(0, inner_colon_pos);
                                    std::string inner_value = line.substr(inner_colon_pos + 1);
                                    info_map[inner_key] = inner_value;
                                }
                            }
                        }
                    } catch (const std::exception &) {
                        // Ignore parsing errors
                    }
                } else {
                    // Direct value
                    info_map[key] = value;
                }
            } else if (key == "meminfo") {
                info_map[key] = value;
            }
        }
    }
    file.close();

    // Display formatted output
    printf("# system information\n");
    printf("# ==================\n");

    // Recorded time
    if (info_map.find("recorded_time") != info_map.end()) {
        printf("# recorded on         : %s\n", info_map["recorded_time"].c_str());
    }

    // Command line
    if (info_map.find("cmdline") != info_map.end()) {
        printf("# cmdline             : %s\n", info_map["cmdline"].c_str());
    }

    // CPU info
    if (info_map.find("desc") != info_map.end()) {
        printf("# cpu info            : %s\n", info_map["desc"].c_str());
    }

    // Number of CPUs
    if (info_map.find("nr_cpus") != info_map.end()) {
        printf("# number of cpus      : %s\n", info_map["nr_cpus"].c_str());
    }

    // Memory info
    if (info_map.find("meminfo") != info_map.end()) {
        printf("# memory info         : %s\n", info_map["meminfo"].c_str());
    }

    // System load (try to get current load if available)
    double load_avg[3];
    if (getloadavg(load_avg, 3) == 3) {
        printf("# system load         : %.2f / %.2f / %.2f (1 / 5 / 15 min)\n", load_avg[0],
               load_avg[1], load_avg[2]);
    } else {
        printf("# system load         : N/A\n");
    }

    // OS info
    if (info_map.find("kernel") != info_map.end()) {
        printf("# kernel version      : %s\n", info_map["kernel"].c_str());
    }

    if (info_map.find("hostname") != info_map.end()) {
        printf("# hostname            : %s\n", info_map["hostname"].c_str());
    }

    if (info_map.find("distro") != info_map.end()) {
        printf("# distro              : %s\n", info_map["distro"].c_str());
    }
}
} // namespace utils

// Process executor for record/top modes
class ProcessExecutor {
  private:
    pid_t child_pid_;
    bool running_;

  public:
    ProcessExecutor() : child_pid_(-1), running_(false) {}

    bool isRunning() const {
        return running_;
    }

    pid_t executeCommand(const std::vector<std::string> &command) {
        if (command.empty()) {
            throw std::runtime_error("No command specified");
        }

        // Convert command to char* array for execvp
        std::vector<char *> args;
        for (const auto &arg : command) {
            args.push_back(const_cast<char *>(arg.c_str()));
        }
        args.push_back(nullptr);

        child_pid_ = fork();
        if (child_pid_ == -1) {
            throw std::runtime_error("Failed to fork: " + std::string(strerror(errno)));
        }

        if (child_pid_ == 0) {
            // Child process
            execvp(args[0], args.data());
            // If we reach here, execvp failed
            std::cerr << "Failed to execute command '" << command[0] << "': " << strerror(errno)
                      << std::endl;
            _exit(127);
        } else {
            // Parent process
            running_ = true;
            return child_pid_;
        }
    }

    int waitForChildNonBlocking() {
        if (!running_ || child_pid_ <= 0) {
            return -1;
        }

        int status;
        pid_t result = waitpid(child_pid_, &status, WNOHANG);
        if (result == -1) {
            throw std::runtime_error("waitpid failed: " + std::string(strerror(errno)));
        }

        if (result == 0) {
            // Child is still running
            return -1;
        }

        // Child has exited
        running_ = false;
        child_pid_ = -1;
        return status;
    }

    int waitForChild() {
        if (!running_ || child_pid_ <= 0) {
            return -1;
        }

        int status;
        pid_t result = waitpid(child_pid_, &status, 0);
        if (result == -1) {
            throw std::runtime_error("waitpid failed: " + std::string(strerror(errno)));
        }

        running_ = false;
        child_pid_ = -1;
        return status;
    }

    void terminateChild() {
        if (running_ && child_pid_ > 0) {
            kill(child_pid_, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            kill(child_pid_, SIGKILL);
        }
    }
};

// Statistics calculator
class StatsCalculator {
  public:
    static BWStats calculate(const SocketMemoryData &data, const SocketMemoryData &acc_data,
                             uint64_t elapsed_time) {
        BWStats stats{};

        stats.dram_read_bw = utils::toBW(data.reads, elapsed_time);
        stats.dram_write_bw = utils::toBW(data.writes, elapsed_time);
        stats.dram_total_bw = stats.dram_read_bw + stats.dram_write_bw;

        stats.cxl_read_bw = utils::toBW(data.cxl_reads, elapsed_time);
        stats.cxl_write_bw = utils::toBW(data.cxl_writes, elapsed_time);
        stats.cxl_total_bw = stats.cxl_read_bw + stats.cxl_write_bw;

        stats.dram_read_sz = utils::toSizeGB(acc_data.reads);
        stats.dram_write_sz = utils::toSizeGB(acc_data.writes);
        stats.dram_total_sz = stats.dram_read_sz + stats.dram_write_sz;

        stats.cxl_read_sz = utils::toSizeGB(acc_data.cxl_reads);
        stats.cxl_write_sz = utils::toSizeGB(acc_data.cxl_writes);
        stats.cxl_total_sz = stats.cxl_read_sz + stats.cxl_write_sz;

        // Calculate ratios
        stats.dram_read_ratio = utils::calculateRatio(stats.dram_read_sz, stats.dram_total_sz);
        stats.dram_write_ratio = utils::calculateRatio(stats.dram_write_sz, stats.dram_total_sz);
        stats.cxl_read_ratio = utils::calculateRatio(stats.cxl_read_sz, stats.cxl_total_sz);
        stats.cxl_write_ratio = utils::calculateRatio(stats.cxl_write_sz, stats.cxl_total_sz);

        const double total_sz = stats.dram_total_sz + stats.cxl_total_sz;
        stats.dram_ratio = utils::calculateRatio(stats.dram_total_sz, total_sz);
        stats.cxl_ratio = utils::calculateRatio(stats.cxl_total_sz, total_sz);

        return stats;
    }
};

// PCM Manager - handles all PCM related operations
class PCMManager {
  private:
    pcm::PCM *pcm_;

  public:
    PCMManager() : pcm_(nullptr) {}

    void initialize() {
        pcm_ = pcm::PCM::getInstance();
        if (pcm_->program() != pcm::PCM::Success) {
            throw std::runtime_error("Failed to initialize PCM");
        }
    }

    ~PCMManager() {
        if (pcm_) {
            pcm_->cleanup();
        }
    }

    int64_t getCPUFamilyModel() const {
        return pcm_->getCPUFamilyModel();
    }
    uint64_t getTickCount() const {
        return pcm_->getTickCount();
    }
    size_t getNumSockets() const {
        return pcm_->getNumSockets();
    }
    size_t getNumCXLPorts(size_t socket) const {
        return pcm_->getNumCXLPorts(socket);
    }

    void readStates(std::vector<pcm::ServerUncoreCounterState> &states) {
        const size_t num_sockets = pcm_->getNumSockets();
        states.resize(num_sockets);
        for (size_t i = 0; i < num_sockets; ++i) {
            states[i] = pcm_->getServerUncoreCounterState(i);
        }
    }

    void initializeMemoryMetrics() {
        pcm::ServerUncoreMemoryMetrics metrics = pcm::PartialWrites;
        int rankA = -1, rankB = -1;
        pcm::PCM::ErrorCode status = pcm_->programServerUncoreMemoryMetrics(metrics, rankA, rankB);
        pcm_->checkError(status);
    }
};

// Core monitoring logic
class EventProcessor {
  private:
    const size_t max_channel_;

  public:
    explicit EventProcessor(size_t max_channel) : max_channel_(max_channel) {}

    void calculateEvents(int target_socket, size_t num_sockets,
                         const std::vector<size_t> &cxl_ports_per_socket,
                         const int64_t cpu_family_model,
                         const std::vector<pcm::ServerUncoreCounterState> &prev_states,
                         const std::vector<pcm::ServerUncoreCounterState> &curr_states,
                         std::vector<SocketMemoryData> &socket_data) {

        for (size_t skt = 0; skt < num_sockets; ++skt) {
            socket_data[skt].reset();

            if (target_socket != -1 && static_cast<size_t>(target_socket) != skt)
                continue;

            processMemoryChannels(prev_states[skt], curr_states[skt], socket_data[skt],
                                  cpu_family_model);
            processCXLPorts(cxl_ports_per_socket[skt], prev_states[skt], curr_states[skt],
                            socket_data[skt]);
        }
    }

  private:
    void processMemoryChannels(const pcm::ServerUncoreCounterState &prev_state,
                               const pcm::ServerUncoreCounterState &curr_state,
                               SocketMemoryData &data, int64_t cpu_family_model) {
        for (size_t channel = 0; channel < max_channel_; ++channel) {
            data.reads += pcm::getMCCounter(channel, pcm::ServerUncorePMUs::EventPosition::READ,
                                            prev_state, curr_state);
            data.writes += pcm::getMCCounter(channel, pcm::ServerUncorePMUs::EventPosition::WRITE,
                                             prev_state, curr_state);

            switch (cpu_family_model) {
            case pcm::PCM::GNR:
            case pcm::PCM::GNR_D:
            case pcm::PCM::GRR:
            case pcm::PCM::SRF:
                data.reads += pcm::getMCCounter(
                    channel, pcm::ServerUncorePMUs::EventPosition::READ2, prev_state, curr_state);
                data.writes += pcm::getMCCounter(
                    channel, pcm::ServerUncorePMUs::EventPosition::WRITE2, prev_state, curr_state);
                break;
            }
        }
    }

    void processCXLPorts(size_t num_ports, const pcm::ServerUncoreCounterState &prev_state,
                         const pcm::ServerUncoreCounterState &curr_state, SocketMemoryData &data) {
        for (size_t p = 4; p < num_ports; ++p) {
            data.cxl_reads += pcm::getCXLCMCounter(p, pcm::PCM::EventPosition::CXL_RxC_MEM,
                                                   prev_state, curr_state);
            data.cxl_writes += pcm::getCXLDPCounter(p, pcm::PCM::EventPosition::CXL_TxC_MEM,
                                                    prev_state, curr_state);
        }
    }
};

// System info collector
class SystemInfoCollector {
  public:
    static void collectAndSave(const std::string &data_dir, const std::string &cmdline) {
        std::string filename = data_dir + "/info.txt";
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open " + filename + " for writing");
        }

        // Data version
        file << "data_version:1\n";

        // Command line
        file << "cmdline:" << cmdline << "\n";

        // Current time
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
        file << "recorded_time:" << std::ctime(&time_t_now);

        // CPU info
        long online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
        long conf_cpus = sysconf(_SC_NPROCESSORS_CONF);
        file << "cpuinfo:lines=2\n";
        file << "nr_cpus:" << online_cpus << " / " << conf_cpus << " (online/possible)\n";

        // Try to get CPU model name from /proc/cpuinfo
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        std::string cpu_model = "Unknown CPU";
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    cpu_model = line.substr(colon_pos + 2); // Skip ": "
                    break;
                }
            }
        }
        cpuinfo.close();
        file << "desc:" << cpu_model << "\n";

        // Memory info
        long page_size = sysconf(_SC_PAGESIZE);
        long free_pages = sysconf(_SC_AVPHYS_PAGES);
        long total_pages = sysconf(_SC_PHYS_PAGES);

        double free_tb =
            (static_cast<double>(free_pages) * page_size) / (1024.0 * 1024.0 * 1024.0 * 1024.0);
        double total_tb =
            (static_cast<double>(total_pages) * page_size) / (1024.0 * 1024.0 * 1024.0 * 1024.0);

        file << "meminfo:" << std::fixed << std::setprecision(1) << free_tb << " / " << total_tb
             << " TB (free / total)\n";

        // OS info
        file << "osinfo:lines=3\n";

        // Kernel info
        struct utsname buf;
        if (uname(&buf) == 0) {
            file << "kernel:" << buf.sysname << " " << buf.release << "\n";
        } else {
            file << "kernel:Unknown\n";
        }

        // Hostname
        if (gethostname(buf.nodename, sizeof(buf.nodename)) == 0) {
            file << "hostname:" << buf.nodename << "\n";
        } else {
            file << "hostname:Unknown\n";
        }

        // Distro info (try to get from /etc/os-release)
        std::string distro = "Unknown distribution";
        std::ifstream os_release("/etc/os-release");
        std::string os_line;
        while (std::getline(os_release, os_line)) {
            if (os_line.find("PRETTY_NAME=") == 0) {
                distro = os_line.substr(12); // Skip "PRETTY_NAME="
                // Remove quotes if present
                if (!distro.empty() && distro.front() == '"' && distro.back() == '"') {
                    distro = distro.substr(1, distro.length() - 2);
                }
                break;
            }
        }
        os_release.close();
        file << "distro:" << distro << "\n";

        file.close();
    }
};

// Output formatter
class OutputFormatter {
  private:
    const Config &config_;
    size_t num_sockets_;
    std::ofstream csv_file_;
    bool header_written_;
    bool show_realtime_output_;

  public:
    OutputFormatter(const Config &config)
        : config_(config), num_sockets_(0), csv_file_(), header_written_(false),
          show_realtime_output_(false) {
        // Determine if we should show real-time output once at initialization
        show_realtime_output_ = (config_.command == CommandType::TOP) ||
                                (config_.command == CommandType::RECORD && config_.show_realtime);
    }

    ~OutputFormatter() {
        if (csv_file_.is_open()) {
            csv_file_.close();
        }
    }

    void setNumSockets(size_t num_sockets) {
        num_sockets_ = num_sockets;
    }

    void openCSVFile(const std::string &data_dir) {
        // Only for record mode
        if (config_.command != CommandType::RECORD) {
            return;
        }

        std::string filename = data_dir + "/bwprof.csv";
        csv_file_.open(filename);
        if (!csv_file_.is_open()) {
            throw std::runtime_error("Failed to open " + filename + " for writing");
        }
    }

    void writeCSVHeader() {
        // Only for record mode
        if (config_.command != CommandType::RECORD) {
            return;
        }

        // Check if num_sockets_ is properly set
        if (num_sockets_ == 0) {
            throw std::runtime_error("Number of sockets not set before writing CSV header");
        }

        if (header_written_ || !csv_file_.is_open()) {
            return; // Prevent duplicate header writing or writing to closed file
        }

        // Write timestamp column first
        csv_file_ << "Timestamp";

        for (size_t skt = 0; skt < num_sockets_; ++skt) {
            if (config_.socket != -1 && static_cast<size_t>(config_.socket) != skt)
                continue;

            csv_file_ << ",SKT" << skt << "-RD,SKT" << skt << "-WR,SKT" << skt << "-SUM,SKT" << skt
                      << "-CXLRD,SKT" << skt << "-CXLWR,SKT" << skt << "-CXLSUM";
        }
        csv_file_ << "\n";
        csv_file_.flush();
        header_written_ = true;
    }

    void writeCSVLine(const std::vector<BWStats> &stats_list) {
        // Only for record mode
        if (config_.command != CommandType::RECORD || !csv_file_.is_open()) {
            return;
        }

        // Get current monotonic timestamp
        double timestamp = utils::getMonotonicTimestamp();

        // Write timestamp first
        csv_file_ << std::fixed << std::setprecision(9) << timestamp;

        // Write stats for each socket
        for (const auto &stats : stats_list) {
            csv_file_ << "," << std::fixed << std::setprecision(2) << stats.dram_read_bw << ","
                      << stats.dram_write_bw << "," << stats.dram_total_bw << ","
                      << stats.cxl_read_bw << "," << stats.cxl_write_bw << ","
                      << stats.cxl_total_bw;
        }
        csv_file_ << "\n";
        csv_file_.flush();
    }

    void printResults(const std::vector<SocketMemoryData> &current_data,
                      const std::vector<SocketMemoryData> &accumulated_data, uint64_t elapsed_time,
                      int target_socket) {

        // For real-time output modes, clear screen and show formatted output
        if (show_realtime_output_) {
            pcm::clear_screen();
        }

        std::vector<BWStats> stats_list;
        for (size_t skt = 0; skt < num_sockets_; ++skt) {
            if (target_socket != -1 && static_cast<size_t>(target_socket) != skt)
                continue;

            const BWStats stats =
                StatsCalculator::calculate(current_data[skt], accumulated_data[skt], elapsed_time);
            stats_list.push_back(stats);

            // For real-time output modes, print formatted output to console
            if (show_realtime_output_) {
                utils::printFormattedOutput(skt, stats);
            }
        }

        // For record mode, write to CSV file only
        if (config_.command == CommandType::RECORD) {
            writeCSVLine(stats_list);
        }
    }
};

// Main orchestrator for record/top modes
class BandwidthProfiler {
  private:
    Config config_;
    PCMManager pcm_manager_;
    EventProcessor processor_;
    OutputFormatter formatter_;
    ProcessExecutor process_executor_;

    std::vector<SocketMemoryData> current_socket_data_;
    std::vector<SocketMemoryData> accumulated_socket_data_;
    std::vector<pcm::ServerUncoreCounterState> prev_states_;
    std::vector<pcm::ServerUncoreCounterState> curr_states_;

  public:
    explicit BandwidthProfiler(Config config)
        : config_(std::move(config)), pcm_manager_(),
          processor_(pcm::ServerUncoreCounterState::maxChannels), formatter_(config_) {}

    void prepareDataDirectory() {
        std::string data_dir = config_.data_dir;
        std::string backup_dir = data_dir + ".old";

        // Handle existing data directory
        if (utils::dirExists(data_dir)) {
            if (utils::dirExists(backup_dir)) {
                if (!utils::removePathRecursive(backup_dir)) {
                    throw std::runtime_error("Failed to remove existing backup directory: " +
                                             backup_dir);
                }
            }
            if (!utils::renamePath(data_dir, backup_dir)) {
                throw std::runtime_error("Failed to rename existing data directory to backup: " +
                                         data_dir);
            }
        }

        // Create new data directory
        if (!utils::createDir(data_dir)) {
            throw std::runtime_error("Failed to create data directory: " + data_dir);
        }
    }

    void initialize() {
        pcm_manager_.initialize();
        pcm_manager_.initializeMemoryMetrics();

        const size_t num_sockets = pcm_manager_.getNumSockets();

        // Set number of sockets in formatter
        formatter_.setNumSockets(num_sockets);

        current_socket_data_.resize(num_sockets);
        accumulated_socket_data_.resize(num_sockets);
    }

    void run() {
        switch (config_.command) {
        case CommandType::RECORD:
            runRecordMode();
            break;
        case CommandType::TOP:
            runTopMode();
            break;
        case CommandType::DUMP:
            utils::printDump(config_.data_dir);
            break;
        case CommandType::REPORT:
            utils::printReport(config_.data_dir);
            break;
        case CommandType::INFO:
            utils::printInfo(config_.data_dir);
            break;
        case CommandType::HELP:
            // Help is handled in main()
            break;
        default:
            break;
        }
    }

  private:
    void runRecordMode() {
        // Prepare data directory FIRST, before PCM initialization
        prepareDataDirectory();

        // NOW initialize PCM after directory setup is complete
        initialize();

        // Open CSV file AFTER directory creation
        formatter_.openCSVFile(config_.data_dir);
        // Write CSV header for record mode
        formatter_.writeCSVHeader();

        // Optional command execution
        pid_t child_pid = -1;
        if (!config_.command_args.empty()) {
            std::cout << "Recording bandwidth profile..." << std::endl;
            child_pid = process_executor_.executeCommand(config_.command_args);
            std::cout << "Started command with PID " << child_pid << std::endl;
        } else {
            std::cout << "Recording system-wide bandwidth (press Ctrl+C to exit)" << std::endl;
        }

        // Collect and save system information
        std::string full_cmdline = "bwprof record";
        if (!config_.command_args.empty()) {
            full_cmdline += " --";
        }
        for (const auto &arg : config_.command_args) {
            full_cmdline += " " + arg;
        }
        SystemInfoCollector::collectAndSave(config_.data_dir, full_cmdline);

        // Run monitoring loop
        runMonitoringLoop();

        // If command was executed, wait for it to complete
        if (child_pid > 0) {
            int status = process_executor_.waitForChild();

            if (WIFEXITED(status)) {
                std::cout << "Command exited with status " << WEXITSTATUS(status) << std::endl;
            } else if (WIFSIGNALED(status)) {
                std::cout << "Command terminated by signal " << WTERMSIG(status) << std::endl;
            }
        }
    }

    void runTopMode() {
        // Initialize PCM first for top mode
        initialize();

        // Optional command execution
        pid_t child_pid = -1;
        if (!config_.command_args.empty()) {
            std::cout << "Starting bandwidth monitoring in top mode..." << std::endl;
            child_pid = process_executor_.executeCommand(config_.command_args);
            std::cout << "Started command with PID " << child_pid << std::endl;
        } else {
            std::cout << "Monitoring system-wide bandwidth (press Ctrl+C to exit)" << std::endl;
        }

        // Run monitoring loop
        runMonitoringLoop();

        // If command was executed, wait for it to complete
        if (child_pid > 0) {
            int status = process_executor_.waitForChild();

            if (WIFEXITED(status)) {
                std::cout << "Command exited with status " << WEXITSTATUS(status) << std::endl;
            } else if (WIFSIGNALED(status)) {
                std::cout << "Command terminated by signal " << WTERMSIG(status) << std::endl;
            }
        }
    }

    void runMonitoringLoop() {
        // Initial setup
        uint64_t prev_time = pcm_manager_.getTickCount();
        pcm_manager_.readStates(prev_states_);

        std::this_thread::sleep_for(std::chrono::seconds(config_.interval));

        // Main monitoring loop
        while (true) {
            // For modes with command, check if child process is still running (non-blocking)
            if (!config_.command_args.empty()) {
                int status = process_executor_.waitForChildNonBlocking();
                if (status != -1) {
                    // Child has exited
                    break;
                }
            }

            const uint64_t current_time = pcm_manager_.getTickCount();
            const uint64_t elapsed_time = current_time - prev_time;

            pcm_manager_.readStates(curr_states_);

            // Pre-calculate CXL ports count for each socket
            const size_t num_sockets = pcm_manager_.getNumSockets();
            std::vector<size_t> cxl_ports_per_socket(num_sockets);
            for (size_t s = 0; s < num_sockets; ++s) {
                cxl_ports_per_socket[s] = pcm_manager_.getNumCXLPorts(s);
            }

            processor_.calculateEvents(config_.socket, num_sockets, cxl_ports_per_socket,
                                       pcm_manager_.getCPUFamilyModel(), prev_states_, curr_states_,
                                       current_socket_data_);

            // Accumulate data
            for (size_t s = 0; s < current_socket_data_.size(); ++s) {
                accumulated_socket_data_[s].accumulate(current_socket_data_[s]);
            }

            formatter_.printResults(current_socket_data_, accumulated_socket_data_, elapsed_time,
                                    config_.socket);

            // Prepare for next iteration
            prev_time = current_time;
            prev_states_.swap(curr_states_);

            std::this_thread::sleep_for(std::chrono::seconds(config_.interval));
        }
    }
};

} // anonymous namespace

int main(int argc, char *argv[]) {
    try {
        struct argp argp {};
        argp.options = options;
        argp.parser = parse_option;
        argp.args_doc = "<command> [<args>]";
        argp.doc = "bwprof -- memory bandwidth profiler\n\n"
                   "Commands:\n"
                   "  top       Monitor bandwidth in real-time (with optional command)\n"
                   "  record    Record bandwidth profile (with optional command)\n"
                   "  report    Display aggregated bandwidth statistics\n"
                   "  dump      Display raw recorded bandwidth profile\n"
                   "  info      Display system information from recording session\n"
                   "  help      Show this help message\n"
                   "\n"
                   "Examples:\n"
                   "  bwprof top -- sleep 2\n"
                   "  bwprof top\n"
                   "  bwprof record -- ls -la\n"
                   "  bwprof record --top -- ls -la\n"
                   "  bwprof record\n"
                   "  bwprof report\n"
                   "  bwprof dump\n"
                   "  bwprof info\n"
                   "  bwprof help\n";

        Config config;
        argp_parse(&argp, argc, argv, ARGP_IN_ORDER, nullptr, &config);

        switch (config.command) {
        case CommandType::RECORD:
        case CommandType::TOP: {
            BandwidthProfiler profiler(std::move(config));
            profiler.run();
            break;
        }
        case CommandType::DUMP:
            utils::printDump(config.data_dir);
            break;
        case CommandType::REPORT:
            utils::printReport(config.data_dir);
            break;
        case CommandType::INFO:
            utils::printInfo(config.data_dir);
            break;
        case CommandType::HELP: {
            argp_help(&argp, stdout, ARGP_HELP_STD_HELP, nullptr);
            break;
        }

        default:
            // Show help for invalid commands
            argp_help(&argp, stdout, ARGP_HELP_STD_HELP, nullptr);
            return 1;
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

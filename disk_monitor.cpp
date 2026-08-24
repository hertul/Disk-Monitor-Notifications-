// disk_monitor.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <getopt.h>
#include <sys/statvfs.h>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

using namespace std;
using json = nlohmann::json;

const string CONFIG_FILE = "disk_monitor_config.json";

struct Config {
    int threshold = 80;
    int interval = 60;
    string path = "/";
};

Config loadConfig() {
    Config cfg;
    ifstream f(CONFIG_FILE);
    if (!f.is_open()) return cfg;
    json j;
    f >> j;
    if (j.contains("threshold")) cfg.threshold = j["threshold"];
    if (j.contains("interval")) cfg.interval = j["interval"];
    if (j.contains("path")) cfg.path = j["path"];
    return cfg;
}

void saveConfig(const Config& cfg) {
    json j = {{"threshold", cfg.threshold}, {"interval", cfg.interval}, {"path", cfg.path}};
    ofstream f(CONFIG_FILE);
    f << setw(2) << j << endl;
}

struct DiskUsage {
    unsigned long long total, used, free;
    double percent;
};

DiskUsage getDiskUsage(const string& path) {
#ifdef _WIN32
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (!GetDiskFreeSpaceEx(path.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        return {0,0,0,0};
    }
    unsigned long long total = totalBytes.QuadPart;
    unsigned long long free = freeBytesAvailable.QuadPart;
    unsigned long long used = total - free;
    double percent = (double)used / total * 100;
    return {total, used, free, percent};
#else
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) {
        return {0,0,0,0};
    }
    unsigned long long total = stat.f_blocks * stat.f_frsize;
    unsigned long long free = stat.f_bfree * stat.f_frsize;
    unsigned long long used = total - free;
    double percent = (double)used / total * 100;
    return {total, used, free, percent};
#endif
}

string currentTime() {
    time_t t = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return string(buf);
}

void monitor(const Config& cfg, bool once) {
    cout << "💾 Disk Monitor\n";
    cout << "Monitoring: " << cfg.path << "\n";
    cout << "Threshold: " << cfg.threshold << "% | Interval: " << cfg.interval << "s\n\n";

    bool alerted = false;
    while (true) {
        DiskUsage usage = getDiskUsage(cfg.path);
        if (usage.total == 0) {
            cerr << "Error getting disk usage.\n";
            break;
        }
        string now = currentTime();
        double totalGB = usage.total / (1024.0 * 1024 * 1024);
        double usedGB = usage.used / (1024.0 * 1024 * 1024);
        string status = usage.percent > cfg.threshold ? "🔔 ALERT" : "✅ OK";
        string color = usage.percent > cfg.threshold ? "\033[91m" : "\033[92m";
        cout << "[" << now << "] " << color << status << "\033[0m – "
             << fixed << setprecision(1) << usage.percent << "% used ("
             << usedGB << " GB / " << totalGB << " GB)\n";

        if (usage.percent > cfg.threshold && !alerted) {
            alerted = true;
        } else if (usage.percent <= cfg.threshold) {
            alerted = false;
        }

        if (once) break;
        this_thread::sleep_for(chrono::seconds(cfg.interval));
    }
}

int main(int argc, char* argv[]) {
    Config cfg = loadConfig();
    int opt;
    bool once = false, save = false;
    static struct option long_options[] = {
        {"threshold", required_argument, 0, 't'},
        {"interval", required_argument, 0, 'i'},
        {"path", required_argument, 0, 'p'},
        {"once", no_argument, 0, 'o'},
        {"save", no_argument, 0, 's'},
        {0,0,0,0}
    };
    while ((opt = getopt_long(argc, argv, "t:i:p:os", long_options, nullptr)) != -1) {
        switch (opt) {
            case 't': cfg.threshold = stoi(optarg); break;
            case 'i': cfg.interval = stoi(optarg); break;
            case 'p': cfg.path = optarg; break;
            case 'o': once = true; break;
            case 's': save = true; break;
            default:
                cerr << "Usage: disk_monitor [--threshold N] [--interval N] [--path PATH] [--once] [--save]\n";
                return 1;
        }
    }
    if (save) {
        saveConfig(cfg);
        cout << "✅ Configuration saved.\n";
        return 0;
    }
    monitor(cfg, once);
    return 0;
}

#pragma once

#include <string>
#include <vector>

#define CONSOLE 0
#define SYSTEM  1

extern std::vector<std::string> logs[2];
extern int logs_index[2];
extern int logs_focus[2];
extern const char* eascii[0x80];

template<int INDEX>
static int LoggerV(const char* format, va_list va)
{
    int index = INDEX;
    int length = vsnprintf(nullptr, 0, format, va) + 1;
//  if (strncmp(format, "[CALL]", 6) == 0)
//      return length;
    if (index == SYSTEM) {
        logs[index].push_back(std::string());
    }
    else if (logs[index].empty()) {
        logs[index].push_back(std::string());
    }
    std::string& log = logs[index].back();
    size_t offset = log.size();
    log.resize(offset + length);
    vsnprintf(log.data() + offset, length, format, va);
    log.pop_back();

    // Tab
    for (size_t i = offset, j = 0; i < log.size(); ++i, ++j) {
        char c = log[i];
        if (c == '\n')
            j = size_t(0) - 1;
        if (c != '\t')
            continue;
        size_t tab = 8 - (j % 8);
        log.replace(i, 1, tab, ' ');
        i += tab - 1;
        j += tab - 1;
    }

    // Breakline
    bool first = true;
    size_t len = log.size();
    std::vector<std::string> appends;
    appends.push_back(std::string());
    for (size_t i = offset; i < log.size(); ++i) {
        char c = log[i];
        if (c == '\n') {
            if (first) {
                len = i;
            }
            first = false;
            appends.push_back(std::string());
            continue;
        }
        if (first) {
            continue;
        }
        appends.back().push_back(c);
    }
    log.resize(len);
    std::move(appends.begin() + 1, appends.end(), std::back_inserter(logs[index]));
    logs_focus[index] = (int)logs[index].size() - 1;

//  // Call
//  if (logs[index].size() >= 2) {
//      auto back1 = logs[index][logs[index].size() - 2];
//      auto back2 = logs[index][logs[index].size() - 1];
//      if (back1.size() >= back2.size() && back1.compare(0, back2.size(), back2) == 0) {
//          logs[index].pop_back();
//          logs[index].back() += '.';
//      }
//  }

    return length;
}

template<int INDEX>
static int Logger(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    int length = LoggerV<INDEX>(format, va);
    va_end(va);
    return length;
}

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <string>

#define F(x) x

class String {
public:
    String() = default;
    String(const char* s) : data_(s ? s : "") {}
    String(int v) : data_(std::to_string(v)) {}
    String(const std::string& s) : data_(s) {}

    const char* c_str() const { return data_.c_str(); }
    int length() const { return (int)data_.size(); }
    bool startsWith(const String& prefix) const { return data_.rfind(prefix.data_, 0) == 0; }
    bool endsWith(const String& suffix) const {
        if (suffix.data_.size() > data_.size()) return false;
        return data_.compare(data_.size() - suffix.data_.size(), suffix.data_.size(), suffix.data_) == 0;
    }
    String substring(int from, int to) const { return String(data_.substr(from, to - from)); }
    int toInt() const { return std::stoi(data_.empty() ? "0" : data_); }
    String operator+(const String& o) const { return String(data_ + o.data_); }
    bool operator==(const String& o) const { return data_ == o.data_; }
    bool operator!=(const String& o) const { return data_ != o.data_; }

    void toCharArray(char* out, size_t len) const {
        if (!out || len == 0) return;
        std::strncpy(out, data_.c_str(), len - 1);
        out[len - 1] = '\0';
    }

private:
    std::string data_;
};

template<typename T>
T constrain(T val, T lo, T hi) { return val < lo ? lo : (val > hi ? hi : val); }

struct SerialStub {
    template <typename... Args>
    int printf(const char* fmt, Args... args) {
        return std::printf(fmt, args...);
    }
    void println(const char* s = "") {
        std::printf("%s\n", s ? s : "");
    }
    explicit operator bool() const { return true; }
};

extern SerialStub Serial;

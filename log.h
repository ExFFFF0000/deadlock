#pragma once
#include <print>

#define LOG(msg,...) std::print("[+]: " msg "\n", ##__VA_ARGS__)
#define ERROR(msg,...) std::print("[-]: " msg "\n", ##__VA_ARGS__)
#define WARNING(msg,...) std::print("[!]: " msg "\n", ##__VA_ARGS__)

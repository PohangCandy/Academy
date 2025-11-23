#pragma once
#include <random>
#include <string>
#include <format>

#define OUT_DEFAULT		    "\033[0m"
#define OUT_RED		        "\033[0;31m"
#define OUT_GREEN		    "\033[0;32m"
#define OUT_BLUE		    "\033[0;34m"
#define OUT_CLEAR_LINE		"\033[K"
#define OUT_CURSOR_UP		"\033[1A"
#define OUT_CURSOR_DOWN		"\033[1B"

extern std::random_device rd;
extern std::mt19937 gen;

int GetRandomNumber(int minInclusive, int maxInclusive);

std::string GetRandomString(int length);

template<typename T>
void ExpectEqual(T actual, T expected, std::string name) requires std::equality_comparable<T>
{
    if (actual != expected)
    {
        throw std::runtime_error(std::format("{} is invalid - expected: {} / actual: {}", name,expected, actual));
    }
}
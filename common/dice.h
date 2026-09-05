#pragma once
#include <string>
#include <vector>

struct RollResult {
    std::vector<int> dice;
    int modifier = 0;
    int total = 0;
};

RollResult roll(const std::string& expr);
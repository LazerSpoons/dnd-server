#include "dice.h"
#include <set>

RollResult roll(const std::string& expr) {
    std::set<int> nums{1,2,3,4,5,6,7,8,9};
    std::set<char> maths{'+', '-', '*', '/'};
    int dcount = 0;
    for (size_t i = 0; i < expr.size(); ++i) {
        // Process each character in the expression
        if (expr[i] == 'd') {
            dcount++;
        }
        else if (maths.contains(expr[i])) {
            // Handle mathematical operators if needed
        }
        else if (!nums.contains(expr[i])) {
            return RollResult{{}, 0, -100};
        }
        

    }
    return RollResult{{1}, 0, 1};
}
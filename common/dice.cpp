#include "dice.h"
#include <set>
#include <cctype>
#include <random>
namespace {
    std::mt19937& rng() {
        thread_local std::mt19937 engine{std::random_device{}()};
        return engine;
    }
}
std::optional<RollResult> roll(const std::string& expr) {
   std::set<char> maths{'+', '-'};
    RollResult result;

    int sign = 1;            // sign of the term currently being read
    int number = 0;          // digits accumulated so far
    bool haveNumber = false;
    int dcount = 0;          // how many dice, captured when 'd' is seen
    bool afterD = false;
    
    auto finishTerm = [&](){
        if (afterD){
            if(!haveNumber || number < 1) return false;
            std::uniform_int_distribution<int> dist(1,number);
            for(int k = 0; k < dcount; ++k){
                int r = sign * dist(rng());
                result.dice.push_back(r);
                result.total += r;
            }
            
        } else{
            if(!haveNumber) return false;
            result.modifier += sign * number;
            result.total += sign * number;
        }
        number = 0;
        haveNumber = false;
        dcount = 0;
        afterD = false;
        return true;
    };
    for (size_t i = 0; i < expr.size(); ++i) {
       char ch = expr[i];
       if (std::isspace(static_cast<unsigned char>(ch))){
            continue;
       }
       if (std::isdigit(static_cast<unsigned char>(ch))) {
            number = number * 10 + (ch - '0');
            if (number > 1000) return std::nullopt;              // guards against overflow
            haveNumber = true;
        }
        else if (ch == 'd' || ch == 'D') {
            if (afterD) return std::nullopt;                     // "1d6d6"
            dcount = haveNumber ? number : 1;              // "d20" means "1d20"
            if (dcount < 1) return std::nullopt;                 // "0d6"
            number = 0;
            haveNumber = false;
            afterD = true;
        }
        else if (maths.contains(ch)) {
            if (!finishTerm()) return std::nullopt;
            sign = (ch == '-') ? -1 : 1;
        }
        else {
            return std::nullopt;
        }
    }
    if (!finishTerm()) return std::nullopt;
    return result;
}
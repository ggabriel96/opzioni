#ifndef OPZIONI_TYPES_H
#define OPZIONI_TYPES_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace opz {

struct ParseResult {
  std::unique_ptr<std::vector<std::string>> remaining;
  std::unordered_map<std::string, std::string> options;
  std::unordered_set<std::string> flags;
  std::vector<std::string> positional;
};

} // namespace opz

#endif // OPZIONI_TYPES_H

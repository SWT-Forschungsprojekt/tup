#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>

namespace tup {

using meta_entry_t = std::pair<std::string, std::uint64_t>;
using meta_t = std::map<std::string, std::uint64_t>;

constexpr auto const n_version = []() {
  return meta_entry_t{"nigiri_bin_ver", 8U};
};

std::string to_str(meta_t const&);

meta_t read_hashes(std::filesystem::path const& data_path,
                   std::string const& name);

void write_hashes(std::filesystem::path const& data_path,
                  std::string const& name,
                  meta_t const& h);

}  // namespace tup
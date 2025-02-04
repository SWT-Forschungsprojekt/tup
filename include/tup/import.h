#pragma once

#include <filesystem>

#include "tup/config.h"
#include "tup/data.h"

namespace tup {

data import(config const&,
            std::filesystem::path const& data_path,
            bool write = true);

}  // namespace tup

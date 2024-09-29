#pragma once
#include "includes/toml.hpp"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class KotoLibraryConfig {
    public:
        KotoLibraryConfig(const toml::value &v);
        ~KotoLibraryConfig();
        std::string getName();
        fs::path getPath();
    private:
        std::string i_name;
        fs::path i_path;
};

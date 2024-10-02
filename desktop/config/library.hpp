#pragma once
#include <filesystem>
#include <string>

#include "includes/toml.hpp"

namespace fs = std::filesystem;

class KotoLibraryConfig {
  public:
    KotoLibraryConfig(std::string name, fs::path path);
    KotoLibraryConfig(const toml::value& v);
    ~KotoLibraryConfig();
    std::string getName();
    fs::path    getPath();

  private:
    std::string i_name;
    fs::path    i_path;
};

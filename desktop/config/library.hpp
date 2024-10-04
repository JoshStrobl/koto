#pragma once
#include <filesystem>
#include <string>

#include "includes/toml.hpp"

namespace fs = std::filesystem;

enum class KotoLibraryType {
  Audiobooks,
  Music,
  Podcasts,
};

KotoLibraryType libraryTypeFromString(const std::string& type);
std::string     libraryTypeToString(KotoLibraryType type);

class KotoLibraryConfig {
  public:
    KotoLibraryConfig(std::string name, fs::path path, KotoLibraryType type);
    KotoLibraryConfig(const toml::value& v);
    ~KotoLibraryConfig();
    std::string         getName();
    fs::path            getPath();
    KotoLibraryType     getType();
    toml::ordered_value serialize();

  private:
    std::string     i_name;
    fs::path        i_path;
    KotoLibraryType i_type;
};

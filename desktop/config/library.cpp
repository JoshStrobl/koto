#include "library.hpp"

#include <QDebug>
#include <string>

KotoLibraryConfig::KotoLibraryConfig(std::string name, fs::path path, KotoLibraryType type) {
  this->i_name = name;
  this->i_path = path;
  this->i_type = type;
  qDebug() << "Library: " << this->i_name.c_str() << " at " << this->i_path.c_str();
}

KotoLibraryConfig::~KotoLibraryConfig() {}

KotoLibraryConfig::KotoLibraryConfig(const toml::value& v) {
  this->i_name = toml::find<std::string>(v, "name");
  this->i_path = toml::find<std::string>(v, "path");
  this->i_type = libraryTypeFromString(toml::find<std::string>(v, "type"));
}

std::string KotoLibraryConfig::getName() {
  return this->i_name;
}

fs::path KotoLibraryConfig::getPath() {
  return this->i_path;
}

KotoLibraryType KotoLibraryConfig::getType() {
  return this->i_type;
}

toml::ordered_value KotoLibraryConfig::serialize() {
  toml::ordered_value library_table(toml::ordered_table {});
  library_table["name"] = this->i_name;
  library_table["path"] = this->i_path.string();
  auto stringifiedType  = libraryTypeToString(this->i_type);
  library_table["type"] = stringifiedType;
  return library_table;
}

std::string libraryTypeToString(KotoLibraryType type) {
  switch (type) {
    case KotoLibraryType::Audiobooks:
      return std::string {"audiobooks"};
    case KotoLibraryType::Music:
      return std::string {"music"};
    case KotoLibraryType::Podcasts:
      return std::string {"podcasts"};
    default:
      return std::string {"unknown"};
  }
}

KotoLibraryType libraryTypeFromString(const std::string& type) {
  if (type == "audiobooks") return KotoLibraryType::Audiobooks;
  if (type == "music") return KotoLibraryType::Music;
  if (type == "podcasts") return KotoLibraryType::Podcasts;
  throw std::invalid_argument("Unknown KotoLibraryType: " + type);
}

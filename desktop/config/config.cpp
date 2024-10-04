#include "config.hpp"

#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <filesystem>

namespace fs = std::filesystem;

KotoConfig::KotoConfig() {
  // Define our application's config location
  auto configDir        = QDir(QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppConfigLocation));
  auto configDirPath    = configDir.absolutePath();
  this->i_configDirPath = configDirPath;
  this->i_libraries     = QList<KotoLibraryConfig*>();

  fs::path filePath {};
  auto     configPathStd = configDirPath.toStdString();
  filePath /= configPathStd;
  filePath /= "config.toml";
  this->i_configPath = QString {filePath.c_str()};

  if (QFileInfo::exists(i_configPath)) {
    this->parseConfigFile(filePath);
  } else {
    this->bootstrap();
  }
}

KotoConfig& KotoConfig::instance() {
  static KotoConfig _instance;
  return _instance;
}

void KotoConfig::bootstrap() {
  this->i_uiPreferences = new KotoUiPreferences();

  auto musicDir     = QDir(QStandardPaths::writableLocation(QStandardPaths::StandardLocation::MusicLocation));
  auto musicLibrary = new KotoLibraryConfig("Music", musicDir.absolutePath().toStdString(), KotoLibraryType::Music);

  this->i_libraries.append(musicLibrary);

  this->save();
}

QString KotoConfig::getConfigDirPath() {
  return QString {this->i_configDirPath};
}

KotoUiPreferences* KotoConfig::getUiPreferences() {
  return this->i_uiPreferences;
}

QList<KotoLibraryConfig*> KotoConfig::getLibraries() {
  return this->i_libraries;
}

void KotoConfig::parseConfigFile(std::string filePath) {
  auto                       data = toml::parse(filePath);
  std::optional<toml::value> ui_prefs;

  if (data.contains("preferences.ui")) {
    auto ui_prefs_at = data.at("preferences.ui");
    if (ui_prefs_at.is_table()) ui_prefs = ui_prefs_at.as_table();
  }

  auto prefs            = new KotoUiPreferences(ui_prefs);
  this->i_uiPreferences = prefs;

  for (const auto& lib_value : toml::find<std::vector<toml::value>>(data, "libraries")) {
    auto lib = new KotoLibraryConfig(lib_value);
    this->i_libraries.append(lib);
  }
}

void KotoConfig::save() {
  toml::ordered_value config_table(toml::ordered_table {});
  config_table["preferences.ui"] = this->i_uiPreferences->serialize();

  toml::ordered_value libraries_array(toml::ordered_array {});
  for (auto lib : this->i_libraries) {
    auto lib_table = lib->serialize();
    libraries_array.push_back(lib_table);
  }
  config_table["libraries"] = libraries_array;

  auto configContent = toml::format(config_table);

  auto config_dir = QDir {this->i_configDirPath};
  if (!config_dir.exists()) config_dir.mkpath(".");
  auto config_file = QFile {this->i_configPath};

  auto out = QTextStream {&config_file};
  if (config_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    out << configContent.c_str();
    config_file.close();
  }
}

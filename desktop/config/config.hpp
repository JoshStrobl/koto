#pragma once

#include <QList>
#include <QString>

#include "library.hpp"
#include "ui_prefs.hpp"

class KotoConfig {
  public:
    KotoConfig();
    static KotoConfig& instance();
    static KotoConfig* create() { return &instance(); }
    void               save();

    QString                   getConfigDirPath();
    QList<KotoLibraryConfig*> getLibraries();
    KotoUiPreferences*        getUiPreferences();

  private:
    void bootstrap();
    void parseConfigFile(std::string filePath);

    QString                   i_configDirPath;
    QString                   i_configPath;
    QList<KotoLibraryConfig*> i_libraries;
    KotoUiPreferences*        i_uiPreferences;
};

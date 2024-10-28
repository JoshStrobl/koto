#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <iostream>
#include <thread>

#include "config/config.hpp"
#include "datalake/database.hpp"
#include "datalake/indexer.hpp"

int main(int argc, char* argv[]) {
  QQuickStyle::setStyle(QStringLiteral("org.kde.breeze"));
  QGuiApplication app(argc, argv);
  app.setApplicationDisplayName("Koto");
  app.setDesktopFileName("com.github.joshstrobl.koto.desktop");

  QQmlApplicationEngine engine;

  engine.loadFromModule("com.github.joshstrobl.koto", "Main");

  if (engine.rootObjects().isEmpty()) { return -1; }

  std::thread([]() {
    KotoConfig::create();
    KotoDatabase::create();

    KotoDatabase::instance().connect();

    // If we needed to bootstrap, index all libraries, otherwise load the database
    if (KotoDatabase::instance().requiredBootstrap()) {
      indexAllLibraries();
    } else {
      KotoDatabase::instance().load();
      std::cout << "===== Summary =====" << std::endl;
      for (auto artist : Cartographer::instance().getArtists()) {
        std::cout << "Artist: " << artist->getName().toStdString() << std::endl;
        for (auto album : artist->getAlbums()) {
          std::cout << "  Album: " << album->getTitle().toStdString() << std::endl;
          for (auto track : album->getTracks()) { std::cout << "    Track: " << track->getTitle().toStdString() << std::endl; }
        }
      }
      std::cout << "===== Tracks without albums and/or artists =====" << std::endl;
      for (auto track : Cartographer::instance().getTracks()) {
        if (track->album_uuid.has_value()) continue;
        std::cout << "Track: " << track->getTitle().toStdString() << std::endl;
      }
    }
  }).detach();

  return app.exec();
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <thread>

#include "config/library.hpp"
#include "datalake/indexer.hpp"

int main(int argc, char* argv[]) {
  QQuickStyle::setStyle(QStringLiteral("org.kde.breeze"));
  QGuiApplication app(argc, argv);
  app.setApplicationDisplayName("Koto");
  app.setDesktopFileName("com.github.joshstrobl.koto.desktop");

  QQmlApplicationEngine engine;

  engine.loadFromModule("com.github.joshstrobl.koto", "Main");

  if (engine.rootObjects().isEmpty()) { return -1; }

  //  std::thread([]() {
  //    auto config = KotoLibraryConfig("Music", "/home/joshua/Music");
  //
  //    auto indexExample = FileIndexer(&config);
  //    indexExample.index();
  //  }).detach();
  Cartographer::create();
  auto config       = KotoLibraryConfig("Music", "/home/joshua/Music");
  auto indexExample = FileIndexer(&config);
  indexExample.index();
  return app.exec();
}

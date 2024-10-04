#include "database.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <iostream>

#include "cartographer.hpp"
#include "config/config.hpp"

KotoDatabase::KotoDatabase() {
  QString dbPath        = QDir(KotoConfig::instance().getConfigDirPath()).filePath("koto.db");
  this->shouldBootstrap = !QFileInfo::exists(dbPath);

  this->db = QSqlDatabase::addDatabase("QSQLITE");
  std::cout << "Database path: " << dbPath.toStdString() << std::endl;

  this->db.setDatabaseName(dbPath);
}

KotoDatabase& KotoDatabase::instance() {
  static KotoDatabase _instance;
  return _instance;
}

void KotoDatabase::connect() {
  if (!this->db.open()) {
    std::cerr << "Failed to open database" << std::endl;
    QCoreApplication::quit();
  }

  if (this->shouldBootstrap) this->bootstrap();
}

void KotoDatabase::disconnect() {
  this->db.close();
}

QSqlDatabase KotoDatabase::getDatabase() {
  return this->db;
}

bool KotoDatabase::requiredBootstrap() {
  return this->shouldBootstrap;
}

void KotoDatabase::bootstrap() {
  QSqlQuery query(this->db);

  query.exec("CREATE TABLE IF NOT EXISTS artists(id string UNIQUE PRIMARY KEY, name string, art_path string);");
  query.exec(
      "CREATE TABLE IF NOT EXISTS albums(id string UNIQUE PRIMARY KEY, artist_id string, name string, description string, narrator string, art_path string, "
      "genres strings, year int, FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE);");
  query.exec(
      "CREATE TABLE IF NOT EXISTS tracks(id string UNIQUE PRIMARY KEY, artist_id string, album_id string, name string, disc int, position int, duration int, "
      "genres string, FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE, FOREIGN KEY (album_id) REFERENCES albums(id) ON DELETE CASCADE);");
  query.exec(
      "CREATE TABLE IF NOT EXISTS libraries_albums(id string, album_id string, path string, PRIMARY KEY (id, album_id) FOREIGN KEY(album_id) REFERENCES "
      "albums(id) "
      "ON DELETE CASCADE);");
  query.exec(
      "CREATE TABLE IF NOT EXISTS libraries_artists(id string, artist_id string, path string, PRIMARY KEY(id, artist_id) FOREIGN KEY(artist_id) REFERENCES "
      "artists(id) ON DELETE CASCADE);");
  query.exec(
      "CREATE TABLE IF NOT EXISTS libraries_tracks(id string, track_id string, path string, PRIMARY KEY(id, track_id) FOREIGN KEY(track_id) REFERENCES "
      "tracks(id) "
      "ON DELETE CASCADE);");
  query.exec(
      "CREATE TABLE IF NOT EXISTS playlist_meta(id string UNIQUE PRIMARY KEY, name string, art_path string, preferred_model int, album_id string, track_id "
      "string, "
      "playback_position_of_track int);");
  query.exec(
      "CREATE TABLE IF NOT EXISTS playlist_tracks(position INTEGER PRIMARY KEY AUTOINCREMENT, playlist_id string, track_id string, FOREIGN KEY(playlist_id) "
      "REFERENCES playlist_meta(id), FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE);");
}

void KotoDatabase::load() {
  QSqlQuery query(this->db);

  query.exec("SELECT * FROM artists;");
  while (query.next()) {
    KotoArtist* artist = KotoArtist::fromDb(query, query.record());
    Cartographer::instance().addArtist(artist);
  }

  query.exec("SELECT * FROM albums;");
  while (query.next()) {
    KotoAlbum* album  = KotoAlbum::fromDb(query, query.record());
    auto       artist = Cartographer::instance().getArtist(album->artist_uuid);
    if (artist.has_value()) { artist.value()->addAlbum(album); }
    Cartographer::instance().addAlbum(album);
  }

  query.exec("SELECT * FROM tracks;");
  while (query.next()) {
    KotoTrack* track  = KotoTrack::fromDb(query, query.record());
    auto       artist = Cartographer::instance().getArtist(track->artist_uuid);
    if (artist.has_value()) { artist.value()->addTrack(track); }
    Cartographer::instance().addTrack(track);
  }
}

#include <iostream>

#include "database.hpp"
#include "structs.hpp"

KotoAlbum::KotoAlbum() {
  this->uuid   = QUuid::createUuid();
  this->tracks = QList<KotoTrack*>();
}

KotoAlbum* KotoAlbum::fromDb(const QSqlQuery& query, const QSqlRecord& record) {
  KotoAlbum* album      = new KotoAlbum();
  album->uuid           = QUuid {query.value(record.indexOf("id")).toString()};
  album->artist_uuid    = QUuid {query.value(record.indexOf("artist_id")).toString()};
  album->title          = QString {query.value(record.indexOf("name")).toString()};
  album->year           = query.value(record.indexOf("year")).toInt();
  album->description    = QString {query.value(record.indexOf("description")).toString()};
  album->narrator       = QString {query.value(record.indexOf("narrator")).toString()};
  album->album_art_path = QString {query.value(record.indexOf("art_path")).toString()};
  album->genres         = QList {query.value(record.indexOf("genres")).toString().split(", ")};
  return album;
}

KotoAlbum::~KotoAlbum() {
  for (auto track : this->tracks) { delete track; }
  this->tracks.clear();
}

void KotoAlbum::addTrack(KotoTrack* track) {
  this->tracks.append(track);
}

void KotoAlbum::commit() {
  QSqlQuery query(KotoDatabase::instance().getDatabase());

  query.prepare(
      "INSERT INTO albums(id, artist_id, name, description, narrator, art_path, genres, year) "
      "VALUES (:id, :artist_id, :name, :description, :narrator, :art_path, :genres, :year) "
      "ON CONFLICT(id) DO UPDATE SET artist_id = :artist_id, name = :name, description = :description, narrator = :narrator, art_path = "
      ":art_path, genres = :genres, year = :year");

  query.bindValue(":id", this->uuid.toString());
  query.bindValue(":artist_id", this->artist_uuid.toString());
  query.bindValue(":name", this->title);
  query.bindValue(":year", this->year.value_or(NULL));
  query.bindValue(":description", this->description);
  query.bindValue(":art_path", this->album_art_path);
  query.bindValue(":narrator", this->narrator);
  query.bindValue(":genres", this->genres.join(", "));
  query.exec();
}

QString KotoAlbum::getAlbumArtPath() {
  return QString {this->album_art_path};
}

QString KotoAlbum::getDescription() {
  return QString {this->description};
}

QList<QString> KotoAlbum::getGenres() {
  return QList {this->genres};
}

QString KotoAlbum::getPath() {
  return this->path;
}

QString KotoAlbum::getNarrator() {
  return QString {this->narrator};
}

QString KotoAlbum::getTitle() {
  return QString {this->title};
}

QList<KotoTrack*> KotoAlbum::getTracks() {
  return QList {this->tracks};
}

std::optional<int> KotoAlbum::getYear() {
  return this->year;
}

int KotoAlbum::getYearQml() {
  return this->year.value_or(0);
}

void KotoAlbum::removeTrack(KotoTrack* track) {
  this->tracks.removeOne(track);
}

void KotoAlbum::setAlbumArtPath(QString str) {
  this->album_art_path = QString {path};
}

void KotoAlbum::setDescription(QString str) {
  this->description = QString {str};
}

void KotoAlbum::setGenres(QList<QString> list) {
  this->genres = QList {list};
}

void KotoAlbum::setNarrator(QString str) {
  this->narrator = QString {str};
}

void KotoAlbum::setPath(QString str) {
  this->path = QString {str};
}

void KotoAlbum::setTitle(QString str) {
  this->title = QString {str};
}

void KotoAlbum::setYear(int num) {
  this->year = num;
}

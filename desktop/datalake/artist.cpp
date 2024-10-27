#include <QSqlQuery>

#include "database.hpp"
#include "structs.hpp"

KotoArtist::KotoArtist() {
  this->uuid = QUuid::createUuid();
}

KotoArtist* KotoArtist::fromDb(const QSqlQuery& query, const QSqlRecord& record) {
  KotoArtist* artist = new KotoArtist();
  artist->uuid       = QUuid {query.value(record.indexOf("id")).toString()};
  artist->name       = QString {query.value(record.indexOf("name")).toString()};
  artist->path       = QString {query.value(record.indexOf("art_path")).toString()};
  return artist;
}

KotoArtist::~KotoArtist() {
  for (auto album : this->albums) { delete album; }
  for (auto track : this->tracks) { delete track; }
  this->albums.clear();
  this->tracks.clear();
}

void KotoArtist::addAlbum(KotoAlbum* album) {
  this->albums.append(album);
}

void KotoArtist::addTrack(KotoTrack* track) {
  this->tracks.append(track);
  if (!track->album_uuid.has_value()) return;
  for (auto album : this->albums) {
    if (album->uuid == track->album_uuid.value()) {
      album->addTrack(track);
      return;
    }
  }
}

void KotoArtist::commit() {
  QSqlQuery query(KotoDatabase::instance().getDatabase());
  query.prepare("INSERT INTO artists(id, name, art_path) VALUES (:id, :name, :art_path) ON CONFLICT(id) DO UPDATE SET name = :name, art_path = :art_path");
  query.bindValue(":id", this->uuid.toString());
  query.bindValue(":name", this->name);
  query.bindValue(":art_path", this->path);
  query.exec();
}

QList<KotoAlbum*> KotoArtist::getAlbums() {
  return QList {this->albums};
}

std::optional<KotoAlbum*> KotoArtist::getAlbumByName(QString name) {
  for (auto album : this->albums) {
    if (album->getTitle().contains(name)) { return std::optional {album}; }
  }

  return std::nullopt;
}

QString KotoArtist::getName() {
  return QString {this->name};
}

QString KotoArtist::getPath() {
  return QString {this->path};
}

QList<KotoTrack*> KotoArtist::getTracks() {
  return QList {this->tracks};
}

QUuid KotoArtist::getUuid() {
  return this->uuid;
}

void KotoArtist::removeAlbum(KotoAlbum* album) {
  this->albums.removeOne(album);
}

void KotoArtist::removeTrack(KotoTrack* track) {
  this->tracks.removeOne(track);
}

void KotoArtist::setName(QString str) {
  this->name = QString {str};
}

void KotoArtist::setPath(QString str) {
  this->path = QString {str};
}

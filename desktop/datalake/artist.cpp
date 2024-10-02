#include "structs.hpp"

KotoArtist::KotoArtist() {
  this->uuid = QUuid::createUuid();
}

KotoArtist* KotoArtist::fromDb() {
  return new KotoArtist();
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

QList<KotoTrack*> KotoArtist::getTracks() {
  return QList {this->tracks};
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

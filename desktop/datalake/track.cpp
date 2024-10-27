#include <QFileInfo>
#include <iostream>

#include "cartographer.hpp"
#include "database.hpp"
#include "structs.hpp"

KotoTrack::KotoTrack() {
  this->uuid = QUuid::createUuid();
}

KotoTrack* KotoTrack::fromDb(const QSqlQuery& query, const QSqlRecord& record) {
  KotoTrack* track = new KotoTrack();
  track->uuid      = QUuid {query.value(record.indexOf("id")).toString()};
  auto artist_id   = query.value(record.indexOf("artist_id"));
  if (!artist_id.isNull()) { track->artist_uuid = QUuid {artist_id.toString()}; }

  auto album_id = query.value(record.indexOf("album_id"));
  if (!album_id.isNull()) { track->album_uuid = QUuid {album_id.toString()}; }

  track->title        = QString {query.value(record.indexOf("name")).toString()};
  track->disc_number  = query.value(record.indexOf("disc")).toInt();
  track->track_number = query.value(record.indexOf("position")).toInt();
  track->duration     = query.value(record.indexOf("duration")).toInt();
  track->genres       = QList {query.value(record.indexOf("genres")).toString().split(", ")};
  return track;
}

KotoTrack* KotoTrack::fromMetadata(const KFileMetaData::SimpleExtractionResult& metadata, const QFileInfo& info) {
  auto       props   = metadata.properties();
  KotoTrack* track   = new KotoTrack();
  track->disc_number = props.value(KFileMetaData::Property::DiscNumber, 0).toInt();
  track->duration    = props.value(KFileMetaData::Property::Duration, 0).toInt();

  QStringList genres;
  for (auto v : props.values(KFileMetaData::Property::Genre)) { genres.append(v.toString()); }

  track->genres = genres;

  track->lyrics       = props.value(KFileMetaData::Property::Lyrics).toString();
  track->narrator     = props.value(KFileMetaData::Property::Performer).toString();
  track->path         = info.absolutePath();
  track->track_number = props.value(KFileMetaData::Property::TrackNumber, 0).toInt();
  track->year         = props.value(KFileMetaData::Property::ReleaseYear, 0).toInt();

  auto titleResult = props.value(KFileMetaData::Property::Title);
  if (titleResult.isValid() && !titleResult.isNull()) {
    track->title = titleResult.toString();
  } else {
    // TODO: mirror the same logic we had for cleaning up file name to determine track name, position, chapter, artist, etc.
    track->title = info.fileName();
  }

  auto artistResult   = props.value(KFileMetaData::Property::Artist);
  auto artistOptional = std::optional<KotoArtist*>();
  if (artistResult.isValid()) {
    artistOptional = Cartographer::instance().getArtist(artistResult.toString());

    if (artistOptional.has_value()) {
      auto artist        = artistOptional.value();
      track->artist_uuid = QUuid(artist->uuid);
      artist->addTrack(track);
    }
  }

  auto albumResult = props.value(KFileMetaData::Property::Album);
  if (albumResult.isValid() && artistOptional.has_value()) {
    auto artist = artistOptional.value();

    auto albumMetaName = albumResult.toString();
    auto albumOptional = artist->getAlbumByName(albumMetaName);

    if (albumOptional.has_value()) {
      auto album        = albumOptional.value();
      track->album_uuid = QUuid(album->uuid);
      album->addTrack(track);
      if (album->getTitle() != albumMetaName) album->setTitle(albumMetaName);
    }
  }

  return track;
}

KotoTrack::~KotoTrack() {}

void KotoTrack::commit() {
  QSqlQuery query(KotoDatabase::instance().getDatabase());
  query.prepare(
      "INSERT INTO tracks(id, artist_id, album_id, name, disc, position, duration, genres) "
      "VALUES (:id, :artist_id, :album_id, :name, :disc, :position, :duration, :genres) "
      "ON CONFLICT(id) DO UPDATE SET artist_id = :artist_id, album_id = :album_id, name = :name, disc = :disc, position = :position, duration = :duration, "
      "genres = :genres");
  query.bindValue(":id", this->uuid.toString());
  query.bindValue(":artist_id", !this->artist_uuid.isNull() ? this->artist_uuid.toString() : NULL);
  query.bindValue(":album_id", this->album_uuid.has_value() ? this->album_uuid.value().toString() : NULL);
  query.bindValue(":name", this->title);
  query.bindValue(":disc", this->disc_number);
  query.bindValue(":position", this->track_number);
  query.bindValue(":duration", this->duration);
  query.bindValue(":genres", this->genres.join(", "));

  query.exec();
}

QString KotoTrack::getAlbumUuid() {
  if (!this->album_uuid.has_value()) return this->album_uuid.value().toString();
  return {};
}

QUuid KotoTrack::getArtistUuid() {
  return this->artist_uuid;
}

int KotoTrack::getDiscNumber() {
  return this->disc_number;
}

int KotoTrack::getDuration() {
  return this->duration;
}

QList<QString> KotoTrack::getGenres() {
  return QList {this->genres};
}

QString KotoTrack::getLyrics() {
  return QString {this->lyrics};
}

QString KotoTrack::getNarrator() {
  return QString {this->narrator};
}

QString KotoTrack::getPath() {
  return QString {this->path};
}

QString KotoTrack::getTitle() {
  return QString {this->title};
}

int KotoTrack::getTrackNumber() {
  return this->track_number;
}

QUuid KotoTrack::getUuid() {
  return this->uuid;
}

int KotoTrack::getYear() {
  return this->year;
}

void KotoTrack::setAlbum(KotoAlbum* album) {
  this->album_uuid = QUuid(album->uuid);

  if (this->artist_uuid.isNull()) QUuid(album->artist_uuid);
}

void KotoTrack::setArtist(KotoArtist* artist) {
  this->artist_uuid = QUuid(artist->uuid);
}

void KotoTrack::setDiscNumber(int num) {
  this->disc_number = num;
}

void KotoTrack::setDuration(int num) {
  this->duration = num;
}

void KotoTrack::setGenres(QList<QString> list) {
  this->genres = QList {list};
}

void KotoTrack::setLyrics(QString str) {
  this->lyrics = QString {str};
}

void KotoTrack::setNarrator(QString str) {
  this->narrator = QString {str};
}

void KotoTrack::setPath(QString str) {
  this->path = QString {str};
}

void KotoTrack::setTitle(QString str) {
  this->title = QString {str};
}

void KotoTrack::setTrackNumber(int num) {
  this->track_number = num;
}

void KotoTrack::setYear(int num) {
  this->year = num;
}

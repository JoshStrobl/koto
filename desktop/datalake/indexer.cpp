#include "indexer.hpp"

#include <KFileMetaData/ExtractorCollection>
#include <QDebug>
#include <QDirIterator>
#include <QMimeDatabase>
#include <iostream>

FileIndexer::FileIndexer(KotoLibraryConfig* config) {
  this->i_root = QString {config->getPath().c_str()};
}

FileIndexer::~FileIndexer() = default;

void FileIndexer::index() {
  QMimeDatabase                      db;
  KFileMetaData::ExtractorCollection extractors;

  QStringList root_dirs {this->i_root.split(QDir::separator())};

  QDirIterator it {this->i_root, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories};

  while (it.hasNext()) {
    QString   path = it.next();
    QFileInfo info {path};

    if (info.isDir()) {
      auto diffPath     = info.dir().relativeFilePath(this->i_root);
      auto diffDirs     = diffPath.split("..");
      auto diffDirsSize = diffDirs.size() - 1;

      // This is going to be an artist
      if (diffDirsSize == 0) {
        auto artist = new KotoArtist();
        artist->setName(info.fileName());
        artist->setPath(path);
        artist->commit();
        this->i_artists.append(artist);
        Cartographer::instance().addArtist(artist);
        continue;
      } else if (diffDirsSize == 1) {
        auto album = new KotoAlbum();
        album->setTitle(info.fileName());

        auto artistDir      = QDir(info.dir());
        auto artistName     = artistDir.dirName();
        auto artistOptional = Cartographer::instance().getArtist(artistName);

        if (artistOptional.has_value()) {
          auto artist        = artistOptional.value();
          album->artist_uuid = artist->uuid;
          artist->addAlbum(album);
        }

        album->commit();
        Cartographer::instance().addAlbum(album);
        continue;
      }
    }

    // This is a file
    QMimeType mime = db.mimeTypeForFile(info);
    if (mime.name().startsWith("audio/")) {
      auto extractorList = extractors.fetchExtractors(mime.name());
      if (extractorList.isEmpty()) { continue; }

      auto result = KFileMetaData::SimpleExtractionResult(path, mime.name(), KFileMetaData::ExtractionResult::ExtractMetaData);
      extractorList.first()->extract(&result);

      if (!result.types().contains(KFileMetaData::Type::Audio)) { continue; }

      auto track = KotoTrack::fromMetadata(result, info);

      this->i_tracks.append(track);
      track->commit();
      Cartographer::instance().addTrack(track);
    } else if (mime.name().startsWith("image/")) {
      // This is an image, TODO add cover art to album
    }
  }
}

void indexAllLibraries() {
  for (auto library : KotoConfig::instance().getLibraries()) {
    auto indexer = new FileIndexer(library);
    indexer->index();
  }
}

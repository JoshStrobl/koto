#pragma once

#include <string>

#include "cartographer.hpp"
#include "config/library.hpp"
#include "structs.hpp"

class FileIndexer {
  public:
    FileIndexer(KotoLibraryConfig* config);
    ~FileIndexer();

    QList<KotoArtist*> getArtists();
    QList<KotoTrack*>  getFiles();
    QString            getRoot();

    void index();

  protected:
    void               indexDirectory(QString path, int depth);
    QList<KotoArtist*> i_artists;
    QList<KotoTrack*>  i_tracks;
    QString            i_root;
};

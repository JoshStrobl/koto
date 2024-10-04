#pragma once

#include <string>

#include "cartographer.hpp"
#include "config/config.hpp"
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
    QList<KotoArtist*> i_artists;
    QList<KotoTrack*>  i_tracks;
    QString            i_root;
};

void indexAllLibraries();

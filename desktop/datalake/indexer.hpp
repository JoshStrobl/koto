#pragma once

#include <string>
#include "config/library.hpp"
#include "track.hpp"

class FileIndexer {
    public:
        FileIndexer(KotoLibraryConfig * config);
        ~FileIndexer();

        std::vector<KotoTrack *> getFiles();
        std::string getRoot();

        void index();
    protected:
        std::vector<KotoTrack *> i_tracks;
        std::string i_root;
};

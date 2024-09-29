#pragma once
#include <KFileMetaData/SimpleExtractionResult>
#include <string>
#include <vector>

class KotoTrack {
    public:
        KotoTrack(); // No-op constructor
        static KotoTrack * fromDb();
        static KotoTrack * fromMetadata(KFileMetaData::SimpleExtractionResult metadata);
        ~KotoTrack();

    private:
        std::string album;
        std::string album_artist;
        std::string artist;
        int disc_number;
        int duration;
        std::vector<std::string> genres;
        std::string lyrics;
        std::string narrator;
        std::string path;
        std::string title;
        int track_number;
        int year;
};

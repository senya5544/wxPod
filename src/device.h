#pragma once

#include "app.h"

struct Album {
    std::string title;
    std::string artist;
};

class Device {
public:
    std::vector<Itdb_Track*> m_library;
    std::vector<std::string> m_artists;
    std::vector<Album> m_albums;
    std::string m_mountpoint;
    Itdb_iTunesDB* m_itdb;
    std::vector<std::string> m_to_be_removed;

    Device(int product_id, std::string mountpoint);

    std::string GetModelName();
    std::string GetMountpoint();

    void ApplyChanges();
    void DiscardChanges();
    
    void listTracks();
private:
    int m_product_id;
};
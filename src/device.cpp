#include "device.h"

#include <filesystem>

std::map<int, std::string> PRODUCTS = { // https://theapplewiki.com/wiki/USB_Product_IDs
    {0x1202, "iPod (1st/2nd generation)"},
    {0x1201, "iPod (3rd generation)"},
    {0x1203, "iPod (4th generation)"},
    {0x1204, "iPod Photo"},
    {0x1209, "iPod (5th generation)"},
    {0x1261, "iPod classic (6th generation)"},
    {0x1205, "iPod mini (1st/2nd generation)"},
    {0x120A, "iPod nano"},
    {0x1260, "iPod nano (2nd generation)"},
    {0x1262, "iPod nano (3rd generation)"},
    {0x1263, "iPod nano (4th generation)"},
    {0x1265, "iPod nano (5th generation)"},
    {0x1266, "iPod nano (6th generation)"},
    {0x1267, "iPod nano (7th generation)"},
    {0x1300, "iPod shuffle"},
    {0x1301, "iPod shuffle (2nd generation)"},
};

Device::Device(int product_id, std::string mountpoint) {
    m_product_id = product_id;
    m_mountpoint = mountpoint;
}

std::string Device::GetModelName() {
    return PRODUCTS[m_product_id];
}

std::string Device::GetMountpoint() {
    return m_mountpoint;
}

void Device::ApplyChanges() {
    for(int i = 0; i < m_to_be_removed.size(); i++) {
        if(std::filesystem::exists(m_to_be_removed[i])) {
            std::filesystem::remove(m_to_be_removed[i]);
        }
    }
    itdb_write(m_itdb, nullptr);
    listTracks();
}

void Device::DiscardChanges() {
    itdb_free(m_itdb);
    listTracks();
}

void Device::listTracks() {
    m_library.clear();
    m_to_be_removed.clear();

    m_itdb = itdb_parse(m_mountpoint.c_str(), nullptr);

    g_list_foreach(m_itdb->tracks, [](gpointer data, gpointer m_library) {
        Itdb_Track* track = (Itdb_Track*)data;
        ((std::vector<Itdb_Track*>*)(m_library))->push_back(track);
    }, &m_library);
}

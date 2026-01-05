#pragma once

#include <wx/wx.h>
#include <wx/dataview.h>
#include <libudev.h>
#include <itdb.h>
#include <map>

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

enum TRACK_CONTEXT_MENU {
    RENAME,
    DELETE,
    PROPERTIES
};

struct Track {
    uint id;
    std::string title;
    std::string artist;
    std::string album;
    ulong duration;
    std::string path;
};

struct Album {
    std::string title;
    std::string artist;
};

class App : public wxApp {
public:
    udev* m_udev;
    Itdb_iTunesDB* m_itdb;
    bool OnInit() override;
    int OnExit() override;
};

class Frame : public wxFrame {
public:
    Frame(App* app);
private:
    App* m_app;
    wxDataViewListCtrl* m_library_table;
    wxDataViewColumn* m_library_table_col_title;
    wxComboBox* m_devices_cb;
    wxButton* m_db_apply;
    wxButton* m_db_discard;
    wxDataViewListCtrl* m_artists_table;
    wxDataViewListCtrl* m_albums_table;

    std::map<std::string, std::string> m_devices;
    std::vector<std::string> m_devices_mounts;
    std::vector<Itdb_Track*> m_library;
    std::vector<std::string> m_artists;
    std::vector<Album> m_albums;
    int m_device_selected = -1;
    bool m_unsaved = false;
    std::vector<std::string> m_to_be_removed;

    void listDevices();
    void listTracks();
    
    void UpdateAlbumList();
    void UpdateTrackList();
    void Refresh();
    void IndicateUnsavedChanges();

    void OnExit(wxCloseEvent& ev);

    void OnDeviceSelected(wxCommandEvent& ev);
    void OnRefresh(wxCommandEvent& ev);
    void OnApply(wxCommandEvent& ev);
    void OnDiscard(wxCommandEvent& ev);

    void OnTrackOpened(wxDataViewEvent& ev);
    void OnTrackContextMenu(wxDataViewEvent& ev);
    void OnTrackContextMenuButton(wxCommandEvent& ev);
    void OnTrackRenamed(wxDataViewEvent& ev);

    void OnArtistsSelectionChanged(wxDataViewEvent& ev);
    void OnAlbumsSelectionChanged(wxDataViewEvent& ev);
};

class FiltersDVLStore : public wxDataViewListStore { // to keep "All" at the top & make it bold
public:
    inline virtual bool GetAttr(const wxDataViewItem &item, unsigned int col, wxDataViewItemAttr &attr) const override;
    virtual int Compare(const wxDataViewItem &item1, const wxDataViewItem &item2, unsigned int column, bool ascending) const override;
};

class TrackPropertiesFrame : public wxFrame {
public:
    TrackPropertiesFrame(App* app);
private:
    App* m_app;
    wxDataViewListCtrl* m_library_table;
    wxDataViewColumn* m_library_table_col_title;
    wxComboBox* m_devices_cb;
    wxDataViewListCtrl* m_artists_table;
    wxDataViewListCtrl* m_albums_table;

    std::map<std::string, std::string> m_devices;
    std::vector<std::string> m_devices_mounts;
    std::vector<Itdb_Track*> m_library;
    std::vector<std::string> m_artists;
    std::vector<Album> m_albums;
    int m_device_selected = -1;

    void listDevices();
    void listTracks();
    
    void UpdateAlbumList();
    void UpdateTrackList();
    void Refresh();

    void OnDeviceSelected(wxCommandEvent& ev);
    void OnRefresh(wxCommandEvent& ev);

    void OnTrackOpened(wxDataViewEvent& ev);
    void OnTrackContextMenu(wxDataViewEvent& ev);
    void OnTrackContextMenuButton(wxCommandEvent& ev);
    void OnTrackRenamed(wxDataViewEvent& ev);

    void OnArtistsSelectionChanged(wxDataViewEvent& ev);
    void OnAlbumsSelectionChanged(wxDataViewEvent& ev);
};

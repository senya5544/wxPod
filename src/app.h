#pragma once

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <taglib/tag.h>
#include <libudev.h>
#include <itdb.h>
#include <map>

enum CONTEXT_MENU {
    RENAME,
    DELETE,
    PROPERTIES
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

    std::vector<Itdb_Track*> m_library;
    std::vector<std::string> m_artists;
    std::vector<Album> m_albums;

    wxDataViewListCtrl* m_library_table;
    wxDataViewListCtrl* m_artists_table;
    wxDataViewListCtrl* m_albums_table;
    wxDataViewColumn* m_library_table_col_title;
    wxDataViewColumn* m_library_table_col_artist;
    wxDataViewColumn* m_library_table_col_album;
    wxDataViewColumn* m_artists_table_col;
    wxDataViewColumn* m_albums_table_col;

    void IndicateUnsavedChanges();
private:
    App* m_app;

    wxComboBox* m_devices_cb;
    wxButton* m_db_apply;
    wxButton* m_db_discard;

    std::map<std::string, std::string> m_devices;
    std::vector<std::string> m_devices_mounts;
    int m_device_selected = -1;
    bool m_unsaved = false;
    std::vector<std::string> m_to_be_removed;

    void listDevices();
    void listTracks();
    
    void UpdateAlbumList();
    void UpdateTrackList();
    void Refresh();

    void OnExit(wxCloseEvent& ev);

    void OnDeviceSelected(wxCommandEvent& ev);
    void OnRefresh(wxCommandEvent& ev);
    void OnApply(wxCommandEvent& ev);
    void OnDiscard(wxCommandEvent& ev);

    void OnTrackOpened(wxDataViewEvent& ev);
    void OnTrackContextMenu(wxDataViewEvent& ev);
    void OnTrackContextMenuButton(wxCommandEvent& ev);
    void OnTrackRenamed(wxDataViewEvent& ev);

    void OnAlbumContextMenu(wxDataViewEvent& ev);
    void OnAlbumContextMenuButton(wxCommandEvent& ev);
    void OnAlbumEditingStarted(wxDataViewEvent& ev);
    void OnAlbumRenamed(wxDataViewEvent& ev);

    void OnArtistsSelectionChanged(wxDataViewEvent& ev);
    void OnAlbumsSelectionChanged(wxDataViewEvent& ev);
};

class FiltersDVLStore : public wxDataViewListStore { // to keep "All" at the top & make it bold
public:
    inline virtual bool GetAttr(const wxDataViewItem &item, unsigned int col, wxDataViewItemAttr &attr) const override;
    virtual int Compare(const wxDataViewItem &item1, const wxDataViewItem &item2, unsigned int column, bool ascending) const override;
};

class TrackPropertiesDialog : public wxDialog {
public:
    TrackPropertiesDialog(Frame* frame, Itdb_Track* track, wxDataViewItem item);
private:
    Frame* m_frame;
    Itdb_Track* m_track;
    wxDataViewItem m_item;

    wxTextCtrl* m_title_input;
    wxTextCtrl* m_artist_input;
    wxTextCtrl* m_album_input;
    wxTextCtrl* m_genre_input;

    //void OnPrevious(wxCommandEvent& ev);
    //void OnNext(wxCommandEvent& ev);
    void OnOk(wxCommandEvent& ev);
    void OnCancel(wxCommandEvent& ev);
    void OnExit(wxCloseEvent& ev);
};

class AlbumPropertiesDialog : public wxDialog {
public:
    AlbumPropertiesDialog(Frame* frame, Album album, wxDataViewItem item);
private:
    Frame* m_frame;
    Album m_album;
    wxDataViewItem m_item;

    wxTextCtrl* m_title_input;
    wxTextCtrl* m_artist_input;
    wxStaticBitmap* m_bitmap;

    bool m_missing_artwork = false;
    std::string m_new_artwork_path;
    TagLib::ByteVector m_new_artwork;
    bool m_remove_artwork = false;

    void OnBrowse(wxCommandEvent& ev);
    void OnFetch(wxCommandEvent& ev);
    void OnRemove(wxCommandEvent& ev);

    void OnOk(wxCommandEvent& ev);
    void OnCancel(wxCommandEvent& ev);
    void OnExit(wxCloseEvent& ev);
};

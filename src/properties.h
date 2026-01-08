#pragma once

#include "app.h"
#include "device.h"

class TrackPropertiesDialog : public wxDialog {
public:
    TrackPropertiesDialog(Frame* frame, Device* device, Itdb_Track* track, wxDataViewItem item);
private:
    Frame* m_frame;
    Device* m_device;
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
    AlbumPropertiesDialog(Frame* frame, Device* device, Album album, wxDataViewItem item);
private:
    Frame* m_frame;
    Device* m_device;
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

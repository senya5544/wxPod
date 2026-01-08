#pragma once

#include "app.h"

class TrackList : public wxDataViewListCtrl {
public:
    wxDataViewColumn* m_col_title;
    wxDataViewColumn* m_col_artist;
    wxDataViewColumn* m_col_album;

    TrackList(wxWindow* parent, Frame* frame, Device* device);
    void SetDevice(Device* device);
    void AddTrack(Itdb_Track* track, int data);
    void RefreshList();
private:
    Frame* m_frame;
    Device* m_device;

    void OnTrackOpened(wxDataViewEvent& ev);
    void OnContextMenu(wxDataViewEvent& ev);
    void OnContextMenuButton(wxCommandEvent& ev);
    void OnTrackRenamed(wxDataViewEvent& ev);
};
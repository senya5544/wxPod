#pragma once

#include "app.h"

class ArtistFilters : public wxDataViewListCtrl {
public:
    wxDataViewColumn* m_col;

    ArtistFilters(wxWindow* parent, Frame* frame, Device* device);
    void SetDevice(Device* device);
    void AddArtist(std::string artist, int data);
private:
    Frame* m_frame;
    Device* m_device;

    void OnSelectionChanged(wxDataViewEvent& ev);
    void OnContextMenu(wxDataViewEvent& ev);
    void OnContextMenuButton(wxCommandEvent& ev);
    void OnArtistRenamingStarted(wxDataViewEvent& ev);
    void OnArtistRenamed(wxDataViewEvent& ev);
};

class AlbumFilters : public wxDataViewListCtrl {
public:
    wxDataViewColumn* m_col;

    AlbumFilters(wxWindow* parent, Frame* frame, Device* device);
    void SetDevice(Device* device);
    void RefreshList();
    void AddAlbum(std::string album, int data);
private:
    Frame* m_frame;
    Device* m_device;

    void OnSelectionChanged(wxDataViewEvent& ev);
    void OnContextMenu(wxDataViewEvent& ev);
    void OnContextMenuButton(wxCommandEvent& ev);
    void OnAlbumRenamingStarted(wxDataViewEvent& ev);
    void OnAlbumRenamed(wxDataViewEvent& ev);
};

class FiltersDVLStore : public wxDataViewListStore { // to keep "All" at the top & make it bold
public:
    inline virtual bool GetAttr(const wxDataViewItem &item, unsigned int col, wxDataViewItemAttr &attr) const override;
    virtual int Compare(const wxDataViewItem &item1, const wxDataViewItem &item2, unsigned int column, bool ascending) const override;
};

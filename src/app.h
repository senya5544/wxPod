#pragma once

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <taglib/tag.h>
#include <libudev.h>
#include <itdb.h>
#include <map>

class Device;
class TrackList;
class ArtistFilters;
class AlbumFilters;
class TrackPropertiesDialog;
class AlbumPropertiesDialog;

wxVector<wxVariant> wxV(std::vector<std::string> X); // std::vector<std::string> -> wxVector<wxVariant> helper fuction
wxVector<wxVariant> wxV_wxs(std::vector<wxString> X); // std::vector<wxString> -> wxVector<wxVariant> helper function

enum CONTEXT_MENU {
    RENAME,
    DELETE,
    PROPERTIES
};

class App : public wxApp {
public:
    udev* m_udev;

    bool OnInit() override;
    int OnExit() override;
};

class Frame : public wxFrame {
public:
    TrackList* m_tracklist;
    ArtistFilters* m_artist_filters;
    AlbumFilters* m_album_filters;

    Frame(App* app);

    void IndicateUnsavedChanges();
private:
    App* m_app;
    Device* m_device;

    wxComboBox* m_devices_cb;
    wxButton* m_db_apply;
    wxButton* m_db_discard;

    std::vector<Device*> m_devices;
    int m_device_selected = -1;
    bool m_unsaved = false;

    void listDevices();
    void RefreshApp();

    void OnExit(wxCloseEvent& ev);

    void OnDeviceSelected(wxCommandEvent& ev);
    void OnRefresh(wxCommandEvent& ev);
    void OnApply(wxCommandEvent& ev);
    void OnDiscard(wxCommandEvent& ev);
};
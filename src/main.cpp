#include "app.h"

#include "device.h"
#include "tracklist.h"
#include "filters.h"
#include "properties.h"

#include <wx/splitter.h>
#include <wx/persist/toplevel.h>
#include <wx/artprov.h>
#include <mntent.h>
#include <filesystem>

wxVector<wxVariant> wxV(std::vector<std::string> X) {
    wxVector<wxVariant> _;
    for(int i = 0; i < X.size(); i++) {
        _.push_back(wxString::FromUTF8(X[i]));
    }
    return _;
}

wxVector<wxVariant> wxV_wxs(std::vector<wxString> X) {
    wxVector<wxVariant> _;
    for(int i = 0; i < X.size(); i++) {
        _.push_back(X[i]);
    }
    return _;
} 

bool App::OnInit() {
    wxInitAllImageHandlers();

    m_udev = udev_new();

    auto frame = new Frame(this);
    frame->Show();

    return true;
}

int App::OnExit() {
    udev_unref(m_udev);

    return 0;
}

Frame::Frame(App* app) : wxFrame(nullptr, wxID_ANY, "wxPod", wxDefaultPosition, wxSize(800, 450)) {
    m_app = app;
    auto sizer = new wxBoxSizer(wxVERTICAL);

    auto topBar = new wxBoxSizer(wxHORIZONTAL);
    
    m_devices_cb = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, wxDefaultSize.GetHeight()), wxArrayString(), wxCB_DROPDOWN | wxCB_READONLY);
    m_devices_cb->Bind(wxEVT_COMBOBOX, &Frame::OnDeviceSelected, this);
    topBar->Add(m_devices_cb, 0, wxALL, 5);

    auto devices_refresh = new wxButton(this, wxID_ANY, "Refresh");
    devices_refresh->SetToolTip("Refresh device list and library");
    devices_refresh->SetBitmap(wxArtProvider::GetBitmap(wxART_REFRESH));
    devices_refresh->Bind(wxEVT_BUTTON, &Frame::OnRefresh, this);
    topBar->Add(devices_refresh, 0, wxALL, 5);

    m_db_apply = new wxButton(this, wxID_ANY, "Apply");
    m_db_apply->SetToolTip("Apply changes to the library");
    m_db_apply->SetBitmap(wxArtProvider::GetBitmap(wxART_TICK_MARK));
    m_db_apply->Enable(false);
    m_db_apply->Bind(wxEVT_BUTTON, &Frame::OnApply, this);
    topBar->Add(m_db_apply, 0, wxALL, 5);

    m_db_discard = new wxButton(this, wxID_ANY, "Discard");
    m_db_discard->SetToolTip("Discard changes to the library");
    m_db_discard->SetBitmap(wxArtProvider::GetBitmap(wxART_UNDO));
    m_db_discard->Enable(false);
    m_db_discard->Bind(wxEVT_BUTTON, &Frame::OnDiscard, this);
    topBar->Add(m_db_discard, 0, wxALL, 5);
    
    sizer->Add(topBar);

    auto splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    splitter->SetSashGravity(0.5);

    m_tracklist = new TrackList(splitter, this, nullptr);

    auto filters = new wxWindow(splitter, wxID_ANY);
    auto filters_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_artist_filters = new ArtistFilters(filters, this, nullptr);
    m_album_filters = new AlbumFilters(filters, this, nullptr);
    
    filters_sizer->Add(m_artist_filters, 1, wxEXPAND | wxALL);
    filters_sizer->Add(m_album_filters, 1, wxEXPAND | wxALL);
    filters->SetSizer(filters_sizer);

    splitter->SplitHorizontally(filters, m_tracklist);
    sizer->Add(splitter, 1, wxEXPAND | wxALL, 5);

    SetSizer(sizer);
    SetMinSize(wxSize(400, 225));
    RefreshApp();

    Bind(wxEVT_CLOSE_WINDOW, &Frame::OnExit, this);

    if(!wxPersistentRegisterAndRestore(this, "wxPod")) {
        SetClientSize(wxSize(800, 450));
    }
}

void Frame::OnExit(wxCloseEvent &ev) {
    if(m_unsaved) {
        auto dialog = new wxMessageDialog(this, "You have unsaved changes! Exit anyway?", "Unsaved changes", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTRE);
        int ans = dialog->ShowModal();
        if(ans != wxID_YES) {
            return;
        }
    }
    ev.Skip();
}

void Frame::listDevices() {
    // TODO: needs to be improved

    m_devices.clear();

    udev* udev = m_app->m_udev;
    auto enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "disk");
    udev_enumerate_scan_devices(enumerate);
    auto devices = udev_enumerate_get_list_entry(enumerate);

    FILE* mnttable = setmntent("/proc/mounts", "r");
    mntent* ent;
    std::map<std::string, std::string> mounts;
    while((ent = getmntent(mnttable)) != NULL) {
        if(std::string(ent->mnt_fsname).rfind("/dev/sd", 0) == 0) {
            mounts[ent->mnt_fsname] = ent->mnt_dir;
        }
    }
    endmntent(mnttable);

    udev_list_entry* entry;
    udev_list_entry_foreach(entry, devices) {
        auto path = udev_list_entry_get_name(entry);
        auto device = udev_device_new_from_syspath(udev, path);
        auto removable = udev_device_get_sysattr_value(device, "removable");
        if(removable && strcmp(removable, "1") == 0) {
            if(auto dev_node = udev_device_get_devnode(device)) {
                if(auto parent_dev = udev_device_get_parent_with_subsystem_devtype(device, "usb", "usb_device")) {
                    if(auto idVendor = udev_device_get_sysattr_value(parent_dev, "idVendor")) {
                        auto vendor = std::stoi(idVendor, 0, 16);
                        if(vendor == 0x05ac) { // apple vendor ID
                            if(auto idProduct = udev_device_get_sysattr_value(parent_dev, "idProduct")) {
                                auto model = std::stoi(idProduct, 0, 16);
                                for(std::map<std::string, std::string>::iterator it = mounts.begin(); it != mounts.end(); it++) {
                                    if(it->first.rfind(dev_node, 0) == 0) {
                                        m_devices.push_back(new Device(model, it->second));
                                        break;
                                    }
                                }
                                // otherwise the device is not mounted
                            }
                        }
                    } 
                }
            }
        }
        udev_device_unref(device);
    }

    udev_enumerate_unref(enumerate);
}

void Frame::OnDeviceSelected(wxCommandEvent& ev) {
    m_device_selected = m_devices_cb->GetSelection();
    if(m_device_selected != -1) {
        m_device = m_devices[m_device_selected];
        m_device->listTracks();
        RefreshApp();
    }
}

void Frame::OnRefresh(wxCommandEvent& ev) {
    RefreshApp();
}

void Frame::OnApply(wxCommandEvent& ev) {
    if(m_unsaved) {
        m_unsaved = false;
        m_db_apply->Enable(false);
        m_db_discard->Enable(false);
        SetTitle("wxPod");
        m_device->ApplyChanges();
        RefreshApp();
    }
}

void Frame::OnDiscard(wxCommandEvent& ev) {
    if(m_unsaved) {
        m_unsaved = false;
        m_db_apply->Enable(false);
        m_db_discard->Enable(false);
        SetTitle("wxPod");
        m_device->DiscardChanges();
        RefreshApp();
    }
}

void Frame::RefreshApp() {
    listDevices();
    m_devices_cb->Clear();

    for(int i = 0; i < m_devices.size(); i++) {
        m_devices_cb->Append(wxString::Format("%s (at %s)", m_devices[i]->GetModelName(), m_devices[i]->GetMountpoint()));
    }

    if(m_device_selected < m_devices_cb->GetCount()) {
        m_devices_cb->SetSelection(m_device_selected);
    } else {
        m_devices_cb->RemoveSelection();
        m_device_selected = -1;
        m_tracklist->DeleteAllItems();
        m_artist_filters->DeleteAllItems();
        m_album_filters->DeleteAllItems();
        if(m_device != nullptr) {
            m_device->m_library.clear();
            m_device->m_artists.clear();
            m_device->m_albums.clear();
        }
    }

    if(m_device_selected >= 0) {
        m_tracklist->SetDevice(m_device);
        m_artist_filters->SetDevice(m_device);
        m_album_filters->SetDevice(m_device);
        
        m_tracklist->DeleteAllItems();
        m_artist_filters->DeleteAllItems();
        m_album_filters->DeleteAllItems();

        m_device->m_artists.clear();
        m_device->m_albums.clear();
        for(int i = 0; i < m_device->m_library.size(); i++) {
            if(m_device->m_library[i] == nullptr) continue;

            auto track = m_device->m_library[i];

            m_tracklist->AddTrack(track, i);

            Album album({track->album, track->artist});
            if(std::find(m_device->m_artists.begin(), m_device->m_artists.end(), track->artist) == m_device->m_artists.end()) {
                m_device->m_artists.push_back(track->artist);
                m_artist_filters->AddArtist(track->artist);
            }
            if(std::find_if(m_device->m_albums.begin(), m_device->m_albums.end(), [&](Album a){
                return a.artist == album.artist && a.title == album.title;
            }) == m_device->m_albums.end()) {
                m_device->m_albums.push_back(album);
                m_album_filters->AddAlbum(album.title, m_device->m_albums.size() - 1);
            }
        }

        m_artist_filters->PrependItem(wxV({"All"}), -1);
        m_artist_filters->SelectRow(0);

        m_album_filters->PrependItem(wxV({"All"}), -1);
        m_album_filters->SelectRow(0);
    }
}

void Frame::IndicateUnsavedChanges() {
    m_db_apply->Enable(true);
    m_db_discard->Enable(true);
    m_unsaved = true;
    SetTitle("*wxPod");
}

wxIMPLEMENT_APP(App);

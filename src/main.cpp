#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/dataview.h>
#include <wx/persist/toplevel.h>
#include <wx/artprov.h>
#include <itdb.h>
#include <libudev.h>
#include <mntent.h>
#include <filesystem>

#include "app.h"

#define wxV(...) [](std::vector<std::string> X){\
    wxVector<wxVariant> _;\
    for(int i = 0; i < X.size(); i++) {\
        _.push_back(wxString::FromUTF8(X[i]));\
    }\
    return _;\
}(__VA_ARGS__) // std::vector<std::string> -> wxVector<wxVariant> helper macro

#define wxV_wxs(...) [](std::vector<wxString> X){\
    wxVector<wxVariant> _;\
    for(int i = 0; i < X.size(); i++) {\
        _.push_back(X[i]);\
    }\
    return _;\
}(__VA_ARGS__) // std::vector<wxString> -> wxVector<wxVariant> helper macro

bool App::OnInit() {
    m_udev = udev_new();

    auto frame = new Frame(this);
    frame->Show();

    return true;
}

int App::OnExit() {
    udev_unref(m_udev);

    return 0;
}

inline bool FiltersDVLStore::GetAttr(const wxDataViewItem &item, unsigned int col, wxDataViewItemAttr &attr) const {
    if(GetItemData(item) == -1) {
        attr.SetBold(true);
        return true;
    }
    return wxDataViewListModel::GetAttr(item, col, attr);
}

int FiltersDVLStore::Compare(const wxDataViewItem &item1, const wxDataViewItem &item2, unsigned int column, bool ascending) const {
    bool isSpecial1 = GetItemData(item1) == -1;
    bool isSpecial2 = GetItemData(item2) == -1;
    if(isSpecial1 && !isSpecial2) return -1;
    if(!isSpecial1 && isSpecial2) return 1;
    if(isSpecial1 && isSpecial2) return 0;
    return wxDataViewListStore::Compare(item1, item2, column, ascending);
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

    m_library_table = new wxDataViewListCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE);
    m_library_table_col_title = m_library_table->AppendTextColumn("Title", wxDATAVIEW_CELL_EDITABLE, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    m_library_table->AppendTextColumn("Artist", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    m_library_table->AppendTextColumn("Album", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    m_library_table->AppendTextColumn("Duration", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    m_library_table->GetColumn(0)->SetSortOrder(true);
    m_library_table->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &Frame::OnTrackOpened, this);
    m_library_table->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &Frame::OnTrackContextMenu, this);
    m_library_table->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &Frame::OnTrackRenamed, this);

    auto filters = new wxWindow(splitter, wxID_ANY);
    auto filters_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_artists_table = new wxDataViewListCtrl(filters, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE);
    m_artists_table->AppendTextColumn("Artists", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
    m_artists_table->AssociateModel(new FiltersDVLStore());
    m_artists_table->GetColumn(0)->SetSortOrder(true);
    
    m_artists_table->AppendItem(wxV({"All"}), -1); // -1 means this item is "All" (it's "special")
    m_artists_table->SelectRow(0);
    m_artists_table->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &Frame::OnArtistsSelectionChanged, this);

    m_albums_table = new wxDataViewListCtrl(filters, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE);
    m_albums_table->AppendTextColumn("Albums", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
    m_albums_table->AssociateModel(new FiltersDVLStore());
    m_albums_table->GetColumn(0)->SetSortOrder(true);
    m_albums_table->AppendItem(wxV({"All"}), -1);
    m_albums_table->SelectRow(0);
    m_albums_table->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &Frame::OnAlbumsSelectionChanged, this);
    
    filters_sizer->Add(m_artists_table, 1, wxEXPAND | wxALL);
    filters_sizer->Add(m_albums_table, 1, wxEXPAND | wxALL);
    filters->SetSizer(filters_sizer);

    splitter->SplitHorizontally(filters, m_library_table);
    sizer->Add(splitter, 1, wxEXPAND | wxALL, 5);

    SetSizer(sizer);
    Layout();
    SetMinSize(wxSize(400, 225));
    Refresh();

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
    m_devices.clear();
    m_devices_mounts.clear();

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
                                        m_devices[it->second] = PRODUCTS[model];
                                        m_devices_mounts.push_back(it->second);
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

void Frame::listTracks() {
    m_library.clear();
    m_to_be_removed.clear();

    if(m_device_selected != -1) {
        std::string root = m_devices_mounts[m_device_selected];
        GError* err;
        m_app->m_itdb = itdb_parse(root.c_str(), &err);

        auto tracks = m_app->m_itdb->tracks;
        g_list_foreach(m_app->m_itdb->tracks, [](gpointer data, gpointer m_library) {
            Itdb_Track* track = (Itdb_Track*)data;
            ((std::vector<Itdb_Track*>*)(m_library))->push_back(track);
        }, &m_library);
    }
}


void Frame::OnDeviceSelected(wxCommandEvent& ev) {
    m_device_selected = m_devices_cb->GetSelection();
    if(m_app->m_itdb != nullptr) {
        itdb_free(m_app->m_itdb);
        m_app->m_itdb = nullptr;
    }
    listTracks();
    Refresh();
}

void Frame::OnRefresh(wxCommandEvent& ev) {
    Refresh();
}

void Frame::OnApply(wxCommandEvent& ev) {
    if(m_unsaved) {
        m_unsaved = false;
        m_db_apply->Enable(false);
        m_db_discard->Enable(false);
        SetTitle("wxPod");

        for(int i = 0; i < m_to_be_removed.size(); i++) {
            if(std::filesystem::exists(m_to_be_removed[i])) {
                std::filesystem::remove(m_to_be_removed[i]);
            }
        }
        itdb_write(m_app->m_itdb, nullptr);
    }
}

void Frame::OnDiscard(wxCommandEvent& ev) {
    if(m_unsaved) {
        m_unsaved = false;
        m_db_apply->Enable(false);
        m_db_discard->Enable(false);
        SetTitle("wxPod");
        itdb_free(m_app->m_itdb);
        listTracks();
        Refresh();
    }
}

void Frame::Refresh() {
    listDevices();
    m_devices_cb->Clear();
    wxArrayString devices;
    for(std::map<std::string, std::string>::iterator it = m_devices.begin(); it != m_devices.end(); it++) {
        m_devices_cb->Append(wxString::Format("%s (at %s)", it->second, it->first));
    }
    if(m_device_selected < m_devices_cb->GetCount()) {
        m_devices_cb->SetSelection(m_device_selected);
    } else {
        m_devices_cb->RemoveSelection();
        m_device_selected = -1;
        m_library_table->DeleteAllItems();
        m_artists_table->DeleteAllItems();
        m_albums_table->DeleteAllItems();
        m_library.clear();
        m_artists.clear();
        m_albums.clear();
    }

    if(m_device_selected >= 0) {
        m_library_table->DeleteAllItems();
        m_artists_table->DeleteAllItems();
        m_albums_table->DeleteAllItems();

        m_artists.clear();
        m_albums.clear();
        for(int i = 0; i < m_library.size(); i++) {
            if(m_library[i] == nullptr) continue;
            int h = m_library[i]->tracklen / 1000 / 3600;
            int m = (m_library[i]->tracklen / 1000 % 3600) / 60;
            int s = m_library[i]->tracklen / 1000 % 60;

            m_library_table->AppendItem(wxV_wxs({
                wxString::FromUTF8(m_library[i]->title),
                wxString::FromUTF8(m_library[i]->artist),
                wxString::FromUTF8(m_library[i]->album),
                wxString::Format("%02d:%02d:%02d", h, m, s)
            }), i);

            Album album({m_library[i]->album, m_library[i]->artist});
            if(std::find(m_artists.begin(), m_artists.end(), m_library[i]->artist) == m_artists.end()) {
                m_artists.push_back(m_library[i]->artist);
                m_artists_table->AppendItem(wxV({m_library[i]->artist}));
            }
            if(std::find_if(m_albums.begin(), m_albums.end(), [&](Album a){
                return a.artist == album.artist && a.title == album.title;
            }) == m_albums.end()) {
                m_albums.push_back(album);
                m_albums_table->AppendItem(wxV({album.title}), m_albums.size() - 1);
            }
        }

        m_artists_table->PrependItem(wxV({"All"}), -1);
        m_artists_table->SelectRow(0);

        m_albums_table->PrependItem(wxV({"All"}), -1);
        m_albums_table->SelectRow(0);
    }
}

void Frame::IndicateUnsavedChanges() {
    m_db_apply->Enable(true);
    m_db_discard->Enable(true);
    m_unsaved = true;
    SetTitle("*wxPod");
}

void Frame::OnTrackOpened(wxDataViewEvent& ev) {
    Itdb_Track* track = m_library[m_library_table->GetItemData(ev.GetItem())];
    std::string path = track->ipod_path;
    itdb_filename_ipod2fs(path.data());
    wxLaunchDefaultApplication(itdb_get_mountpoint(track->itdb) + path);
}

void Frame::OnTrackContextMenu(wxDataViewEvent& ev) {
    if(ev.GetItem().IsOk()) {
        wxMenu context;
        context.Append(TRACK_CONTEXT_MENU::RENAME, "Rename")->SetBitmap(wxArtProvider::GetBitmap(wxART_EDIT));
        context.Append(TRACK_CONTEXT_MENU::DELETE, "Delete")->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE));
        context.AppendSeparator();
        context.Append(TRACK_CONTEXT_MENU::PROPERTIES, "Properties");
        context.Bind(wxEVT_MENU, &Frame::OnTrackContextMenuButton, this, wxID_ANY);
        context.SetClientData((void*)(intptr_t)m_library_table->ItemToRow(ev.GetItem()));

        PopupMenu(&context);
    }
}

void Frame::OnTrackContextMenuButton(wxCommandEvent& ev) {
    wxDataViewItem item = m_library_table->RowToItem((intptr_t)static_cast<wxMenu*>(ev.GetEventObject())->GetClientData());
    switch(ev.GetId()) {
        case TRACK_CONTEXT_MENU::RENAME:
            m_library_table->EditItem(item, m_library_table_col_title);
            break;
        case TRACK_CONTEXT_MENU::DELETE:
        {
            auto dialog = new wxMessageDialog(this, "Are you sure you want to delete this track?", "Delete", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTRE);
            if(dialog->ShowModal() == wxID_YES) {
                std::string path = m_library[m_library_table->GetItemData(item)]->ipod_path;
                itdb_filename_ipod2fs(path.data());
                m_to_be_removed.push_back(itdb_get_mountpoint(m_app->m_itdb) + path);
                itdb_playlist_remove_track(itdb_playlist_mpl(m_app->m_itdb), m_library[m_library_table->GetItemData(item)]);
                itdb_track_remove(m_library[m_library_table->GetItemData(item)]);
                m_library[m_library_table->GetItemData(item)] = nullptr;
                m_library_table->DeleteItem(m_library_table->ItemToRow(item));
                
                IndicateUnsavedChanges();
            }
            break;
        }
        case TRACK_CONTEXT_MENU::PROPERTIES:
            break;
    }
}

void Frame::OnTrackRenamed(wxDataViewEvent& ev) {
    Itdb_Track* track = m_library[m_library_table->GetItemData(ev.GetItem())];
    std::string new_title = m_library_table->GetTextValue(m_library_table->ItemToRow(ev.GetItem()), m_library_table->GetColumnPosition(m_library_table_col_title)).utf8_string();
    if(new_title != track->title) {
        track->title = g_strdup(new_title.c_str());
        IndicateUnsavedChanges();
    }
}

void Frame::OnArtistsSelectionChanged(wxDataViewEvent& ev) {
    if(m_artists_table->IsRowSelected(0)) {
        m_artists_table->UnselectAll();
        m_artists_table->SelectRow(0);
    }
    if(m_artists_table->GetSelectedItemsCount() == 0) {
        m_artists_table->SelectRow(0);
    }
    
    UpdateAlbumList();
}

void Frame::UpdateAlbumList() {
    m_albums_table->DeleteAllItems();
    
    if(m_artists_table->IsRowSelected(0)) { // selected "All"
        for(int j = 0; j < m_albums.size(); j++) {
            m_albums_table->AppendItem(wxV({m_albums[j].title}), j);
        }
    } else {
        wxDataViewItemArray selections;
        m_artists_table->GetSelections(selections);
        for(int i = 0; i < selections.size(); i++) {
            std::string artist = m_artists_table->GetTextValue(m_artists_table->ItemToRow(selections[i]), 0).utf8_string();
            for(int j = 0; j < m_albums.size(); j++) {
                if(m_albums[j].artist == artist) {
                    m_albums_table->AppendItem(wxV({m_albums[j].title}), j);
                }
            }
        }
    }
    m_albums_table->PrependItem(wxV({"All"}), -1);
    m_albums_table->SelectRow(0);

    UpdateTrackList();
}

void Frame::OnAlbumsSelectionChanged(wxDataViewEvent& ev) {
    if(m_albums_table->IsRowSelected(0)) {
        m_albums_table->UnselectAll();
        m_albums_table->SelectRow(0);
    }
    if(m_albums_table->GetSelectedItemsCount() == 0) {
        m_albums_table->SelectRow(0);
    }

    UpdateTrackList();
}

void Frame::UpdateTrackList() {
    m_library_table->DeleteAllItems();
    if(m_albums_table->IsRowSelected(0)) { // selected "All" (albums)
        if(m_artists_table->IsRowSelected(0)) { // selected "All" (artists)
            for(int i = 0; i < m_library.size(); i++) {
                if(m_library[i] == nullptr) continue;
                int h = m_library[i]->tracklen / 1000 / 3600;
                int m = (m_library[i]->tracklen / 1000 % 3600) / 60;
                int s = m_library[i]->tracklen / 1000 % 60;

                m_library_table->AppendItem(wxV_wxs({
                    wxString::FromUTF8(m_library[i]->title),
                    wxString::FromUTF8(m_library[i]->artist),
                    wxString::FromUTF8(m_library[i]->album),
                    wxString::Format("%02d:%02d:%02d", h, m, s)
                }), i);
            }
        } else {
            wxDataViewItemArray selections;
            m_artists_table->GetSelections(selections);
            for(int i = 0; i < selections.size(); i++) {
                std::string artist = m_artists_table->GetTextValue(m_artists_table->ItemToRow(selections[i]), 0).utf8_string();
                for(int j = 0; j < m_library.size(); j++) {
                    if(m_library[j] == nullptr) continue;
                    if(m_library[j]->artist == artist) {
                        int h = m_library[j]->tracklen / 1000 / 3600;
                        int m = (m_library[j]->tracklen / 1000 % 3600) / 60;
                        int s = m_library[j]->tracklen / 1000 % 60;

                        m_library_table->AppendItem(wxV_wxs({
                            wxString::FromUTF8(m_library[j]->title),
                            wxString::FromUTF8(m_library[j]->artist),
                            wxString::FromUTF8(m_library[j]->album),
                            wxString::Format("%02d:%02d:%02d", h, m, s)
                        }), j);
                    }
                }
            }
        }
    } else {
        wxDataViewItemArray selections;
        m_albums_table->GetSelections(selections);
        for(int i = 0; i < selections.size(); i++) {
            std::string artist = m_albums[m_albums_table->GetItemData(selections[i])].artist;
            std::string album = m_albums[m_albums_table->GetItemData(selections[i])].title;
            for(int j = 0; j < m_library.size(); j++) {
                if(m_library[j] == nullptr) continue;
                if(m_library[j]->artist == artist && m_library[j]->album == album) {
                    int h = m_library[j]->tracklen / 1000 / 3600;
                    int m = (m_library[j]->tracklen / 1000 % 3600) / 60;
                    int s = m_library[j]->tracklen / 1000 % 60;

                    m_library_table->AppendItem(wxV_wxs({
                        wxString::FromUTF8(m_library[j]->title),
                        wxString::FromUTF8(m_library[j]->artist),
                        wxString::FromUTF8(m_library[j]->album),
                        wxString::Format("%02d:%02d:%02d", h, m, s)
                    }), j);
                }
            }
        }
    }
}

wxIMPLEMENT_APP(App);
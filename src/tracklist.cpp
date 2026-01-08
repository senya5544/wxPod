#include "tracklist.h"
#include "device.h"
#include "filters.h"
#include "properties.h"

#include <wx/artprov.h>

TrackList::TrackList(wxWindow* parent, Frame* frame, Device* device) : wxDataViewListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE) {
    m_frame = frame;
    m_device = device;
    m_col_title = AppendTextColumn("Title", wxDATAVIEW_CELL_EDITABLE, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    m_col_artist = AppendTextColumn("Artist", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    m_col_album = AppendTextColumn("Album", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    AppendTextColumn("Duration", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
    GetColumn(0)->SetSortOrder(true);
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &TrackList::OnTrackOpened, this);
    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &TrackList::OnContextMenu, this);
    Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &TrackList::OnTrackRenamed, this);
}

void TrackList::SetDevice(Device* device) {
    m_device = device;
}

void TrackList::RefreshList() {
    DeleteAllItems();
    if(m_frame->m_album_filters->IsRowSelected(0)) { // selected "All" (albums)
        if(m_frame->m_artist_filters->IsRowSelected(0)) { // selected "All" (artists)
            for(int i = 0; i < m_device->m_library.size(); i++) {
                if(m_device->m_library[i] == nullptr) continue;
                AddTrack(m_device->m_library[i], i);
            }
        } else {
            wxDataViewItemArray selections;
            m_frame->m_artist_filters->GetSelections(selections);
            for(int i = 0; i < selections.size(); i++) {
                std::string artist = m_frame->m_artist_filters->GetTextValue(m_frame->m_artist_filters->ItemToRow(selections[i]), 0).utf8_string();
                for(int j = 0; j < m_device->m_library.size(); j++) {
                    if(m_device->m_library[j] == nullptr) continue;
                    if(m_device->m_library[j]->artist == artist) {
                        AddTrack(m_device->m_library[j], j);
                    }
                }
            }
        }
    } else {
        wxDataViewItemArray selections;
        m_frame->m_album_filters->GetSelections(selections);
        for(int i = 0; i < selections.size(); i++) {
            Album album = m_device->m_albums[m_frame->m_album_filters->GetItemData(selections[i])];
            for(int j = 0; j < m_device->m_library.size(); j++) {
                if(m_device->m_library[j] == nullptr) continue;
                if(m_device->m_library[j]->artist == album.artist && m_device->m_library[j]->album == album.title) {
                    AddTrack(m_device->m_library[j], j);
                }
            }
        }
    }
}

void TrackList::AddTrack(Itdb_Track* track, int data) {
    int h = track->tracklen / 1000 / 3600;
    int m = (track->tracklen / 1000 % 3600) / 60;
    int s = track->tracklen / 1000 % 60;

    AppendItem(wxV_wxs({
            wxString::FromUTF8(track->title),
            wxString::FromUTF8(track->artist),
            wxString::FromUTF8(track->album),
            wxString::Format("%02d:%02d:%02d", h, m, s)
    }), data);
}

void TrackList::OnTrackOpened(wxDataViewEvent& ev) {
    Itdb_Track* track = m_device->m_library[GetItemData(ev.GetItem())];
    std::string path = track->ipod_path;
    itdb_filename_ipod2fs(path.data());
    wxLaunchDefaultApplication(itdb_get_mountpoint(track->itdb) + path);
}

void TrackList::OnContextMenu(wxDataViewEvent& ev) {
    if(ev.GetItem().IsOk()) {
        
        UnselectAll();
        Select(ev.GetItem());

        wxMenu context;
        context.Append(CONTEXT_MENU::RENAME, "Rename")->SetBitmap(wxArtProvider::GetBitmap(wxART_EDIT));
        context.Append(CONTEXT_MENU::DELETE, "Delete")->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE));
        context.AppendSeparator();
        context.Append(CONTEXT_MENU::PROPERTIES, "Properties");
        context.Bind(wxEVT_MENU, &TrackList::OnContextMenuButton, this, wxID_ANY);
        context.SetClientData((void*)(intptr_t)ItemToRow(ev.GetItem()));

        PopupMenu(&context);
    }
}

void TrackList::OnContextMenuButton(wxCommandEvent& ev) {
    wxDataViewItem item = RowToItem((intptr_t)static_cast<wxMenu*>(ev.GetEventObject())->GetClientData());
    if(!item.IsOk()) return;
    switch(ev.GetId()) {
        case CONTEXT_MENU::RENAME:
            EditItem(item, m_col_title);
            break;
        case CONTEXT_MENU::DELETE:
        {
            // TODO: add Device::RemoveTrack() method instead
            auto dialog = new wxMessageDialog(this, "Are you sure you want to delete this track?", "Delete", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTRE);
            if(dialog->ShowModal() == wxID_YES) {
                std::string path = m_device->m_library[GetItemData(item)]->ipod_path;
                itdb_filename_ipod2fs(path.data());
                m_device->m_to_be_removed.push_back(itdb_get_mountpoint(m_device->m_itdb) + path);
                itdb_playlist_remove_track(itdb_playlist_mpl(m_device->m_itdb), m_device->m_library[GetItemData(item)]);
                itdb_track_remove(m_device->m_library[GetItemData(item)]);
                m_device->m_library[GetItemData(item)] = nullptr;
                DeleteItem(ItemToRow(item));
                
                m_frame->IndicateUnsavedChanges();
            }
            break;
        }
        case CONTEXT_MENU::PROPERTIES:
            auto properties = new TrackPropertiesDialog(m_frame, m_device, m_device->m_library[GetItemData(item)], item);
            properties->Show();

            break;
    }
}

void TrackList::OnTrackRenamed(wxDataViewEvent& ev) {
    if(!ev.GetItem().IsOk()) return;
    Itdb_Track* track = m_device->m_library[GetItemData(ev.GetItem())];
    std::string new_title = GetTextValue(ItemToRow(ev.GetItem()), GetColumnPosition(m_col_title)).utf8_string();
    if(new_title != track->title) {
        track->title = g_strdup(new_title.c_str());
        m_frame->IndicateUnsavedChanges();
    }
}
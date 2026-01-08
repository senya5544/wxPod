#include "filters.h"
#include "device.h"
#include "tracklist.h"
#include "properties.h"

#include <wx/artprov.h>

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

ArtistFilters::ArtistFilters(wxWindow* parent, Frame* frame, Device* device) : wxDataViewListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE) {
    m_frame = frame;
    m_device = device;
    m_col = AppendTextColumn("Artists", wxDATAVIEW_CELL_EDITABLE, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
    AssociateModel(new FiltersDVLStore());
    GetColumn(0)->SetSortOrder(true);
    AppendItem(wxV({"All"}), -1); // -1 means this item is "All" (it's "special")
    SelectRow(0);
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ArtistFilters::OnSelectionChanged, this);
}

void ArtistFilters::SetDevice(Device* device) {
    m_device = device;
}

void ArtistFilters::AddArtist(std::string artist) {
    AppendItem(wxV({artist}));
}

void ArtistFilters::OnSelectionChanged(wxDataViewEvent& ev) {
    if(IsRowSelected(0)) {
        UnselectAll();
        SelectRow(0);
    }
    if(GetSelectedItemsCount() == 0) {
        SelectRow(0);
    }
    
    m_frame->m_album_filters->RefreshList();
}

AlbumFilters::AlbumFilters(wxWindow* parent, Frame* frame, Device* device) : wxDataViewListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE) {
    m_frame = frame;
    m_device = device;
    m_col = AppendTextColumn("Albums", wxDATAVIEW_CELL_EDITABLE, -1, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
    AssociateModel(new FiltersDVLStore());
    GetColumn(0)->SetSortOrder(true);
    AppendItem(wxV({"All"}), -1);
    SelectRow(0);
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &AlbumFilters::OnSelectionChanged, this);
    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &AlbumFilters::OnContextMenu, this);
    Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &AlbumFilters::OnAlbumRenamingStarted, this);
    Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &AlbumFilters::OnAlbumRenamed, this);
}

void AlbumFilters::SetDevice(Device* device) {
    m_device = device;
}

void AlbumFilters::AddAlbum(std::string album, int data) {
    AppendItem(wxV({album}), data);
}

void AlbumFilters::RefreshList() {
    DeleteAllItems();

    if(m_frame->m_artist_filters->IsRowSelected(0)) { // selected "All"
        for(int j = 0; j < m_device->m_albums.size(); j++) {
            AppendItem(wxV({m_device->m_albums[j].title}), j);
        }
    } else {
        wxDataViewItemArray selections;
        m_frame->m_artist_filters->GetSelections(selections);
        for(int i = 0; i < selections.size(); i++) {
            std::string artist = m_frame->m_artist_filters->GetTextValue(m_frame->m_artist_filters->ItemToRow(selections[i]), 0).utf8_string();
            for(int j = 0; j < m_device->m_albums.size(); j++) {
                if(m_device->m_albums[j].artist == artist) {
                    AppendItem(wxV({m_device->m_albums[j].title}), j);
                }
            }
        }
    }
    PrependItem(wxV({"All"}), -1);
    SelectRow(0);

    m_frame->m_tracklist->RefreshList();
}

void AlbumFilters::OnSelectionChanged(wxDataViewEvent& ev) {
    if(IsRowSelected(0)) {
        UnselectAll();
        SelectRow(0);
    }
    if(GetSelectedItemsCount() == 0) {
        SelectRow(0);
    }

    m_frame->m_tracklist->RefreshList();
}

void AlbumFilters::OnContextMenu(wxDataViewEvent& ev) {
    if(ev.GetItem().IsOk()) {
        if(GetItemData(ev.GetItem()) == -1) return;
        UnselectAll();
        Select(ev.GetItem());
        m_frame->m_tracklist->RefreshList();

        wxMenu context;
        context.Append(CONTEXT_MENU::RENAME, "Rename")->SetBitmap(wxArtProvider::GetBitmap(wxART_EDIT));
        context.Append(CONTEXT_MENU::DELETE, "Delete")->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE));
        context.AppendSeparator();
        context.Append(CONTEXT_MENU::PROPERTIES, "Properties");
        context.Bind(wxEVT_MENU, &AlbumFilters::OnContextMenuButton, this, wxID_ANY);
        context.SetClientData((void*)(intptr_t)ItemToRow(ev.GetItem()));

        PopupMenu(&context);
    }
}

void AlbumFilters::OnContextMenuButton(wxCommandEvent& ev) {
    wxDataViewItem item = RowToItem((intptr_t)static_cast<wxMenu*>(ev.GetEventObject())->GetClientData());
    if(!item.IsOk()) return;
    if(GetItemData(item) == -1) return;
    switch(ev.GetId()) {
        case CONTEXT_MENU::RENAME:
            EditItem(item, m_col);
            break;
        case CONTEXT_MENU::DELETE:
        {
            // TODO: add Device::RemoveAlbum() method instead
            auto dialog = new wxMessageDialog(this, "Are you sure you want to delete this album?", "Delete", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTRE);
            if(dialog->ShowModal() == wxID_YES) {
                for(int i = 0; i < m_device->m_library.size(); i++) {
                    if(m_device->m_library[i] != nullptr && m_device->m_library[i]->artist == m_device->m_albums[GetItemData(item)].artist && m_device->m_library[i]->album == m_device->m_albums[GetItemData(item)].title) {
                        std::string path = m_device->m_library[i]->ipod_path;
                        itdb_filename_ipod2fs(path.data());
                        m_device->m_to_be_removed.push_back(itdb_get_mountpoint(m_device->m_itdb) + path);
                        itdb_playlist_remove_track(itdb_playlist_mpl(m_device->m_itdb), m_device->m_library[i]);
                        itdb_track_remove(m_device->m_library[i]);
                        m_device->m_library[i] = nullptr;
                    }
                }
                DeleteItem(ItemToRow(item));
                m_frame->IndicateUnsavedChanges();
            }
            break;
        }
        case CONTEXT_MENU::PROPERTIES:
            auto properties = new AlbumPropertiesDialog(m_frame, m_device, m_device->m_albums[GetItemData(item)], item);
            properties->Show();
            break;
    }
}

void AlbumFilters::OnAlbumRenamingStarted(wxDataViewEvent& ev) {
    if(!ev.GetItem().IsOk()) return;
    if(GetItemData(ev.GetItem()) == -1) {
        ev.Veto();
        return;
    }
}

void AlbumFilters::OnAlbumRenamed(wxDataViewEvent& ev) {
    if(!ev.GetItem().IsOk()) return;
    if(GetItemData(ev.GetItem()) == -1) return;
    Album album = m_device->m_albums[GetItemData(ev.GetItem())];
    std::string new_title = GetTextValue(ItemToRow(ev.GetItem()), GetColumnPosition(m_col)).utf8_string();
    if(new_title != album.title) {
        for(int i = 0; i < m_device->m_library.size(); i++) {
            if(m_device->m_library[i] != nullptr && m_device->m_library[i]->album == album.title) {
                m_device->m_library[i]->album = g_strdup(new_title.c_str());
            }
        }
        for(int i = 0; i < m_frame->m_tracklist->GetItemCount(); i++) {
            if(m_frame->m_tracklist->GetTextValue(i, m_frame->m_tracklist->GetColumnPosition(m_frame->m_tracklist->m_col_album)) == album.title) {
                m_frame->m_tracklist->SetTextValue(new_title, i, m_frame->m_tracklist->GetColumnPosition(m_frame->m_tracklist->m_col_album));
            }
        }
        m_device->m_albums[GetItemData(ev.GetItem())].title = new_title;
        m_frame->IndicateUnsavedChanges();
    }
}
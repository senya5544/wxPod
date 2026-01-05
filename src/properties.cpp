#include "app.h"

#include <wx/artprov.h>

wxTextCtrl* AddProperty(wxWindow* parent, wxFlexGridSizer* sizer, std::string name, std::string value) {
    sizer->Add(new wxStaticText(parent, wxID_ANY, name + ":"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    auto input = new wxTextCtrl(parent, wxID_ANY, wxString::FromUTF8(value));
    sizer->Add(input, 0, wxALL | wxEXPAND, 10);
    return input;
}

TrackPropertiesDialog::TrackPropertiesDialog(Frame* frame, Itdb_Track* track, wxDataViewItem item) : wxDialog(nullptr, wxID_ANY, wxString::Format("%s Properties", wxString::FromUTF8(track->title)), wxDefaultPosition, wxSize(350, 300)) {
    m_frame = frame;
    m_track = track;
    m_item = item;

    auto sizer = new wxBoxSizer(wxVERTICAL);

    auto track_properties = new wxPanel(this);
    auto track_sizer = new wxFlexGridSizer(4, 2, -8, 0);
    track_sizer->AddGrowableCol(1);
    track_properties->SetSizer(track_sizer);

    m_title_input = AddProperty(track_properties, track_sizer, "Title", track->title);
    m_artist_input = AddProperty(track_properties, track_sizer, "Artist", track->artist);
    m_album_input = AddProperty(track_properties, track_sizer, "Album", track->album);
    m_genre_input = AddProperty(track_properties, track_sizer, "Genre", track->genre);

    sizer->Add(track_properties, 1, wxALL | wxEXPAND, 10);
    
    auto dialog_button_sizer_v = new wxBoxSizer(wxVERTICAL);
    auto dialog_button_sizer_h = new wxBoxSizer(wxHORIZONTAL);

    auto ok_button = new wxButton(this, wxID_ANY, "OK");
    ok_button->Bind(wxEVT_BUTTON, &TrackPropertiesDialog::OnOk, this);

    auto cancel_button = new wxButton(this, wxID_ANY, "Cancel");
    cancel_button->Bind(wxEVT_BUTTON, &TrackPropertiesDialog::OnCancel, this);

    dialog_button_sizer_h->Add(ok_button, 0, wxALL, 2);
    dialog_button_sizer_h->Add(cancel_button, 0, wxALL, 2);
    dialog_button_sizer_v->Add(dialog_button_sizer_h, 0, wxALL | wxALIGN_RIGHT);
    
    sizer->Add(dialog_button_sizer_v, 0, wxALL | wxEXPAND, 10);

    SetSizer(sizer);

    Bind(wxEVT_CLOSE_WINDOW, &TrackPropertiesDialog::OnExit, this);
}

void TrackPropertiesDialog::OnOk(wxCommandEvent& ev) {
    std::string new_title = m_title_input->GetValue().utf8_string();
    std::string new_artist = m_artist_input->GetValue().utf8_string();
    std::string new_album = m_album_input->GetValue().utf8_string();
    std::string new_genre = m_genre_input->GetValue().utf8_string();

    int row = m_frame->m_library_table->ItemToRow(m_item);

    if(new_title != m_track->title) {
        m_track->title = g_strdup(new_title.c_str());
        m_frame->m_library_table->SetTextValue(wxString::FromUTF8(new_title), row, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_title));
        m_frame->IndicateUnsavedChanges();
    }
    if(new_artist != m_track->artist) {
        m_track->artist = g_strdup(new_artist.c_str());
        m_frame->m_library_table->SetTextValue(wxString::FromUTF8(new_artist), row, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_artist));
        m_frame->IndicateUnsavedChanges();
    }
    if(new_album != m_track->album) {
        m_track->album = g_strdup(new_album.c_str());
        m_frame->m_library_table->SetTextValue(wxString::FromUTF8(new_album), row, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_album));
        m_frame->IndicateUnsavedChanges();
    }
    if(new_genre != m_track->genre) {
        m_track->genre = g_strdup(new_genre.c_str());
        m_frame->IndicateUnsavedChanges();
    }
    // note to self: the library is meant not to resort after the user edited the properties so they can see the changes

    Close();
}

void TrackPropertiesDialog::OnCancel(wxCommandEvent& ev) {
    Close();
}

void TrackPropertiesDialog::OnExit(wxCloseEvent& ev) {
    Destroy();
}


AlbumPropertiesDialog::AlbumPropertiesDialog(Frame* frame, Album album, wxDataViewItem item) : wxDialog(nullptr, wxID_ANY, wxString::Format("%s Properties", wxString::FromUTF8(album.title)), wxDefaultPosition, wxSize(350, 450)) {
    m_frame = frame;
    m_album = album;
    m_item = item;

    auto sizer = new wxBoxSizer(wxVERTICAL);

    auto album_properties = new wxPanel(this);
    auto album_sizer = new wxFlexGridSizer(2, 2, -8, 0);
    album_sizer->AddGrowableCol(1);
    album_properties->SetSizer(album_sizer);

    m_title_input = AddProperty(album_properties, album_sizer, "Album title", album.title);
    m_artist_input = AddProperty(album_properties, album_sizer, "Album artist", album.artist);

    sizer->Add(album_properties, 1, wxALL | wxEXPAND, 10);

    gpointer pixbuf;
    for(int i = 0; i < m_frame->m_library.size(); i++) {
        if(m_frame->m_library[i]->album == m_album.title && m_frame->m_library[i]->artist == m_album.artist) {
            pixbuf = itdb_track_get_thumbnail(m_frame->m_library[i], -1, -1);
            break;
        }
    }
    if(pixbuf != NULL) {
        auto image = wxBitmap((GdkPixbuf*)pixbuf).ConvertToImage();
        auto bitmap = new wxStaticBitmap(this, wxID_ANY, wxBitmapBundle(wxBitmap(
            image.Rescale(image.GetWidth() * 1.7, image.GetHeight() * 1.7, wxIMAGE_QUALITY_HIGH)
        )));
        image.Destroy();
        sizer->Add(bitmap, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
        // unref pixbuf somehow
    }

    auto cover_sizer = new wxBoxSizer(wxHORIZONTAL);

    auto browse = new wxButton(this, wxID_ANY, "Browse");
    browse->SetBitmap(wxArtProvider::GetBitmap(wxART_FILE_OPEN));
    browse->SetToolTip("Browse local files for album cover art");
    cover_sizer->Add(browse, 0, wxALL | wxEXPAND, 5);

    auto fetch = new wxButton(this, wxID_ANY, "Fetch");
    fetch->SetBitmap(wxArtProvider::GetBitmap(wxART_REFRESH));
    fetch->SetToolTip("Fetch album cover art from the audio file");
    cover_sizer->Add(fetch, 0, wxALL | wxEXPAND, 5);

    auto remove = new wxButton(this, wxID_ANY, "Remove");
    remove->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE));
    remove->SetToolTip("Remove album cover art");
    cover_sizer->Add(remove, 0, wxALL | wxEXPAND, 5);

    sizer->Add(cover_sizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

    auto dialog_button_sizer_v = new wxBoxSizer(wxVERTICAL);
    auto dialog_button_sizer_h = new wxBoxSizer(wxHORIZONTAL);

    auto ok_button = new wxButton(this, wxID_ANY, "OK");
    ok_button->Bind(wxEVT_BUTTON, &AlbumPropertiesDialog::OnOk, this);

    auto cancel_button = new wxButton(this, wxID_ANY, "Cancel");
    cancel_button->Bind(wxEVT_BUTTON, &AlbumPropertiesDialog::OnCancel, this);

    dialog_button_sizer_h->Add(ok_button, 0, wxALL, 2);
    dialog_button_sizer_h->Add(cancel_button, 0, wxALL, 2);
    dialog_button_sizer_v->Add(dialog_button_sizer_h, 0, wxALL | wxALIGN_RIGHT);
    
    sizer->Add(dialog_button_sizer_v, 0, wxALL | wxEXPAND, 10);

    SetSizer(sizer);

    Bind(wxEVT_CLOSE_WINDOW, &AlbumPropertiesDialog::OnExit, this);
}

void AlbumPropertiesDialog::OnOk(wxCommandEvent& ev) {
    std::string new_title = m_title_input->GetValue().utf8_string();
    std::string new_artist = m_artist_input->GetValue().utf8_string();

    int row = m_frame->m_albums_table->ItemToRow(m_item);
    int index = (int)m_frame->m_albums_table->GetItemData(m_item);
    
    if(new_title != m_album.title) {
        for(int i = 0; i < m_frame->m_library.size(); i++) {
            if(m_frame->m_library[i]->album == m_album.title) {
                m_frame->m_library[i]->album = g_strdup(new_title.c_str());
            }
        }
        for(int i = 0; i < m_frame->m_library_table->GetItemCount(); i++) {
            if(m_frame->m_library_table->GetTextValue(i, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_album)) == m_album.title) {
                m_frame->m_library_table->SetTextValue(new_title, i, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_album));
            }
        }
        m_album.title = new_title;
        m_frame->m_albums[m_frame->m_albums_table->GetItemData(m_item)].title = new_title;

        m_frame->m_albums_table->SetTextValue(wxString::FromUTF8(new_title), row, m_frame->m_albums_table->GetColumnPosition(m_frame->m_albums_table_col));
        m_frame->IndicateUnsavedChanges();
    }
    if(new_artist != m_album.artist) {
        for(int i = 0; i < m_frame->m_library.size(); i++) {
            if(m_frame->m_library[i] != nullptr && m_frame->m_library[i]->album == m_album.title && m_frame->m_library[i]->artist == m_album.artist) {
                m_frame->m_library[i]->artist = g_strdup(new_artist.c_str());
            }
        }
        for(int i = 0; i < m_frame->m_library_table->GetItemCount(); i++) {
            if(m_frame->m_library_table->GetTextValue(i, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_album)) == m_album.title && m_frame->m_library_table->GetTextValue(i, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_artist)) == m_album.artist) {
                m_frame->m_library_table->SetTextValue(new_artist, i, m_frame->m_library_table->GetColumnPosition(m_frame->m_library_table_col_artist));
            }
        }
        m_album.artist = new_artist;
        m_frame->m_albums[m_frame->m_albums_table->GetItemData(m_item)].artist = new_artist;

        m_frame->IndicateUnsavedChanges();
    }

    Close();
}

void AlbumPropertiesDialog::OnCancel(wxCommandEvent& ev) {
    Close();
}

void AlbumPropertiesDialog::OnExit(wxCloseEvent& ev) {
    Destroy();
}

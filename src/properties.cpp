#include "app.h"

#include <wx/artprov.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <wx/mstream.h>

wxTextCtrl* AddProperty(wxWindow* parent, wxFlexGridSizer* sizer, std::string name, std::string value) {
    sizer->Add(new wxStaticText(parent, wxID_ANY, name + ":"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    auto input = new wxTextCtrl(parent, wxID_ANY, wxString::FromUTF8(value));
    sizer->Add(input, 0, wxALL | wxEXPAND, 10);
    return input;
}

wxSize FitImage(wxSize size1, wxSize size2) {
    if(size1.GetWidth() == size1.GetHeight()) {
        return wxSize(size2.GetWidth(), size2.GetHeight());
    } else if(size1.GetWidth() > size1.GetHeight()) {
        return wxSize(size2.GetWidth(), size2.GetWidth() * ((float)size1.GetHeight() / (float)size1.GetWidth()));
    } else {
        return wxSize(size2.GetHeight() * ((float)size1.GetWidth() / (float)size1.GetHeight()), size2.GetHeight());
    }
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

    // TODO: add previous&next buttons
    // idk how to implement this since frame->m_library_table->ItemToRow() returns the row before sorting not the current one

    /*
        auto prev_button = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap(wxART_GO_BACK));
        prev_button->Bind(wxEVT_BUTTON, &TrackPropertiesDialog::OnPrevious, this);
        if(frame->m_library_table->ItemToRow(item) == 0) {
            prev_button->Enable(false);
        }

        auto next_button = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap(wxART_GO_FORWARD));
        next_button->Bind(wxEVT_BUTTON, &TrackPropertiesDialog::OnNext, this);
        if(frame->m_library_table->ItemToRow(item) == frame->m_library_table->GetItemCount() - 1) {
            next_button->Enable(false);
        }
    */

    auto ok_button = new wxButton(this, wxID_ANY, "OK");
    ok_button->Bind(wxEVT_BUTTON, &TrackPropertiesDialog::OnOk, this);

    auto cancel_button = new wxButton(this, wxID_ANY, "Cancel");
    cancel_button->Bind(wxEVT_BUTTON, &TrackPropertiesDialog::OnCancel, this);

    //dialog_button_sizer_h->Add(prev_button, 0, wxALL | wxEXPAND, 2);
    //dialog_button_sizer_h->Add(next_button, 0, wxALL | wxEXPAND, 2);
    dialog_button_sizer_h->Add(ok_button, 0, wxALL | wxEXPAND, 2);
    dialog_button_sizer_h->Add(cancel_button, 0, wxALL | wxEXPAND, 2);
    dialog_button_sizer_v->Add(dialog_button_sizer_h, 0, wxALL | wxALIGN_RIGHT);
    
    sizer->Add(dialog_button_sizer_v, 0, wxALL | wxEXPAND, 10);

    SetSizer(sizer);

    Bind(wxEVT_CLOSE_WINDOW, &TrackPropertiesDialog::OnExit, this);
}

/*
    void TrackPropertiesDialog::OnPrevious(wxCommandEvent& ev) {
    }

    void TrackPropertiesDialog::OnNext(wxCommandEvent& ev) {
    }
*/

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


AlbumPropertiesDialog::AlbumPropertiesDialog(Frame* frame, Album album, wxDataViewItem item) : wxDialog(nullptr, wxID_ANY, wxString::Format("%s Properties", wxString::FromUTF8(album.title)), wxDefaultPosition, wxSize(350, 415)) {
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
        if(m_frame->m_library[i] != nullptr && m_frame->m_library[i]->album == m_album.title && m_frame->m_library[i]->artist == m_album.artist) {
            pixbuf = itdb_track_get_thumbnail(m_frame->m_library[i], -1, -1);
            break;
        }
    }

    wxSize target_size(150, 150);
    m_bitmap = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap(wxART_MISSING_IMAGE, wxString::FromAscii("wxART_OTHER_C"), target_size), wxDefaultPosition, target_size);
    if(pixbuf != NULL) {
        auto image = wxBitmap((GdkPixbuf*)pixbuf).ConvertToImage();
        auto image_size = FitImage(image.GetSize(), target_size);
        m_bitmap->SetBitmap(wxBitmapBundle(wxBitmap(
            image.Rescale(image_size.GetWidth(), image_size.GetHeight(), wxIMAGE_QUALITY_HIGH)
        )));
        image.Destroy();
        // unref pixbuf somehow
    } else {
        m_missing_artwork = true;
    }
    sizer->Add(m_bitmap, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);

    auto cover_sizer = new wxBoxSizer(wxHORIZONTAL);

    auto browse = new wxButton(this, wxID_ANY, "Browse");
    browse->SetBitmap(wxArtProvider::GetBitmap(wxART_FOLDER));
    browse->SetToolTip("Browse local files for the artwork");
    browse->Bind(wxEVT_BUTTON, &AlbumPropertiesDialog::OnBrowse, this);
    cover_sizer->Add(browse, 0, wxALL | wxEXPAND, 5);

    auto fetch = new wxButton(this, wxID_ANY, "Use embedded");
    fetch->SetBitmap(wxArtProvider::GetBitmap(wxART_FILE_OPEN));
    fetch->SetToolTip("Use the artwork embedded into the audio file");
    fetch->Bind(wxEVT_BUTTON, &AlbumPropertiesDialog::OnFetch, this);
    cover_sizer->Add(fetch, 0, wxALL | wxEXPAND, 5);

    auto remove = new wxButton(this, wxID_ANY, "Remove");
    remove->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE));
    remove->SetToolTip("Remove artwork");
    remove->Bind(wxEVT_BUTTON, &AlbumPropertiesDialog::OnRemove, this);
    cover_sizer->Add(remove, 0, wxALL | wxEXPAND, 5);

    sizer->Add(cover_sizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

    auto dialog_button_sizer_v = new wxBoxSizer(wxVERTICAL);
    auto dialog_button_sizer_h = new wxBoxSizer(wxHORIZONTAL);

    auto ok_button = new wxButton(this, wxID_ANY, "OK");
    ok_button->Bind(wxEVT_BUTTON, &AlbumPropertiesDialog::OnOk, this);

    auto cancel_button = new wxButton(this, wxID_ANY, "Cancel");
    cancel_button->Bind(wxEVT_BUTTON, &AlbumPropertiesDialog::OnCancel, this);

    dialog_button_sizer_h->Add(ok_button, 0, wxALL | wxEXPAND, 2);
    dialog_button_sizer_h->Add(cancel_button, 0, wxALL | wxEXPAND, 2);
    dialog_button_sizer_v->Add(dialog_button_sizer_h, 0, wxALL | wxALIGN_RIGHT);
    
    sizer->Add(dialog_button_sizer_v, 0, wxALL | wxEXPAND, 10);

    SetSizer(sizer);

    Bind(wxEVT_CLOSE_WINDOW, &AlbumPropertiesDialog::OnExit, this);
}

void AlbumPropertiesDialog::OnBrowse(wxCommandEvent& ev) {
    auto dialog = wxFileDialog(this, "Select artwork", wxEmptyString, wxEmptyString, "PNG files (*.png)|*.png|JPG files (*.jpg;*.jpeg)|*.jpg;*.jpeg|BMP files (*.bmp)|*.bmp");
    if(dialog.ShowModal() == wxID_CANCEL) return;

    auto bitmap = wxBitmap(dialog.GetPath());
    auto image = bitmap.ConvertToImage();
    auto image_size = FitImage(image.GetSize(), wxSize(150, 150));
    m_bitmap->SetBitmap(wxBitmapBundle(wxBitmap(
        image.Rescale(image_size.GetWidth(), image_size.GetHeight(), wxIMAGE_QUALITY_HIGH)
    )));
    image.Destroy();

    m_remove_artwork = false;
    m_new_artwork.clear();
    m_new_artwork_path = dialog.GetPath().utf8_string();
}

void AlbumPropertiesDialog::OnFetch(wxCommandEvent& ev) {
    for(int i = 0; i < m_frame->m_library.size(); i++) {
        if(m_frame->m_library[i] != nullptr && m_frame->m_library[i]->album == m_album.title && m_frame->m_library[i]->artist == m_album.artist) {
            std::string ipod_path = m_frame->m_library[i]->ipod_path;
            itdb_filename_ipod2fs(ipod_path.data());
            std::string path = itdb_get_mountpoint(m_frame->m_library[i]->itdb) + ipod_path;

            TagLib::MPEG::File file(path.c_str());
            auto tag = file.ID3v2Tag();
            if(!tag) continue;
            auto frames = tag->frameListMap()["APIC"];
            if(frames.isEmpty()) continue;
            for(int i = 0; i < frames.size(); i++) {
                TagLib::ID3v2::AttachedPictureFrame* picFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames[i]);

                if(picFrame && picFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                    TagLib::ByteVector pictureData = picFrame->picture();
                    
                    wxImage image;
                    if(picFrame->mimeType() == "image/png") {
                        wxLogNull logNo; // suppress "iCCP: known incorrect sRGB profile" warning
                        auto bitmap = wxBitmap::NewFromPNGData(pictureData.data(), pictureData.size());
                        image = bitmap.ConvertToImage();
                    } else if(picFrame->mimeType() == "image/jpeg") {
                        wxMemoryInputStream stream(pictureData.data(), pictureData.size());
                        image.LoadFile(stream, wxBITMAP_TYPE_JPEG);
                    } else {
                        wxMessageDialog(this, "Unknown album artwork image format.", "Use embedded", wxOK | wxICON_ERROR).ShowModal();
                        return;
                    }
                    
                    auto image_size = FitImage(image.GetSize(), wxSize(150, 150));
                    m_bitmap->SetBitmap(wxBitmapBundle(wxBitmap(
                        image.Rescale(image_size.GetWidth(), image_size.GetHeight(), wxIMAGE_QUALITY_HIGH)
                    )));
                    image.Destroy();

                    m_new_artwork_path = "";
                    m_remove_artwork = false;
                    m_new_artwork = pictureData;
                    pictureData.clear();

                    return;
                }
            }
        }
    }
    wxMessageDialog(this, "Couldn't find any embedded album artworks.", "Use embedded", wxOK | wxICON_ERROR).ShowModal();
}

void AlbumPropertiesDialog::OnRemove(wxCommandEvent& ev) {
    if(!m_new_artwork_path.empty() || !m_new_artwork.isEmpty() || !m_missing_artwork) {
        m_new_artwork.clear();
        m_new_artwork_path = "";
        m_bitmap->SetBitmap(wxArtProvider::GetBitmap(wxART_MISSING_IMAGE, wxString::FromAscii("wxART_OTHER_C"), wxSize(150, 150)));
    }
    if(!m_missing_artwork) m_remove_artwork = true;
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
    if(!m_new_artwork_path.empty()) {
        for(int i = 0; i < m_frame->m_library.size(); i++) {
            if(m_frame->m_library[i] != nullptr && m_frame->m_library[i]->album == m_album.title && m_frame->m_library[i]->artist == m_album.artist) {
                itdb_track_set_thumbnails(m_frame->m_library[i], g_strdup(m_new_artwork_path.c_str()));
            }
        }
        m_frame->IndicateUnsavedChanges();
    }
    if(!m_new_artwork.isEmpty()) {
        for(int i = 0; i < m_frame->m_library.size(); i++) {
            if(m_frame->m_library[i] != nullptr && m_frame->m_library[i]->album == m_album.title && m_frame->m_library[i]->artist == m_album.artist) {
                itdb_track_set_thumbnails_from_data(m_frame->m_library[i], (const guchar*)m_new_artwork.data(), m_new_artwork.size());
            }
        }
        m_new_artwork.clear();
        m_frame->IndicateUnsavedChanges();
    }
    if(m_remove_artwork) {
        for(int i = 0; i < m_frame->m_library.size(); i++) {
            if(m_frame->m_library[i] != nullptr && m_frame->m_library[i]->album == m_album.title && m_frame->m_library[i]->artist == m_album.artist) {
                itdb_track_remove_thumbnails(m_frame->m_library[i]);
            }
        }
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

#include "cMain.h"


#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>


#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


using boost::asio::ip::tcp;

enum { max_length = 1024 };

BEGIN_EVENT_TABLE(cMain, wxFrame)
    EVT_BUTTON(BUTTON_GetPing, cMain::OnGetPing) // Tell the OS to run MainFrame::OnExit when
END_EVENT_TABLE() // The button is pressed



cMain::cMain(): wxFrame(nullptr, wxID_ANY,"Ping Monitor")
{
    txtPingEvents = new wxTextCtrl(this, TEXT_PingEvents, "", wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_RICH, wxDefaultValidator, wxTextCtrlNameStr);

    cmdGetPing = new wxButton(this, BUTTON_GetPing, _T("Get ping"),  wxDefaultPosition, wxDefaultSize, 0); 
    

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    // create text ctrl with minimal size 100x60
    topsizer->Add(txtPingEvents,
        1,            // make vertically stretchable
        wxEXPAND |    // make horizontally stretchable
        wxALL,        //   and make border all around
        10);         // set border width to 10

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->Add(cmdGetPing,
        0,           // make horizontally unstretchable
        wxALL,       // make border all around (implicit top alignment)
        10);        // set border width to 10
    button_sizer->Add(
        new wxButton(this, wxID_CANCEL, "Cancel"),
        0,           // make horizontally unstretchable
        wxALL,       // make border all around (implicit top alignment)
        10);        // set border width to 10
    
    topsizer->Add(
        button_sizer,
        0,                // make vertically unstretchable
        wxALIGN_CENTER); // no border and centre horizontally
    SetSizerAndFit(topsizer); // use the sizer for layout and size window
                              // accordingly and prevent it from being resized
                              // to smaller size

    CreateStatusBar(2);
    SetStatusText(wxT("Ready"), 0);
    SetStatusText(wxT("test"), 1);

    Layout();
    Maximize();
}

cMain::~cMain()
{
}


void cMain::OnExit(wxCommandEvent& event)
{
    Close(TRUE); // Tells the OS to quit running this process
}



void cMain::OnGetPing(wxCommandEvent& event)
{
    try
    {
        boost::asio::io_context io_context;

        tcp::socket s(io_context);
        tcp::resolver resolver(io_context);
        boost::asio::connect(s, resolver.resolve("127.0.0.1", "4444"));

        
        char request[max_length] = "client msg";
      
        size_t request_length = std::strlen(request);
        boost::asio::write(s, boost::asio::buffer(request, request_length));

        char reply[max_length];
        //TODO:implement readfully
        size_t reply_length = s.read_some(boost::asio::buffer(reply));


        std::stringstream b;
        b << "reply_length: " << reply_length << "\n";
        b.write(reply, reply_length);
       
        txtPingEvents->AppendText(b.str() + "\n");
        //wxMessageBox(b.str(), wxT("reply msg"), wxICON_INFORMATION);

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

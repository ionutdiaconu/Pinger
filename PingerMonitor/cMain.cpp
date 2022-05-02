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

    cmdGetPing = new wxButton(this, BUTTON_GetPing, _T("Get ping"),  wxDefaultPosition, wxDefaultSize, 0); 
    
        
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
        size_t reply_length = boost::asio::read(s,boost::asio::buffer(reply, request_length));


        std::stringstream b;

        b.write(reply, reply_length);
       
        wxMessageBox(b.str(), wxT("reply msg"), wxICON_INFORMATION);

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

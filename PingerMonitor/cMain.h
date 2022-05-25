#pragma once

#include "wx/wx.h"

class cMain : public wxFrame
{

public:

	cMain();
	~cMain();

	wxButton* cmdGetPing;
	wxTextCtrl* txtPingEvents;
	wxTextCtrl* txtPingServer;

	void OnExit(wxCommandEvent& event);
	void OnGetPing(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()

};


enum
{
	BUTTON_GetPing = wxID_HIGHEST + 1, // declares an id which will be used to call our button
	TEXT_PingEvents,
	TEXT_PingServer
};


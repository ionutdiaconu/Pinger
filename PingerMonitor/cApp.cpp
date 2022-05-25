#include "cApp.h"

void wxIMPLEMENT_APP(cApp);


cApp::cApp()
{

}


cApp::~cApp() 
{
}

bool cApp::OnInit() 
{

	m_frame1 = new cMain();

	//wxSize frameSize = wxSize{10,10};
	m_frame1->SetMinSize(wxSize(1024, 800));
	m_frame1->Show();
	SetTopWindow(m_frame1); // and finally, set it as the main window
				
	return true;
}


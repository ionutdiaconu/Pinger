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
	m_frame1->Show();
	SetTopWindow(m_frame1); // and finally, set it as the main window
				
	return true;
}


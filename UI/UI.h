
// UI.h : main header file for the UI application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CUIApp:
// See UI.cpp for the implementation of this class
//

class CUIApp : public CWinApp
{
public:
	CUIApp() noexcept;


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CUIApp theApp;

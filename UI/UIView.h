// UIView.h : interface of the CUIView class
//

#pragma once
#include "UIDoc.h"

#define IDC_BTN_PLAY       5001
#define IDC_BTN_STEP       5002
#define IDC_BTN_RESET      5003
#define IDC_RAD_SPEED500   5004
#define IDC_RAD_SPEED1000  5005
#define IDC_RAD_SPEED2000  5006
#define IDC_LIST_LOGS      5007

class CUIView : public CView
{
protected: // create from serialization only
	CUIView() noexcept;
	DECLARE_DYNCREATE(CUIView)

// Attributes
public:
	CUIDoc* GetDocument() const;

	// UI Controls
	CButton m_btnPlay;
	CButton m_btnStep;
	CButton m_btnReset;
	CButton m_btnSpeed500;
	CButton m_btnSpeed1000;
	CButton m_btnSpeed2000;
	CListBox m_listLogs;

	// Custom Graphic Fonts
	CFont m_fontTitle;
	CFont m_fontLabel;
	CFont m_fontValue;
	CFont m_fontQueue;

// Operations
public:
	void UpdateControlsState();

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CUIView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnBtnPlay();
	afx_msg void OnBtnStep();
	afx_msg void OnBtnReset();
	afx_msg void OnRadSpeed500();
	afx_msg void OnRadSpeed1000();
	afx_msg void OnRadSpeed2000();
	afx_msg LRESULT OnUserSimUpdate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUserSimLog(WPARAM wParam, LPARAM lParam);
};

#ifndef _DEBUG  // debug version in UIView.cpp
inline CUIDoc* CUIView::GetDocument() const
   { return reinterpret_cast<CUIDoc*>(m_pDocument); }
#endif

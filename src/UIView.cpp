// UIView.cpp : implementation of the CUIView class
//

#include "pch.h"
#include "framework.h"
#include <cmath>

#ifndef SHARED_HANDLERS
#include "UI.h"
#endif

#include "UIDoc.h"
#include "UIView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CUIView

IMPLEMENT_DYNCREATE(CUIView, CView)

BEGIN_MESSAGE_MAP(CUIView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_BTN_PLAY, &CUIView::OnBtnPlay)
	ON_BN_CLICKED(IDC_BTN_STEP, &CUIView::OnBtnStep)
	ON_BN_CLICKED(IDC_BTN_RESET, &CUIView::OnBtnReset)
	ON_BN_CLICKED(IDC_RAD_SPEED500, &CUIView::OnRadSpeed500)
	ON_BN_CLICKED(IDC_RAD_SPEED1000, &CUIView::OnRadSpeed1000)
	ON_BN_CLICKED(IDC_RAD_SPEED2000, &CUIView::OnRadSpeed2000)
	ON_MESSAGE(WM_USER_SIM_UPDATE, &CUIView::OnUserSimUpdate)
	ON_MESSAGE(WM_USER_SIM_LOG, &CUIView::OnUserSimLog)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
END_MESSAGE_MAP()

// CUIView construction/destruction

CUIView::CUIView() noexcept
{
}

CUIView::~CUIView()
{
}

BOOL CUIView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

int CUIView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CUIDoc* pDoc = GetDocument();
	if (pDoc) {
		pDoc->SetViewWindow(m_hWnd);
	}

	// Create Child Controls in Sidebar
	CRect rectDummy(0, 0, 0, 0);
	m_btnPlay.Create(_T("자동 실행"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rectDummy, this, IDC_BTN_PLAY);
	m_btnStep.Create(_T("단계 실행"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rectDummy, this, IDC_BTN_STEP);
	m_btnReset.Create(_T("재설정 (Reset)"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rectDummy, this, IDC_BTN_RESET);

	m_btnSpeed500.Create(_T("0.5초 배속"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, rectDummy, this, IDC_RAD_SPEED500);
	m_btnSpeed1000.Create(_T("1.0초 배속"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, rectDummy, this, IDC_RAD_SPEED1000);
	m_btnSpeed2000.Create(_T("2.0초 배속"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, rectDummy, this, IDC_RAD_SPEED2000);

	m_btnSpeed1000.SetCheck(BST_CHECKED);

	m_listLogs.Create(WS_CHILD | WS_VISIBLE | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_BORDER, rectDummy, this, IDC_LIST_LOGS);

	// Create Elegant Vector Graphics Fonts (Anti-aliased, larger size)
	m_fontTitle.CreateFontW(34, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Malgun Gothic"));
	m_fontLabel.CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Malgun Gothic"));
	m_fontValue.CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Consolas"));
	m_fontQueue.CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Consolas"));

	m_listLogs.SetFont(&m_fontQueue);
	m_btnPlay.SetFont(&m_fontLabel);
	m_btnStep.SetFont(&m_fontLabel);
	m_btnReset.SetFont(&m_fontLabel);
	m_btnSpeed500.SetFont(&m_fontQueue);
	m_btnSpeed1000.SetFont(&m_fontQueue);
	m_btnSpeed2000.SetFont(&m_fontQueue);

	UpdateControlsState();
	return 0;
}

void CUIView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (cx < 100 || cy < 100) return;

	int sbLeft = cx - 310;

	// Position Sidebar controls
	if (m_btnPlay.GetSafeHwnd()) {
		m_btnPlay.MoveWindow(sbLeft, 20, 140, 38);
	}
	if (m_btnStep.GetSafeHwnd()) {
		m_btnStep.MoveWindow(sbLeft + 150, 20, 140, 38);
	}
	if (m_btnReset.GetSafeHwnd()) {
		m_btnReset.MoveWindow(sbLeft, 68, 290, 38);
	}

	if (m_btnSpeed500.GetSafeHwnd()) {
		m_btnSpeed500.MoveWindow(sbLeft, 118, 90, 25);
	}
	if (m_btnSpeed1000.GetSafeHwnd()) {
		m_btnSpeed1000.MoveWindow(sbLeft + 95, 118, 90, 25);
	}
	if (m_btnSpeed2000.GetSafeHwnd()) {
		m_btnSpeed2000.MoveWindow(sbLeft + 190, 118, 90, 25);
	}

	if (m_listLogs.GetSafeHwnd()) {
		m_listLogs.MoveWindow(sbLeft, 155, 290, cy - 175);
	}
}

BOOL CUIView::OnEraseBkgnd(CDC* /*pDC*/)
{
	// Suppress background clearing to completely eliminate flickering (double buffered)
	return TRUE;
}

// CUIView drawing

void CUIView::OnDraw(CDC* pDC)
{
	CUIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CRect rect;
	GetClientRect(&rect);

	// 더블 버퍼링 설정(가상 메모리 DC 및 비트맵 생성)
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);	// 가상 도화지 생성
	CBitmap memBitmap;
	memBitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());	// 가상 비트맵 연결
	CBitmap* pOldBitmap = memDC.SelectObject(&memBitmap);

	// 배경 및 타이틀 텍스트 렌더링
	CBrush bgBrush(RGB(241, 245, 249));
	memDC.FillRect(&rect, &bgBrush);

	int canvasWidth = rect.Width() - 320;
	if (canvasWidth < 300) canvasWidth = 300;

	int ringCx = canvasWidth / 2;
	int ringCy = (int)(rect.Height() * 0.38);
	int R = 140; // 외경 반지름
	int r = 70;  // 내경 반지름

	// 타이틀 렌더링
	memDC.SelectObject(&m_fontTitle);
	memDC.SetTextColor(RGB(15, 23, 42)); // Slate-900
	memDC.SetBkMode(TRANSPARENT);
	memDC.TextOutW(25, 20, _T("원형 버퍼 생산자-소비자 시각화"));

	memDC.SelectObject(&m_fontQueue);
	memDC.SetTextColor(RGB(37, 99, 235)); // Blue-600
	
	CString strSubtitle;
	strSubtitle.Format(_T("진행 단계: %d / %d | 생산 스레드: %d명 | 소비 스레드: %d명"), 
		pDoc->GetCurrentStepIndex(), pDoc->GetTotalSteps(), 
		pDoc->m_producerCount, pDoc->m_consumerCount);
	memDC.TextOutW(25, 52, strSubtitle);

	CBrush brushEmpty(RGB(226, 232, 240));   // Slate-200
	CBrush brushFilled(RGB(16, 185, 129));   // Emerald Green
	CPen penBorder(PS_SOLID, 2, RGB(148, 163, 184)); // Slate-400
	CPen penFilledBorder(PS_SOLID, 2, RGB(4, 120, 87)); // Emerald-700
	
	// 원형 버퍼의 4개 파이조각 그리기
	for (int i = 0; i < RING_BUFFER_SIZE; ++i) {
		bool isFilled = (strlen(pDoc->m_rb.buffer[i]) > 0);
		memDC.SelectObject(isFilled ? &brushFilled : &brushEmpty);		// 채워졌으면 녹색, 비었으면 회색
		memDC.SelectObject(isFilled ? &penFilledBorder : &penBorder);
		
		// Pie() 함수로 9도짜리 부채꼴 조각을 각각 우상, 우하, 좌하, 좌상에 그린다.
		if (i == 0) {
			memDC.Pie(ringCx - R, ringCy - R, ringCx + R, ringCy + R, ringCx + R, ringCy, ringCx, ringCy - R);
		}
		else if (i == 1) {
			memDC.Pie(ringCx - R, ringCy - R, ringCx + R, ringCy + R, ringCx, ringCy + R, ringCx + R, ringCy);
		}
		else if (i == 2) {
			memDC.Pie(ringCx - R, ringCy - R, ringCx + R, ringCy + R, ringCx - R, ringCy, ringCx, ringCy + R);
		}
		else if (i == 3) {
			memDC.Pie(ringCx - R, ringCy - R, ringCx + R, ringCy + R, ringCx, ringCy - R, ringCx - R, ringCy);
		}
	}
	
	// 가운데 구멍 뚫어 링 형태로 만들기
	memDC.SelectObject(&bgBrush);
	memDC.SelectObject(&penBorder);
	memDC.Ellipse(ringCx - r, ringCy - r, ringCx + r, ringCy + r);
	
	memDC.SelectObject(&m_fontLabel);
	memDC.SetTextColor(RGB(71, 85, 105)); // Slate-600
	CRect innerTextRect(ringCx - r, ringCy - 12, ringCx + r, ringCy + 12);
	memDC.DrawText(_T("BUFFER"), &innerTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	// 버퍼 이름 외각 라벨 그리기
	memDC.SelectObject(&m_fontLabel);
	memDC.SetTextColor(RGB(15, 23, 42));
	
	memDC.TextOutW(ringCx + R + 15, ringCy - 60, _T("A[0]"));
	memDC.TextOutW(ringCx + R + 15, ringCy + 35, _T("A[1]"));
	memDC.TextOutW(ringCx - R - 50, ringCy + 35, _T("A[2]"));
	memDC.TextOutW(ringCx - R - 50, ringCy - 60, _T("A[3]"));

	// 각 부채꼴 영역 정중앙에 데이터 텍스트 배치
	int offset = (int)((R + r) * 0.3535);  // 삼각법(cos 45도)을 통한 중앙 좌표 연산
	for (int i = 0; i < RING_BUFFER_SIZE; ++i) {
		CString strContent(pDoc->m_rb.buffer[i]);
		// 대각선 방향으로 tx, ty 좌표를 
		int tx = 0, ty = 0;
		if (i == 0) { tx = ringCx + offset; ty = ringCy - offset; }
		else if (i == 1) { tx = ringCx + offset; ty = ringCy + offset; }
		else if (i == 2) { tx = ringCx - offset; ty = ringCy + offset; }
		else if (i == 3) { tx = ringCx - offset; ty = ringCy - offset; }
		
		CRect txtRect(tx - 50, ty - 12, tx + 50, ty + 12);
		if (strContent.IsEmpty()) {
			memDC.SetTextColor(RGB(100, 116, 139)); // Slate-500
			memDC.DrawText(_T("Empty"), &txtRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		} else {
			memDC.SetTextColor(RGB(255, 255, 255)); // White inside filled green sector
			memDC.DrawText(strContent, &txtRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
	}

	// in과 out 인덱스를 표기할 계기판 상자 좌표 지정
	int pBoxY = ringCy - 35;
	CRect rectIn(ringCx + R + 80, pBoxY, ringCx + R + 175, pBoxY + 70);
	CRect rectOut(ringCx + R + 185, pBoxY, ringCx + R + 280, pBoxY + 70);
	
	CBrush brushInBox(RGB(239, 246, 255)); 
	CPen penInBox(PS_SOLID, 2, RGB(37, 99, 235)); 
	memDC.SelectObject(&brushInBox);
	memDC.SelectObject(&penInBox);
	memDC.RoundRect(&rectIn, CPoint(8, 8));
	
	memDC.SelectObject(&m_fontLabel);
	memDC.SetTextColor(RGB(37, 99, 235));
	CRect inLblRect(rectIn.left, rectIn.top + 8, rectIn.right, rectIn.top + 28);
	memDC.DrawText(_T("in"), &inLblRect, DT_CENTER | DT_SINGLELINE);
	
	memDC.SelectObject(&m_fontValue);
	memDC.SetTextColor(RGB(15, 23, 42));
	CString strIn; strIn.Format(_T("%d"), pDoc->m_rb.in);
	CRect inValRect(rectIn.left, rectIn.top + 30, rectIn.right, rectIn.bottom);
	memDC.DrawText(strIn, &inValRect, DT_CENTER | DT_SINGLELINE);

	CBrush brushOutBox(RGB(254, 242, 242)); // Softer light red fill
	CPen penOutBox(PS_SOLID, 2, RGB(220, 38, 38)); // Red-600 border
	memDC.SelectObject(&brushOutBox);
	memDC.SelectObject(&penOutBox);
	memDC.RoundRect(&rectOut, CPoint(8, 8));
	
	memDC.SelectObject(&m_fontLabel);
	memDC.SetTextColor(RGB(220, 38, 38));
	CRect outLblRect(rectOut.left, rectOut.top + 8, rectOut.right, rectOut.top + 28);
	memDC.DrawText(_T("out"), &outLblRect, DT_CENTER | DT_SINGLELINE);
	
	memDC.SelectObject(&m_fontValue);
	memDC.SetTextColor(RGB(15, 23, 42));
	CString strOut; strOut.Format(_T("%d"), pDoc->m_rb.out);
	CRect outValRect(rectOut.left, rectOut.top + 30, rectOut.right, rectOut.bottom);
	memDC.DrawText(strOut, &outValRect, DT_CENTER | DT_SINGLELINE);

	//  화살표를 그리는 도우미 함수 정의
	auto DrawArrow = [&](CDC* pDC, CPoint ptFrom, CPoint ptTo, COLORREF color) {
		CPen penArrow(PS_SOLID, 2, color);
		CBrush brushArrow(color);
		pDC->SelectObject(&penArrow);
		pDC->SelectObject(&brushArrow);
		
		pDC->MoveTo(ptFrom);
		pDC->LineTo(ptTo);
		
		// ant2를 활용한 화살표의 깃과 머리 삼각형 각도 계산
		double angle = atan2(ptTo.y - ptFrom.y, ptTo.x - ptFrom.x);
		int arrowSize = 8;
		CPoint p1(ptTo.x - (int)(arrowSize * cos(angle - 0.4)), ptTo.y - (int)(arrowSize * sin(angle - 0.4)));
		CPoint p2(ptTo.x - (int)(arrowSize * cos(angle + 0.4)), ptTo.y - (int)(arrowSize * sin(angle + 0.4)));
		
		CPoint arrowPts[3] = { ptTo, p1, p2 };
		pDC->Polygon(arrowPts, 3);
	};

	auto GetSectorCenter = [&](int index) -> CPoint {
		int offsetVal = (int)((R + r) * 0.3535);
		int sx = 0, sy = 0;
		if (index == 0) { sx = ringCx + offsetVal; sy = ringCy - offsetVal; }
		else if (index == 1) { sx = ringCx + offsetVal; sy = ringCy + offsetVal; }
		else if (index == 2) { sx = ringCx - offsetVal; sy = ringCy + offsetVal; }
		else if (index == 3) { sx = ringCx - offsetVal; sy = ringCy - offsetVal; }
		return CPoint(sx, sy);
	};

	// 현재 가리키는 버퍼 조각으로 화살표 표시
	DrawArrow(&memDC, CPoint(rectIn.left, rectIn.top + 35), GetSectorCenter(pDoc->m_rb.in), RGB(37, 99, 235));
	DrawArrow(&memDC, CPoint(rectOut.left, rectOut.top + 35), GetSectorCenter(pDoc->m_rb.out), RGB(220, 38, 38));

	// 세마포어, 동적 대기 큐 상자 구현
	int semY = (int)(rect.Height() * 0.65);
	CBrush semValBrush(RGB(255, 255, 255)); // Pure White fill for value box
	CBrush semQueueBrush(RGB(226, 232, 240)); // Slate-200 fill for queue box
	CPen semBorderPen(PS_SOLID, 1, RGB(148, 163, 184)); // Slate-400 border
	CPen semQueueBorderPen(PS_SOLID, 1, RGB(203, 213, 225)); // Slate-300 border
	
	auto DrawSemaphore = [&](CDC* pDC, CString label, int val, const std::vector<std::string>& waiting, CRect rectVal, CRect rectQueue, bool isLeftCol) {
		// 라벨 출력
		pDC->SelectObject(&m_fontLabel);
		pDC->SetTextColor(RGB(71, 85, 105)); // Slate-600
		pDC->TextOutW(rectVal.left, rectVal.top - 24, label);
		
		// 수치 박스
		pDC->SelectObject(&semValBrush);
		pDC->SelectObject(&semBorderPen);
		pDC->RoundRect(&rectVal, CPoint(6, 6));
		
		pDC->SelectObject(&m_fontValue);
		pDC->SetTextColor(RGB(15, 23, 42)); // Slate-900
		CString strVal; strVal.Format(_T("%d"), val);
		pDC->DrawText(strVal, &rectVal, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		
		// 대기열 박스
		pDC->SelectObject(&semQueueBrush);
		pDC->SelectObject(&semQueueBorderPen);
		pDC->RoundRect(&rectQueue, CPoint(6, 6));
		
		pDC->SelectObject(&m_fontQueue);
		if (waiting.empty()) {
			pDC->SetTextColor(RGB(100, 116, 139)); // Slate-500
			pDC->DrawText(_T("[대기 스레드 없음]"), &rectQueue, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		} else {
			pDC->SetTextColor(RGB(217, 119, 6)); // Amber-600
			CString strQueue;
			for (size_t k = 0; k < waiting.size(); ++k) {
				if (k > 0) strQueue += _T(", ");
				strQueue += CString(waiting[k].c_str());
			}
			strQueue += _T(" [Sleep]");
			pDC->DrawText(strQueue, &rectQueue, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
	};

	// 좌측 열 배치
	CRect mvP(ringCx - 120, semY, ringCx - 50, semY + 50);
	CRect mqP(ringCx - 370, semY, ringCx - 140, semY + 50);
	DrawSemaphore(&memDC, _T("mutexP"), pDoc->m_rb.mutexP.value, pDoc->m_rb.mutexP.waitingThreads, mvP, mqP, true);
	
	CRect mvF(ringCx - 120, semY + 70, ringCx - 50, semY + 120);
	CRect mqF(ringCx - 370, semY + 70, ringCx - 130, semY + 120);
	DrawSemaphore(&memDC, _T("nrfull"), pDoc->m_rb.nrfull.value, pDoc->m_rb.nrfull.waitingThreads, mvF, mqF, true);

	// 우측 열 배치
	CRect mvC(ringCx + 50, semY, ringCx + 120, semY + 50);
	CRect mqC(ringCx + 130, semY, ringCx + 370, semY + 50);
	DrawSemaphore(&memDC, _T("mutexC"), pDoc->m_rb.mutexC.value, pDoc->m_rb.mutexC.waitingThreads, mvC, mqC, false);
	
	CRect mvE(ringCx + 50, semY + 70, ringCx + 120, semY + 120);
	CRect mqE(ringCx + 130, semY + 70, ringCx + 370, semY + 120);
	DrawSemaphore(&memDC, _T("nrempty"), pDoc->m_rb.nrempty.value, pDoc->m_rb.nrempty.waitingThreads, mvE, mqE, false);

	// 하단 상태 메시지
	CRect bannerRect(25, rect.Height() - 40, canvasWidth - 25, rect.Height() - 15);
	memDC.SelectObject(&m_fontQueue);
	memDC.SetTextColor(RGB(71, 85, 105)); // Slate-600
	
	CString strStatusText;
	if (pDoc->IsAutoPlaying()) {
		strStatusText = _T("상태: 자동 재생 중... 시나리오의 흐름을 한눈에 확인할 수 있습니다.");
	} else if (pDoc->GetCurrentStepIndex() >= pDoc->GetTotalSteps()) {
		strStatusText = _T("상태: 시나리오 완료! [Reset]을 누르면 버퍼가 완전히 초기화됩니다.");
	} else {
		strStatusText = _T("상태: 일시 정지됨. [단계 실행] 또는 [자동 실행]을 누르십시오.");
	}
	memDC.DrawText(strStatusText, &bannerRect, DT_LEFT | DT_SINGLELINE);

	// Perform actual Screen Blit
	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
	
	memDC.SelectObject(pOldBitmap);
}


// CUIView printing

BOOL CUIView::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

void CUIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CUIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}


// CUIView diagnostics

#ifdef _DEBUG
void CUIView::AssertValid() const
{
	CView::AssertValid();
}

void CUIView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CUIDoc* CUIView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CUIDoc)));
	return (CUIDoc*)m_pDocument;
}
#endif //_DEBUG

// 자동 실행 버튼 클릭
void CUIView::OnBtnPlay()
{
	CUIDoc* pDoc = GetDocument();
	if (pDoc) {
		if (pDoc->IsAutoPlaying()) {
			pDoc->StopAutoPlay();
		} else {
			pDoc->StartAutoPlay();
		}
		UpdateControlsState();
	}
}

// 단계 실행 버튼 클릭
void CUIView::OnBtnStep()
{
	CUIDoc* pDoc = GetDocument();
	if (pDoc) {
		pDoc->ExecuteNextStep();
		UpdateControlsState();
	}
}

// 재설정(Reset) 버튼 클릭
void CUIView::OnBtnReset()
{
	CUIDoc* pDoc = GetDocument();
	if (pDoc) {
		pDoc->ResetSimulation();
		m_listLogs.ResetContent();
		UpdateControlsState();
	}
}

void CUIView::OnRadSpeed500()
{
	GetDocument()->SetAutoPlaySpeed(500);
}

void CUIView::OnRadSpeed1000()
{
	GetDocument()->SetAutoPlaySpeed(1000);
}

void CUIView::OnRadSpeed2000()
{
	GetDocument()->SetAutoPlaySpeed(2000);
}

LRESULT CUIView::OnUserSimUpdate(WPARAM wParam, LPARAM lParam)
{
	UpdateControlsState();
	Invalidate(FALSE); // redraw with double buffering (flicker-free)
	return 0;
}

LRESULT CUIView::OnUserSimLog(WPARAM wParam, LPARAM lParam)
{
	CUIDoc* pDoc = GetDocument();
	if (pDoc) {
		std::vector<std::string> logs = pDoc->GetLogs();
		int curCount = m_listLogs.GetCount();
		int logCount = (int)logs.size();
		if (logCount > curCount) {
			for (int i = curCount; i < logCount; ++i) {
				CString strLog(logs[i].c_str());
				m_listLogs.AddString(strLog);
			}
			// Auto scroll to the end
			m_listLogs.SetTopIndex(m_listLogs.GetCount() - 1);
		}
	}
	Invalidate(FALSE); // Redraw canvas as well
	return 0;
}

// 버튼 상태 최신화(자동 실행, 단계 실행 재설정 버튼)
void CUIView::UpdateControlsState()
{
	CUIDoc* pDoc = GetDocument();
	if (!pDoc) return;

	if (m_btnPlay.GetSafeHwnd()) {
		if (pDoc->IsAutoPlaying()) {
			m_btnPlay.SetWindowText(_T("일시 정지"));
		} else {
			m_btnPlay.SetWindowText(_T("자동 실행"));
		}
	}

	bool isScenarioEnd = (pDoc->GetCurrentStepIndex() >= pDoc->GetTotalSteps());
	bool isPlaying = pDoc->IsAutoPlaying();

	if (m_btnPlay.GetSafeHwnd()) {
		m_btnPlay.EnableWindow(!isScenarioEnd);
	}
	if (m_btnStep.GetSafeHwnd()) {
		m_btnStep.EnableWindow(!isScenarioEnd && !isPlaying);
	}
}


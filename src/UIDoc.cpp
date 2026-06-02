
// UIDoc.cpp : implementation of the CUIDoc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "UI.h"
#endif

#include "UIDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CUIDoc

IMPLEMENT_DYNCREATE(CUIDoc, CDocument)

BEGIN_MESSAGE_MAP(CUIDoc, CDocument)
END_MESSAGE_MAP()


// CUIDoc construction/destruction

CUIDoc::CUIDoc() noexcept
{
	m_currentStepIndex = 0;
	m_producerCount = 0;
	m_consumerCount = 0;
	m_isAutoPlaying = false;
	m_autoPlaySpeedMs = 1000;
	m_hWndView = NULL;
	m_terminateAutoPlay = false;
	m_terminateWorkers = false;

	// Scenario sequence matching main.cpp
	m_scenario = {
		"P", "C", "C", "C", "P", "P", "P", "P", "P",
		"P", "P", "P", "C", "P", "P", "P", "C", "C", "C",
		"C", "C", "C", "C", "C"
	};
}

CUIDoc::~CUIDoc()
{
	StopAutoPlay();
	for (auto& t : m_workers) {
		if (t.joinable()) {
			t.join();
		}
	}
}

BOOL CUIDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	ResetSimulation();
	return TRUE;
}

void CUIDoc::AddLog(const std::string& log)
{
	std::lock_guard<std::mutex> lock(m_logMutex);
	m_logs.push_back(log);
	if (m_hWndView && ::IsWindow(m_hWndView)) {
		::PostMessage(m_hWndView, WM_USER_SIM_LOG, 0, 0);
	}
}

std::vector<std::string> CUIDoc::GetLogs()
{
	std::lock_guard<std::mutex> lock(m_logMutex);
	return m_logs;
}

void CUIDoc::ClearLogs()
{
	std::lock_guard<std::mutex> lock(m_logMutex);
	m_logs.clear();
}

// 재설정 동작
void CUIDoc::ResetSimulation()
{
	StopAutoPlay();
	
	// Signal threads to terminate immediately
	m_terminateWorkers = true;

	// Force-wake any threads currently waiting in semaphore queues
	{
		std::lock_guard<std::mutex> lk(m_rb.nrfull.m);
		m_rb.nrfull.cv.notify_all();
	}
	{
		std::lock_guard<std::mutex> lk(m_rb.nrempty.m);
		m_rb.nrempty.cv.notify_all();
	}
	{
		std::lock_guard<std::mutex> lk(m_rb.mutexP.m);
		m_rb.mutexP.cv.notify_all();
	}
	{
		std::lock_guard<std::mutex> lk(m_rb.mutexC.m);
		m_rb.mutexC.cv.notify_all();
	}

	// Join all background worker threads safely and quickly
	for (auto& t : m_workers) {
		if (t.joinable()) {
			t.join();
		}
	}
	m_workers.clear();

	// Reset termination flag for future simulation steps
	m_terminateWorkers = false;

	m_rb.Reset();
	m_currentStepIndex = 0;
	m_producerCount = 0;
	m_consumerCount = 0;
	
	ClearLogs();
	AddLog("시뮬레이션 초기화 완료");

	if (m_hWndView && ::IsWindow(m_hWndView)) {
		::PostMessage(m_hWndView, WM_USER_SIM_UPDATE, 0, 0);
	}
}

// 다음 시나리오 실행 동작(자동 실행, 단계 실행방식 모두 사용)
void CUIDoc::ExecuteNextStep()
{
	if (m_currentStepIndex >= (int)m_scenario.size()) {
		AddLog("시나리오가 모두 종료되었습니다.");
		StopAutoPlay();
		return;
	}

	std::string action = m_scenario[m_currentStepIndex];
	m_currentStepIndex++;

	if (action == "P") {
		m_producerCount++;
		AddLog("생산자 스레드 P" + std::to_string(m_producerCount) + " 생성");
		m_workers.emplace_back(&CUIDoc::ProducerTask, this, m_producerCount);
	}
	else {
		m_consumerCount++;
		AddLog("소비자 스레드 C" + std::to_string(m_consumerCount) + " 생성");
		m_workers.emplace_back(&CUIDoc::ConsumerTask, this, m_consumerCount);
	}

	if (m_hWndView && ::IsWindow(m_hWndView)) {
		::PostMessage(m_hWndView, WM_USER_SIM_UPDATE, 0, 0);
	}
}

// 생산자 동작 
void CUIDoc::ProducerTask(int id)
{
	std::string name = "P" + std::to_string(id);
	
	AddLog(name + " 진입 대기 (nrempty.P 호출)");
	m_rb.nrempty.P(name, m_terminateWorkers);
	if (m_terminateWorkers) return;

	AddLog(name + " mutexP 대기 (mutexP.P 호출)");
	m_rb.mutexP.P(name, m_terminateWorkers);
	if (m_terminateWorkers) return;

	// Critical Section 진입
	std::string content = name + "번 생산";
	strncpy_s(m_rb.buffer[m_rb.in], STR_SIZE - 1, content.c_str(), _TRUNCATE);
	int slot = m_rb.in;
	m_rb.in = (m_rb.in + 1) % RING_BUFFER_SIZE;

	AddLog(name + " 버퍼 쓰기 완료: A[" + std::to_string(slot) + "] = " + content);

	// Critical Section 퇴장
	m_rb.mutexP.V(name);
	AddLog(name + " mutexP 해제 (mutexP.V 호출)");

	m_rb.nrfull.V(name);
	AddLog(name + " nrfull 알림 (nrfull.V 호출)");
}

// 소비자 동작
void CUIDoc::ConsumerTask(int id)
{
	std::string name = "C" + std::to_string(id);

	AddLog(name + " 소비 대기 (nrfull.P 호출)");
	m_rb.nrfull.P(name, m_terminateWorkers);
	if (m_terminateWorkers) return;

	AddLog(name + " mutexC 대기 (mutexC.P 호출)");
	m_rb.mutexC.P(name, m_terminateWorkers);
	if (m_terminateWorkers) return;

	// Critical Section 진입
	std::string content = m_rb.buffer[m_rb.out];
	int slot = m_rb.out;
	memset(m_rb.buffer[m_rb.out], 0, STR_SIZE);
	m_rb.out = (m_rb.out + 1) % RING_BUFFER_SIZE;

	AddLog(name + " 버퍼 읽기 완료: A[" + std::to_string(slot) + "] 확인 (" + content + ")");

	// Critical Section 퇴장
	m_rb.mutexC.V(name);
	AddLog(name + " mutexC 해제 (mutexC.V 호출)");

	m_rb.nrempty.V(name);
	AddLog(name + " nrempty 알림 (nrempty.V 호출)");
}

// 자동 실행 동작 시작 (AutoPlayLoop 실행 스레드 생성 -> ExecuteNextStep 반복)
void CUIDoc::StartAutoPlay()
{
	if (m_isAutoPlaying) return;

	m_isAutoPlaying = true;
	m_terminateAutoPlay = false;
	
	if (m_autoPlayThread.joinable()) {
		m_autoPlayThread.join();
	}

	AddLog("자동 실행을 시작합니다.");
	m_autoPlayThread = std::thread(&CUIDoc::AutoPlayLoop, this);
	
	if (m_hWndView && ::IsWindow(m_hWndView)) {
		::PostMessage(m_hWndView, WM_USER_SIM_UPDATE, 0, 0);
	}
}

// 자동 실행 동작 일시 정지
void CUIDoc::StopAutoPlay()
{
	if (!m_isAutoPlaying) return;

	{
		std::lock_guard<std::mutex> lock(m_autoPlayMutex);
		m_terminateAutoPlay = true;
		m_autoPlayCv.notify_all();
	}

	if (m_autoPlayThread.joinable()) {
		m_autoPlayThread.join();
	}

	m_isAutoPlaying = false;
	AddLog("자동 실행을 일시 정지합니다.");

	if (m_hWndView && ::IsWindow(m_hWndView)) {
		::PostMessage(m_hWndView, WM_USER_SIM_UPDATE, 0, 0);
	}
}

// 자동 실행 동작 구현
void CUIDoc::AutoPlayLoop()
{
	while (true) {
		{
			std::lock_guard<std::mutex> lock(m_autoPlayMutex);
			if (m_terminateAutoPlay) break;
		}

		if (m_currentStepIndex >= (int)m_scenario.size()) {
			m_isAutoPlaying = false;
			AddLog("시나리오가 종료되어 자동 실행이 멈춥니다.");
			if (m_hWndView && ::IsWindow(m_hWndView)) {
				::PostMessage(m_hWndView, WM_USER_SIM_UPDATE, 0, 0);
			}
			break;
		}

		ExecuteNextStep();

		std::unique_lock<std::mutex> lock(m_autoPlayMutex);
		m_autoPlayCv.wait_for(lock, std::chrono::milliseconds(m_autoPlaySpeedMs), [this]() {
			return m_terminateAutoPlay;
		});
		
		if (m_terminateAutoPlay) break;
	}
}



// CUIDoc serialization

void CUIDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CUIDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CUIDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CUIDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CUIDoc diagnostics

#ifdef _DEBUG
void CUIDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CUIDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CUIDoc commands


// UIDoc.h : interface of the CUIDoc class
//

#pragma once
#include "RingBuffer.h"
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

class CUIDoc : public CDocument
{
protected: // create from serialization only
	CUIDoc() noexcept;
	DECLARE_DYNCREATE(CUIDoc)

// Attributes
public:
	RingBuffer m_rb;
	std::vector<std::string> m_scenario;
	int m_currentStepIndex;
	int m_producerCount;
	int m_consumerCount;
	std::vector<std::thread> m_workers;
	std::atomic<bool> m_terminateWorkers; // Termination flag to wake up/exit blocked threads instantly
	std::mutex m_logMutex;
	std::vector<std::string> m_logs;
	
	// Auto play thread state
	bool m_isAutoPlaying;
	int m_autoPlaySpeedMs;
	HWND m_hWndView;
	std::thread m_autoPlayThread;
	bool m_terminateAutoPlay;
	std::mutex m_autoPlayMutex;
	std::condition_variable m_autoPlayCv;

// Operations
public:
	void AddLog(const std::string& log);
	std::vector<std::string> GetLogs();
	void ClearLogs();
	void ResetSimulation();
	void ExecuteNextStep();
	void StartAutoPlay();
	void StopAutoPlay();
	void ProducerTask(int id);
	void ConsumerTask(int id);
	void AutoPlayLoop();
	
	int GetCurrentStepIndex() const { return m_currentStepIndex; }
	int GetTotalSteps() const { return (int)m_scenario.size(); }
	bool IsAutoPlaying() const { return m_isAutoPlaying; }
	int GetAutoPlaySpeed() const { return m_autoPlaySpeedMs; }
	void SetAutoPlaySpeed(int ms) { m_autoPlaySpeedMs = ms; }
	void SetViewWindow(HWND hWnd) { m_hWndView = hWnd; m_rb.SetNotificationWindow(hWnd); }

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CUIDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};


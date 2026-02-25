#ifndef ANXIETY_PLUGIN_H
#define ANXIETY_PLUGIN_H

// Comment out CodeBlocks specific includes for testing
// #include <cbplugin.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/string.h>
#include <wx/defs.h>
#include <wx/evthandler.h>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

// Forward declarations
// class EventMonitor;
// class PythonBridge;
// class InterventionManager;
// class wxMenuBar;
// class wxMenu;
// class wxToolBar;

// Custom event IDs for menu items
enum {
    wxID_START_MONITORING = 1001,
    wxID_STOP_MONITORING = 1002,
    wxID_CONFIGURE = 1003,
    wxID_CALIBRATE = 1004,
    wxID_SHOW_STATS = 1005
};

// Simplified plugin class for testing
class AnxietyPlugin : public wxEvtHandler
{
    public:
        AnxietyPlugin();
        virtual ~AnxietyPlugin();

        void OnAttach();
        void OnRelease(bool appShutDown);
        
        void StartMonitoring();
        void StopMonitoring();
        bool IsMonitoring() const { return m_isMonitoring; }
        
        void BuildMenu(wxMenuBar* menuBar);
        bool BuildToolBar(wxToolBar* toolBar);

    private:
        void OnTimer(wxTimerEvent& event);
        void OnStartMonitoring(wxCommandEvent& event);
        void OnStopMonitoring(wxCommandEvent& event);
        void OnConfigure(wxCommandEvent& event);
        void OnCalibrate(wxCommandEvent& event);
        void OnShowStats(wxCommandEvent& event);
        
        std::unique_ptr<EventMonitor> m_eventMonitor;
        std::unique_ptr<PythonBridge> m_pythonBridge;
        std::unique_ptr<InterventionManager> m_interventionManager;
        
        wxTimer m_timer;
        std::atomic<bool> m_isMonitoring;
        
        wxString m_userDataPath;
        wxString m_pythonPath;
        
        DECLARE_EVENT_TABLE()
};

#endif
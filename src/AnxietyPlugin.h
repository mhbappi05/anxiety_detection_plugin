#ifndef ANXIETY_PLUGIN_H
#define ANXIETY_PLUGIN_H

#include <cbplugin.h>   // from sdk/ stub
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/string.h>
#include <wx/defs.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

// Forward declarations
class EventMonitor;
class PythonBridge;
class InterventionManager;

// Custom event IDs for menu items
enum {
    wxID_START_MONITORING = 1001,
    wxID_STOP_MONITORING = 1002,
    wxID_CONFIGURE = 1003,
    wxID_CALIBRATE = 1004,
    wxID_SHOW_STATS = 1005
};

class AnxietyPlugin : public cbPlugin
{
    public:
        AnxietyPlugin();
        virtual ~AnxietyPlugin();

        virtual void OnAttach() override;
        virtual void OnRelease(bool appShutDown) override;

        void StartMonitoring();
        void StopMonitoring();
        bool IsMonitoring() const { return m_isMonitoring; }

        virtual void BuildMenu(wxMenuBar* menuBar) override;
        virtual bool BuildToolBar(wxToolBar* toolBar) override;

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
#include "AnxietyPlugin.h"
#include "EventMonitor.h"
#include "PythonBridge.h"
#include "InterventionManager.h"
// #include <cbproject.h>
// #include <manager.h>
// #include <logmanager.h>
// #include <configmanager.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/statline.h>
#include <wx/log.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/toolbar.h>
#include <wx/bitmap.h>
#include <wx/event.h>
#include <wx/datetime.h>
#include <wx/dcmemory.h>

// Register the plugin
// namespace
// {
//     PluginRegistrant<AnxietyPlugin> reg(_T("AnxietyPlugin"));
// }

wxBEGIN_EVENT_TABLE(AnxietyPlugin, wxEvtHandler)
    EVT_TIMER(wxID_ANY, AnxietyPlugin::OnTimer)
    EVT_MENU(wxID_START_MONITORING, AnxietyPlugin::OnStartMonitoring)
    EVT_MENU(wxID_STOP_MONITORING, AnxietyPlugin::OnStopMonitoring)
    EVT_MENU(wxID_CONFIGURE, AnxietyPlugin::OnConfigure)
    EVT_MENU(wxID_CALIBRATE, AnxietyPlugin::OnCalibrate)
    EVT_MENU(wxID_SHOW_STATS, AnxietyPlugin::OnShowStats)
wxEND_EVENT_TABLE()

AnxietyPlugin::AnxietyPlugin()
    : m_isMonitoring(false)
{
    // Initialize paths (using simple defaults since Code::Blocks SDK is not available)
    // m_userDataPath = cfg->Read(_T("data_path"), wxGetCwd() + wxFILE_SEP_PATH + _T("anxiety_data"));
    // m_pythonPath = cfg->Read(_T("python_path"), _T("python"));
    m_userDataPath = wxGetCwd() + wxFILE_SEP_PATH + _T("anxiety_data");
    m_pythonPath = _T("python");
    
    // Create data directory if it doesn't exist
    if (!wxDirExists(m_userDataPath))
        wxMkdir(m_userDataPath);
}

AnxietyPlugin::~AnxietyPlugin()
{
    OnRelease(false);
}

void AnxietyPlugin::OnAttach()
{
    // Create components
    m_eventMonitor = std::make_unique<EventMonitor>();
    m_pythonBridge = std::make_unique<PythonBridge>();
    m_interventionManager = std::make_unique<InterventionManager>();
    
    // Set up timer (check every 5 seconds)
    m_timer.SetOwner(this);
    m_timer.Start(5000);
    
    // Start Python service
    wxString modelPath = m_userDataPath + wxFILE_SEP_PATH + _T("best_anxiety_model.pkl");
    if (!m_pythonBridge->StartPythonService(m_pythonPath, modelPath))
    {
        // Manager::Get()->GetLogManager()->LogError(_T("Failed to start Python anxiety detection service"));
        wxLogError(_T("Failed to start Python anxiety detection service"));
    }
    
    // Set callback for predictions
    m_pythonBridge->SetCallback([this](const PredictionResult& result) {
        std::vector<wxString> features;
        features.push_back(result.triggeredFeatures);
        
        if (m_interventionManager->ShouldIntervene(result.level, result.confidence, features))
        {
            // Generate appropriate intervention
            wxString message = wxString::Format(
                _T("High anxiety detected (confidence: %.1f%%)\nTriggered by: %s"),
                result.confidence * 100,
                result.triggeredFeatures
            );
            
            wxString hint = InterventionManager::GetHintForError(result.triggeredFeatures);
            m_interventionManager->ShowIntervention(
                _T("Anxiety Detected"),
                message,
                InterventionType::ERROR_HINT,
                hint
            );
        }
    });
}

void AnxietyPlugin::OnRelease(bool appShutDown)
{
    m_timer.Stop();
    m_isMonitoring = false;
    
    if (m_pythonBridge)
        m_pythonBridge->StopPythonService();
    
    if (m_eventMonitor)
        m_eventMonitor->StopMonitoring();
}

void AnxietyPlugin::BuildMenu(wxMenuBar* menuBar)
{
    // Find or create Tools menu
    int toolsPos = menuBar->FindMenu(_T("Tools"));
    wxMenu* toolsMenu;
    
    if (toolsPos == wxNOT_FOUND)
    {
        toolsMenu = new wxMenu();
        menuBar->Append(toolsMenu, _T("&Tools"));
    }
    else
    {
        toolsMenu = menuBar->GetMenu(toolsPos);
    }
    
    // Add our menu items
    wxMenu* anxietyMenu = new wxMenu();
    anxietyMenu->Append(wxID_START_MONITORING, _T("Start Anxiety Monitoring"));
    anxietyMenu->Append(wxID_STOP_MONITORING, _T("Stop Anxiety Monitoring"));
    anxietyMenu->AppendSeparator();
    anxietyMenu->Append(wxID_CALIBRATE, _T("Calibrate Baseline"));
    anxietyMenu->Append(wxID_CONFIGURE, _T("Configure"));
    anxietyMenu->Append(wxID_SHOW_STATS, _T("Show Statistics"));
    
    toolsMenu->AppendSubMenu(anxietyMenu, _T("&Anxiety Detection"));
}

bool AnxietyPlugin::BuildToolBar(wxToolBar* toolBar)
{
    // Add toolbar button
    wxBitmap bitmap(16, 16); // You'll need actual icons
    toolBar->AddTool(wxID_START_MONITORING, _T("Start Anxiety Detection"), 
                     bitmap, _T("Start Monitoring Anxiety"));
    toolBar->AddTool(wxID_STOP_MONITORING, _T("Stop Anxiety Detection"), 
                     bitmap, _T("Stop Monitoring"));
    return true;
}

void AnxietyPlugin::OnTimer(wxTimerEvent& event)
{
    if (!m_isMonitoring || !m_eventMonitor)
        return;
    
    // Extract features from recent events
    auto features = m_eventMonitor->ExtractFeatures();
    
    // Send to Python for analysis
    if (m_pythonBridge && m_pythonBridge->IsRunning())
    {
        m_pythonBridge->SendFeatures(features);
    }
}

void AnxietyPlugin::OnStartMonitoring(wxCommandEvent& event)
{
    if (!m_isMonitoring)
    {
        m_eventMonitor->StartMonitoring();
        m_isMonitoring = true;
        wxLogMessage(_T("Anxiety monitoring started"));
    }
}

void AnxietyPlugin::OnStopMonitoring(wxCommandEvent& event)
{
    if (m_isMonitoring)
    {
        m_eventMonitor->StopMonitoring();
        m_isMonitoring = false;
        wxLogMessage(_T("Anxiety monitoring stopped"));
        
        // Save session data
        wxString filename = m_userDataPath + wxFILE_SEP_PATH + 
                           wxDateTime::Now().Format(_T("session_%Y%m%d_%H%M%S.csv"));
        m_eventMonitor->SaveSessionData(filename);
    }
}

void AnxietyPlugin::OnConfigure(wxCommandEvent& event)
{
    // Create configuration dialog
    class ConfigDialog : public wxDialog
    {
    public:
        ConfigDialog(wxWindow* parent) 
            : wxDialog(parent, wxID_ANY, _T("Anxiety Detection Configuration"))
        {
            wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
            
            // Python path
            wxBoxSizer* pythonSizer = new wxBoxSizer(wxHORIZONTAL);
            pythonSizer->Add(new wxStaticText(this, wxID_ANY, _T("Python Path:")), 
                            0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            m_pythonPath = new wxTextCtrl(this, wxID_ANY, wxEmptyString, 
                                          wxDefaultPosition, wxSize(300, -1));
            pythonSizer->Add(m_pythonPath, 1, wxALL | wxEXPAND, 5);
            pythonSizer->Add(new wxButton(this, wxID_ANY, _T("Browse...")), 
                            0, wxALL, 5);
            mainSizer->Add(pythonSizer, 0, wxEXPAND);
            
            // Threshold
            wxBoxSizer* thresholdSizer = new wxBoxSizer(wxHORIZONTAL);
            thresholdSizer->Add(new wxStaticText(this, wxID_ANY, _T("Anxiety Threshold:")), 
                               0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            m_threshold = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, 
                                               wxDefaultPosition, wxSize(100, -1));
            m_threshold->SetRange(0.0, 1.0);
            m_threshold->SetValue(0.7);
            m_threshold->SetIncrement(0.05);
            thresholdSizer->Add(m_threshold, 0, wxALL, 5);
            mainSizer->Add(thresholdSizer, 0, wxEXPAND);
            
            // Cooldown
            wxBoxSizer* cooldownSizer = new wxBoxSizer(wxHORIZONTAL);
            cooldownSizer->Add(new wxStaticText(this, wxID_ANY, _T("Intervention Cooldown (minutes):")), 
                               0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            m_cooldown = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, 
                                        wxDefaultPosition, wxSize(100, -1));
            m_cooldown->SetRange(1, 30);
            m_cooldown->SetValue(5);
            cooldownSizer->Add(m_cooldown, 0, wxALL, 5);
            mainSizer->Add(cooldownSizer, 0, wxEXPAND);
            
            // Buttons
            wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
            buttonSizer->Add(new wxButton(this, wxID_OK, _T("OK")), 0, wxALL, 5);
            buttonSizer->Add(new wxButton(this, wxID_CANCEL, _T("Cancel")), 0, wxALL, 5);
            mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER);
            
            SetSizerAndFit(mainSizer);
        }
        
        wxTextCtrl* m_pythonPath;
        wxSpinCtrlDouble* m_threshold;
        wxSpinCtrl* m_cooldown;
    };
    
    ConfigDialog dlg(nullptr);
    if (dlg.ShowModal() == wxID_OK)
    {
        // Apply settings (Config::Blocks SDK not available, so just apply in memory)
        // ConfigManager* cfg = Manager::Get()->GetConfigManager(_T("anxiety_plugin"));
        // cfg->Write(_T("python_path"), dlg.m_pythonPath->GetValue());
        // cfg->Write(_T("threshold"), dlg.m_threshold->GetValue());
        // cfg->Write(_T("cooldown"), dlg.m_cooldown->GetValue());
        
        // Apply settings
        m_pythonPath = dlg.m_pythonPath->GetValue();
        m_interventionManager->SetAnxietyThreshold(dlg.m_threshold->GetValue());
        m_interventionManager->SetCooldownPeriod(dlg.m_cooldown->GetValue());
    }
}

void AnxietyPlugin::OnCalibrate(wxCommandEvent& event)
{
    wxMessageDialog dlg(nullptr,
        _T("Calibration will monitor your normal typing pattern for 5 minutes.\n")
        _T("Please code normally during this time.\n\n")
        _T("Do you want to start calibration?"),
        _T("Calibration"),
        wxYES_NO | wxICON_QUESTION);
    
    if (dlg.ShowModal() == wxID_YES)
    {
        m_eventMonitor->ResetSession();
        m_eventMonitor->StartMonitoring();
        
        wxMessageBox(_T("Calibration started. Code normally for 5 minutes."),
                     _T("Calibration"), wxOK | wxICON_INFORMATION);
        
        // Note: Full 5-minute calibration timer would require a dedicated timer event handler
        // For now, users can manually stop monitoring to end calibration
        wxLogMessage(_T("Calibration in progress. Stop monitoring to complete calibration."));
    }
}

void AnxietyPlugin::OnShowStats(wxCommandEvent& event)
{
    if (!m_eventMonitor)
        return;
    
    auto session = m_eventMonitor->GetCurrentSession();
    auto features = m_eventMonitor->ExtractFeatures();
    
    wxString stats;
    stats.Printf(_T(
        "Current Session Statistics:\n\n"
        "Duration: %ld minutes\n"
        "Total Keystrokes: %d\n"
        "Backspaces: %d (%.1f%%)\n"
        "Compilations: %d\n"
        "Failed Compilations: %d (%.1f%%)\n"
        "RED Metric: %.2f\n"
        "Typing Velocity: %.1f WPM\n"
        "Keystroke Variance: %.3f\n\n"
        "Anxiety Indicators:\n"
        "RED > 2.5: %s\n"
        "Velocity Drop > 35%%: %s\n"
        "High Backspace Rate: %s\n"
        "Irregular Rhythm: %s\n"
    ),
        (wxDateTime::Now() - session.sessionStart).GetMinutes().GetValue(),
        session.totalKeystrokes,
        session.totalBackspaces,
        session.totalKeystrokes > 0 ? (100.0 * session.totalBackspaces / session.totalKeystrokes) : 0,
        session.totalCompiles,
        session.failedCompiles,
        session.totalCompiles > 0 ? (100.0 * session.failedCompiles / session.totalCompiles) : 0,
        features[3], // RED Metric
        features[0] * 60, // Typing Velocity in WPM
        features[1], // Variance
        features[3] > 2.5 ? _T("YES") : _T("no"),
        features[0] < 0.65 ? _T("YES") : _T("no"), // Assuming baseline is 1.0
        features[2] > 0.3 ? _T("YES") : _T("no"),
        features[1] > 0.5 ? _T("YES") : _T("no")
    );
    
    wxMessageBox(stats, _T("Anxiety Statistics"), wxOK | wxICON_INFORMATION);
}
#include "InterventionManager.h"
#include <wx/msgdlg.h>
#include <wx/statline.h>
#include <wx/sound.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/xml/xml.h>
#include <wx/notifmsg.h>
#include <wx/panel.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <random>

// Define custom events
wxDEFINE_EVENT(wxEVT_INTERVENTION_ACTION, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_INTERVENTION_CLOSE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FEEDBACK_SUBMIT, wxCommandEvent);

wxBEGIN_EVENT_TABLE(InterventionManager, wxEvtHandler)
    EVT_BUTTON(wxID_ANY, InterventionManager::OnInterventionButton)
    EVT_TIMER(wxID_ANY, InterventionManager::OnInterventionTimer)
wxEND_EVENT_TABLE()

InterventionManager::InterventionManager()
    : m_parentFrame(nullptr)
    , m_onCooldown(false)
    , m_cooldownSeconds(300)  // 5 minutes default
    , m_anxietyThreshold(0.7f)
    , m_enableC(true)
    , m_enableCpp(true)
    , m_showNotifications(true)
    , m_playSounds(false)
    , m_interventionCounter(0)
    , m_notificationPanel(nullptr)
    , m_titleText(nullptr)
    , m_messageText(nullptr)
    , m_hintText(nullptr)
    , m_acceptButton(nullptr)
    , m_dismissButton(nullptr)
    , m_feedbackButton(nullptr)
    , m_notificationSizer(nullptr)
{
    m_autoHideTimer.SetOwner(this);
    
    // Initialize default messages
    m_relaxationMessages = {
        _T("Take a deep breath. The solution is often simpler than it seems."),
        _T("Consider taking a 2-minute break to clear your mind."),
        _T("Try breaking the problem down into smaller parts."),
        _T("Sometimes walking away for a moment helps. Why not stretch?"),
        _T("You've solved harder problems before. You can do this!"),
        _T("Take a moment to review your logic step by step."),
        _T("Remember: every expert was once a beginner."),
        _T("Try explaining the problem to someone (or to a rubber duck)."),
        _T("Your brain needs rest. A short break will help."),
        _T("Progress, not perfection. You're getting there!")
    };
    
    m_encouragementMessages = {
        _T("You're making great progress!"),
        _T("Keep up the good work!"),
        _T("Every error is a learning opportunity."),
        _T("You've got this!"),
        _T("Persistence pays off!")
    };
    
    m_successMessages = {
        _T("Great job fixing that error!"),
        _T("You're making excellent progress!"),
        _T("Keep up the good work!"),
        _T("Another problem solved!"),
        _T("Well done! That error won't stop you!")
    };
    
    // Initialize error hints
    m_errorHints = {
        {_T("syntax error"), _T("Check for missing semicolons, brackets, or parentheses")},
        {_T("missing semicolon"), _T("You might be missing a semicolon at the end of a statement")},
        {_T("undefined reference"), _T("You might be missing a header file or library link")},
        {_T("missing header"), _T("Check if you've included the necessary header files")},
        {_T("segmentation fault"), _T("Check for null pointers or array bounds")},
        {_T("null pointer"), _T("Make sure to initialize pointers before using them")},
        {_T("array bounds"), _T("Ensure array indices are within bounds")},
        {_T("uninitialized"), _T("Initialize variables before using them")},
        {_T("memory leak"), _T("Remember to free allocated memory")},
        {_T("buffer overflow"), _T("Check array sizes and string lengths")},
        {_T("type mismatch"), _T("Ensure types are compatible")},
        {_T("no matching function"), _T("Check function parameters and overloads")},
        {_T("ambiguous"), _T("Make the call more specific")},
        {_T("redefinition"), _T("Remove duplicate declarations")},
        {_T("undeclared"), _T("Declare variables before using them")},
        {_T("incomplete type"), _T("Include the full type definition")}
    };
}

InterventionManager::~InterventionManager()
{
    m_autoHideTimer.Stop();
    HideNotification();
    SaveConfiguration();
}

void InterventionManager::Initialize(wxFrame* parentFrame)
{
    m_parentFrame = parentFrame;
    CreateNotificationWindow();
    
    // Load configuration
    wxString configDir = wxStandardPaths::Get().GetUserConfigDir();
    m_configPath = configDir + wxFILE_SEP_PATH + _T("CodeBlocks") + 
                   wxFILE_SEP_PATH + _T("anxiety_plugin_config.xml");
    m_userDataPath = configDir + wxFILE_SEP_PATH + _T("CodeBlocks") + 
                     wxFILE_SEP_PATH + _T("anxiety_data");
    
    if (!wxDirExists(m_userDataPath))
        wxMkdir(m_userDataPath);
    
    LoadConfiguration(m_configPath);
}

void InterventionManager::LoadConfiguration(const wxString& configPath)
{
    if (!wxFileExists(configPath))
        return;
    
    wxXmlDocument doc;
    if (!doc.Load(configPath))
        return;
    
    wxXmlNode* root = doc.GetRoot();
    if (root->GetName() != _T("AnxietyPlugin"))
        return;
    
    wxXmlNode* settings = root->GetChildren();
    while (settings)
    {
        if (settings->GetName() == _T("Settings"))
        {
            wxXmlNode* param = settings->GetChildren();
            while (param)
            {
                if (param->GetName() == _T("AnxietyThreshold"))
                {
                    double val;
                    if (param->GetNodeContent().ToDouble(&val))
                        m_anxietyThreshold = static_cast<float>(val);
                }
                else if (param->GetName() == _T("InterventionCooldown"))
                {
                    m_cooldownSeconds = wxAtoi(param->GetNodeContent());
                }
                else if (param->GetName() == _T("EnableC"))
                {
                    m_enableC = param->GetNodeContent().Lower() == _T("true");
                }
                else if (param->GetName() == _T("EnableCpp"))
                {
                    m_enableCpp = param->GetNodeContent().Lower() == _T("true");
                }
                else if (param->GetName() == _T("ShowNotifications"))
                {
                    m_showNotifications = param->GetNodeContent().Lower() == _T("true");
                }
                else if (param->GetName() == _T("PlaySounds"))
                {
                    m_playSounds = param->GetNodeContent().Lower() == _T("true");
                }
                param = param->GetNext();
            }
        }
        else if (settings->GetName() == _T("ErrorHints"))
        {
            LoadErrorHints(configPath);
        }
        else if (settings->GetName() == _T("RelaxationMessages"))
        {
            LoadRelaxationMessages(configPath);
        }
        else if (settings->GetName() == _T("SuccessMessages"))
        {
            LoadSuccessMessages(configPath);
        }
        settings = settings->GetNext();
    }
}

void InterventionManager::LoadErrorHints(const wxString& configPath)
{
    wxXmlDocument doc;
    if (!doc.Load(configPath))
        return;
    
    wxXmlNode* root = doc.GetRoot();
    wxXmlNode* hints = root->GetChildren();
    
    while (hints)
    {
        if (hints->GetName() == _T("ErrorHints"))
        {
            wxXmlNode* hint = hints->GetChildren();
            while (hint)
            {
                if (hint->GetName() == _T("Hint"))
                {
                    wxString error = hint->GetAttribute(_T("error"), _T(""));
                    wxString message = hint->GetNodeContent();
                    if (!error.IsEmpty() && !message.IsEmpty())
                        m_errorHints[error] = message;
                }
                hint = hint->GetNext();
            }
        }
        hints = hints->GetNext();
    }
}

void InterventionManager::LoadRelaxationMessages(const wxString& configPath)
{
    wxXmlDocument doc;
    if (!doc.Load(configPath))
        return;
    
    wxXmlNode* root = doc.GetRoot();
    wxXmlNode* messages = root->GetChildren();
    
    while (messages)
    {
        if (messages->GetName() == _T("RelaxationMessages"))
        {
            m_relaxationMessages.clear();
            wxXmlNode* msg = messages->GetChildren();
            while (msg)
            {
                if (msg->GetName() == _T("Message"))
                    m_relaxationMessages.push_back(msg->GetNodeContent());
                msg = msg->GetNext();
            }
        }
        messages = messages->GetNext();
    }
}

void InterventionManager::LoadSuccessMessages(const wxString& configPath)
{
    wxXmlDocument doc;
    if (!doc.Load(configPath))
        return;
    
    wxXmlNode* root = doc.GetRoot();
    wxXmlNode* messages = root->GetChildren();
    
    while (messages)
    {
        if (messages->GetName() == _T("SuccessMessages"))
        {
            m_successMessages.clear();
            wxXmlNode* msg = messages->GetChildren();
            while (msg)
            {
                if (msg->GetName() == _T("Message"))
                    m_successMessages.push_back(msg->GetNodeContent());
                msg = msg->GetNext();
            }
        }
        messages = messages->GetNext();
    }
}

void InterventionManager::SaveConfiguration()
{
    // Save user feedback and intervention history
    wxString historyFile = m_userDataPath + wxFILE_SEP_PATH + _T("intervention_history.json");
    // TODO: Save history to JSON
}

bool InterventionManager::IsLanguageEnabled(const wxString& language) const
{
    wxString lang = language.Lower();
    if (lang == _T("c") || lang == _T("c language"))
        return m_enableC;
    if (lang == _T("c++") || lang == _T("cpp") || lang == _T("c plus plus"))
        return m_enableCpp;
    return false;
}

bool InterventionManager::IsOnCooldown() const
{
    if (!m_onCooldown)
        return false;
    
    wxTimeSpan elapsed = wxDateTime::Now() - m_lastIntervention;
    if (elapsed.GetSeconds() > m_cooldownSeconds)
    {
        const_cast<InterventionManager*>(this)->m_onCooldown = false;
        return false;
    }
    return true;
}

void InterventionManager::StartCooldown()
{
    m_lastIntervention = wxDateTime::Now();
    m_onCooldown = true;
}

void InterventionManager::ResetCooldown()
{
    m_onCooldown = false;
}

bool InterventionManager::ShouldIntervene(AnxietyLevel level, double confidence,
                                          const std::vector<wxString>& triggeredFeatures)
{
    // Check cooldown
    if (IsOnCooldown())
        return false;
    
    // Check threshold
    if (confidence < m_anxietyThreshold)
        return false;
    
    // Check anxiety level
    bool highAnxiety = (level == AnxietyLevel::HIGH || level == AnxietyLevel::EXTREME);
    if (!highAnxiety)
        return false;
    
    // Don't intervene if too few keystrokes (avoid false positives on idle)
    if (triggeredFeatures.empty())
        return false;
    
    return true;
}

void InterventionManager::ShowIntervention(const wxString& title, const wxString& message,
                                           InterventionType type,
                                           const wxString& hint,
                                           const wxString& errorType)
{
    if (!m_showNotifications)
        return;
    
    // Generate intervention ID
    wxString id = GenerateInterventionId();
    m_currentInterventionId = id;
    
    // Create intervention record
    Intervention intervention;
    intervention.id = id;
    intervention.timestamp = wxDateTime::Now();
    intervention.anxietyLevel = AnxietyLevel::HIGH;
    intervention.type = type;
    intervention.severity = InterventionSeverity::WARNING;
    intervention.title = title;
    intervention.message = message;
    intervention.hint = hint;
    intervention.errorType = errorType;
    intervention.accepted = false;
    intervention.dismissed = false;
    intervention.confidence = 0.8; // This would come from the model
    
    SaveInterventionToHistory(intervention);
    
    // Show notification
    std::vector<wxString> options;
    if (type == InterventionType::ERROR_HINT && !hint.IsEmpty())
    {
        options.push_back(_T("Show Hint"));
        options.push_back(_T("Dismiss"));
    }
    else if (type == InterventionType::BREAK_SUGGESTION)
    {
        options.push_back(_T("Take Break"));
        options.push_back(_T("Continue"));
    }
    else
    {
        options.push_back(_T("OK"));
    }
    
    ShowNotification(title, message, hint, options);
    
    // Start cooldown
    StartCooldown();
    
    // Play sound if enabled
    if (m_playSounds)
        PlayNotificationSound();
    
    LogIntervention(message, AnxietyLevel::HIGH);
}

void InterventionManager::ShowErrorHint(const wxString& errorType, const wxString& errorMessage)
{
    wxString hint = GetHintForError(errorType);
    if (hint.IsEmpty())
        hint = GetHintForError(_T("general"));
    
    wxString title = _T("🔍 Stuck on an error?");
    wxString message = wxString::Format(_T("You've encountered: %s\n\n%s"), 
                                         errorType, hint);
    
    ShowIntervention(title, message, InterventionType::ERROR_HINT, hint, errorType);
}

void InterventionManager::ShowBreakSuggestion()
{
    wxString title = _T("😌 Time for a short break?");
    wxString message = GetRelaxationSuggestion();
    
    ShowIntervention(title, message, InterventionType::BREAK_SUGGESTION);
}

void InterventionManager::ShowEncouragement()
{
    wxString title = _T("💪 You're doing great!");
    wxString message = GetRandomEncouragement();
    
    ShowIntervention(title, message, InterventionType::ENCOURAGEMENT);
}

void InterventionManager::ShowSuccessMessage()
{
    if (m_successMessages.empty())
        return;
    
    // Random success message
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, m_successMessages.size() - 1);
    wxString message = m_successMessages[dis(gen)];
    
    wxString title = _T("✅ Success!");
    
    ShowIntervention(title, message, InterventionType::SUCCESS_CELEBRATION);
    
    if (m_playSounds)
        PlaySuccessSound();
}

void InterventionManager::ShowCalibrationDialog()
{
    InterventionDialog dlg(m_parentFrame, 
        _T("Anxiety Detection Calibration"),
        _T("Calibration will monitor your normal typing pattern for a few minutes.\n\n")
        _T("Please code normally during this time."),
        wxEmptyString,
        InterventionType::CALIBRATION_REQUEST);
    
    if (dlg.ShowModal() == wxID_OK && dlg.WasAccepted())
    {
        // Start calibration
        LogIntervention(_T("Calibration started"));
    }
}

void InterventionManager::ShowStatisticsDialog(const wxString& stats)
{
    wxString title = _T("Anxiety Detection Statistics");
    wxString message = stats;
    
    ShowNotification(title, message, wxEmptyString, {_T("OK")});
}

void InterventionManager::CreateNotificationWindow()
{
    if (!m_parentFrame || m_notificationPanel)
        return;
    
    // Create a hidden panel that will appear when needed
    m_notificationPanel = new wxPanel(m_parentFrame, wxID_ANY, 
                                       wxDefaultPosition, 
                                       wxSize(400, 150),
                                       wxBORDER_SIMPLE | wxSTAY_ON_TOP);
    
    m_notificationPanel->SetBackgroundColour(wxColour(255, 255, 225)); // Light yellow
    
    m_notificationSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    m_titleText = new wxStaticText(m_notificationPanel, wxID_ANY, wxEmptyString);
    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    m_titleText->SetFont(titleFont);
    m_notificationSizer->Add(m_titleText, 0, wxALL | wxEXPAND, 10);
    
    // Message
    m_messageText = new wxStaticText(m_notificationPanel, wxID_ANY, wxEmptyString);
    m_messageText->Wrap(380);
    m_notificationSizer->Add(m_messageText, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);
    
    // Hint (optional)
    m_hintText = new wxStaticText(m_notificationPanel, wxID_ANY, wxEmptyString);
    m_hintText->SetForegroundColour(wxColour(0, 100, 0)); // Dark green
    m_hintText->Wrap(380);
    m_notificationSizer->Add(m_hintText, 0, wxALL | wxEXPAND, 10);
    
    m_notificationSizer->Add(new wxStaticLine(m_notificationPanel), 0, wxEXPAND | wxALL, 5);
    
    // Button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_acceptButton = new wxButton(m_notificationPanel, wxID_ANY, _T("Show Hint"));
    m_dismissButton = new wxButton(m_notificationPanel, wxID_ANY, _T("Dismiss"));
    m_feedbackButton = new wxButton(m_notificationPanel, wxID_ANY, _T("Feedback"));
    
    buttonSizer->Add(m_acceptButton, 0, wxALL, 5);
    buttonSizer->Add(m_dismissButton, 0, wxALL, 5);
    buttonSizer->Add(m_feedbackButton, 0, wxALL, 5);
    
    m_notificationSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);
    
    m_notificationPanel->SetSizer(m_notificationSizer);
    m_notificationPanel->Hide();
}

void InterventionManager::ShowNotification(const wxString& title, const wxString& message,
                                           const wxString& hint,
                                           const std::vector<wxString>& options)
{
    if (!m_notificationPanel || !m_parentFrame)
        return;
    
    // Update text
    m_titleText->SetLabel(title);
    m_messageText->SetLabel(message);
    m_hintText->SetLabel(hint);
    
    // Show/hide hint
    m_hintText->Show(!hint.IsEmpty());
    
    // Configure buttons based on options
    if (options.size() >= 1)
    {
        m_acceptButton->SetLabel(options[0]);
        m_acceptButton->Show(true);
    }
    else
    {
        m_acceptButton->Show(false);
    }
    
    if (options.size() >= 2)
    {
        m_dismissButton->SetLabel(options[1]);
        m_dismissButton->Show(true);
    }
    else
    {
        m_dismissButton->Show(false);
    }
    
    m_feedbackButton->Show(true);
    
    // Position at top-right of parent frame
    UpdateNotificationPosition();
    
    // Show panel
    m_notificationPanel->Show();
    m_notificationPanel->Raise();
    m_notificationSizer->Layout();
    
    // Auto-hide after 15 seconds
    m_autoHideTimer.Start(15000, wxTIMER_ONE_SHOT);
}

void InterventionManager::UpdateNotificationPosition()
{
    if (!m_notificationPanel || !m_parentFrame)
        return;
    
    wxRect frameRect = m_parentFrame->GetRect();
    wxSize panelSize = m_notificationPanel->GetSize();
    
    int x = frameRect.GetRight() - panelSize.GetWidth() - 20;
    int y = frameRect.GetTop() + 50;
    
    m_notificationPanel->SetPosition(wxPoint(x, y));
}

void InterventionManager::HideNotification()
{
    if (m_notificationPanel)
    {
        m_notificationPanel->Hide();
        m_autoHideTimer.Stop();
    }
}

wxString InterventionManager::GenerateInterventionId()
{
    m_interventionCounter++;
    wxDateTime now = wxDateTime::Now();
    return wxString::Format(_T("INT_%s_%d_%d"),
                           now.Format(_T("%Y%m%d_%H%M%S")),
                           m_interventionCounter,
                           wxThread::GetCurrentId());
}

void InterventionManager::SaveInterventionToHistory(const Intervention& intervention)
{
    m_interventionHistory.push_back(intervention);
    
    // Keep only last 100 interventions
    if (m_interventionHistory.size() > 100)
        m_interventionHistory.erase(m_interventionHistory.begin());
}

void InterventionManager::RecordIntervention(const wxString& interventionId, bool accepted,
                                             int reliefScore, bool helpful)
{
    for (auto& intervention : m_interventionHistory)
    {
        if (intervention.id == interventionId)
        {
            intervention.accepted = accepted;
            intervention.responseTime = wxDateTime::Now();
            intervention.reliefScore = reliefScore;
            break;
        }
    }
    
    // Log for analysis
    wxString logMsg = wxString::Format(_T("Intervention %s: accepted=%d, relief=%d"),
                                       interventionId, accepted ? 1 : 0, reliefScore);
    LogIntervention(logMsg);
}

void InterventionManager::RecordUserFeedback(const wxString& interventionId, int rating,
                                             const wxString& comment)
{
    UserFeedback feedback;
    feedback.timestamp = wxDateTime::Now();
    feedback.interventionId = interventionId;
    feedback.rating = rating;
    feedback.comment = comment;
    feedback.helpful = (rating >= 4);
    
    m_userFeedback.push_back(feedback);
    
    // Save to file
    wxString feedbackFile = m_userDataPath + wxFILE_SEP_PATH + _T("user_feedback.csv");
    wxTextFile file(feedbackFile);
    if (file.Exists())
        file.Open();
    else
        file.Create();
    
    file.AddLine(wxString::Format(_T("%s,%s,%d,%s"),
                                   feedback.timestamp.FormatISOCombined(),
                                   interventionId,
                                   rating,
                                   comment));
    file.Write();
    file.Close();
}

int InterventionManager::GetInterventionCount(AnxietyLevel level) const
{
    if (level == AnxietyLevel::UNKNOWN)
        return m_interventionHistory.size();
    
    int count = 0;
    for (const auto& intervention : m_interventionHistory)
    {
        if (intervention.anxietyLevel == level)
            count++;
    }
    return count;
}

double InterventionManager::GetAverageReliefScore() const
{
    int total = 0;
    int count = 0;
    
    for (const auto& intervention : m_interventionHistory)
    {
        if (intervention.reliefScore > 0)
        {
            total += intervention.reliefScore;
            count++;
        }
    }
    
    return count > 0 ? static_cast<double>(total) / count : 0.0;
}

void InterventionManager::LogIntervention(const wxString& message, AnxietyLevel level)
{
    wxString logFile = m_userDataPath + wxFILE_SEP_PATH + _T("intervention_log.txt");
    wxTextFile file(logFile);
    if (file.Exists())
        file.Open();
    else
        file.Create();
    
    wxString levelStr;
    switch (level)
    {
        case AnxietyLevel::LOW: levelStr = _T("LOW"); break;
        case AnxietyLevel::MODERATE: levelStr = _T("MODERATE"); break;
        case AnxietyLevel::HIGH: levelStr = _T("HIGH"); break;
        case AnxietyLevel::EXTREME: levelStr = _T("EXTREME"); break;
        default: levelStr = _T("UNKNOWN");
    }
    
    file.AddLine(wxString::Format(_T("[%s] [%s] %s"),
                                   wxDateTime::Now().FormatISOCombined(),
                                   levelStr,
                                   message));
    file.Write();
    file.Close();
}

// Event handlers
void InterventionManager::OnInterventionButton(wxCommandEvent& event)
{
    wxButton* btn = dynamic_cast<wxButton*>(event.GetEventObject());
    if (!btn) return;
    
    wxString label = btn->GetLabel();
    
    if (label == _T("Show Hint") || label == _T("Take Break"))
    {
        // User accepted
        RecordIntervention(m_currentInterventionId, true, -1, true);
        HideNotification();
        
        // Show hint dialog
        if (!m_hintText->GetLabel().IsEmpty())
        {
            wxMessageDialog dlg(m_parentFrame,
                m_hintText->GetLabel(),
                _T("Helpful Hint"),
                wxOK | wxCENTRE | wxICON_INFORMATION);
            dlg.ShowModal();
        }
    }
    else if (label == _T("Dismiss") || label == _T("Continue"))
    {
        // User dismissed
        RecordIntervention(m_currentInterventionId, false, -1, false);
        HideNotification();
    }
    else if (label == _T("Feedback"))
    {
        // Show feedback dialog
        wxString msg = _T("Was this intervention helpful?\n\nRate from 1 (not helpful) to 5 (very helpful):");
        wxTextEntryDialog dlg(m_parentFrame, msg, _T("Feedback"), _T("3"));
        
        if (dlg.ShowModal() == wxID_OK)
        {
            long rating = 3;
            dlg.GetValue().ToLong(&rating);
            RecordUserFeedback(m_currentInterventionId, static_cast<int>(rating));
        }
    }
    
    wxCommandEvent customEvent(wxEVT_INTERVENTION_ACTION);
    customEvent.SetString(label);
    wxPostEvent(this, customEvent);
}

void InterventionManager::OnInterventionClose(wxCommandEvent& event)
{
    HideNotification();
}

void InterventionManager::OnInterventionTimer(wxTimerEvent& event)
{
    HideNotification();
}

void InterventionManager::OnFeedbackSubmit(wxCommandEvent& event)
{
    // Handle feedback submission
}

// Static helper methods
wxString InterventionManager::GetHintForError(const wxString& errorType)
{
    static std::map<wxString, wxString> hints;
    if (hints.empty())
    {
        hints[_T("syntax error")] = _T("Check for missing semicolons, brackets, or parentheses");
        hints[_T("missing semicolon")] = _T("You might be missing a semicolon at the end of a statement");
        hints[_T("undefined reference")] = _T("You might be missing a header file or library link");
        hints[_T("missing header")] = _T("Check if you've included the necessary header files");
        hints[_T("segmentation fault")] = _T("Check for null pointers or array bounds");
        hints[_T("null pointer")] = _T("Make sure to initialize pointers before using them");
        hints[_T("array bounds")] = _T("Ensure array indices are within bounds");
        hints[_T("uninitialized")] = _T("Initialize variables before using them");
        hints[_T("memory leak")] = _T("Remember to free allocated memory");
        hints[_T("buffer overflow")] = _T("Check array sizes and string lengths");
        hints[_T("type mismatch")] = _T("Ensure types are compatible");
        hints[_T("no matching function")] = _T("Check function parameters and overloads");
        hints[_T("ambiguous")] = _T("Make the call more specific");
        hints[_T("redefinition")] = _T("Remove duplicate declarations");
        hints[_T("undeclared")] = _T("Declare variables before using them");
        hints[_T("incomplete type")] = _T("Include the full type definition");
        hints[_T("general")] = _T("Take a deep breath. Try breaking the problem down into smaller parts.");
    }
    
    wxString lowerType = errorType.Lower();
    for (const auto& [key, hint] : hints)
    {
        if (lowerType.Contains(key))
            return hint;
    }
    
    return hints[_T("general")];
}

wxString InterventionManager::GetRelaxationSuggestion()
{
    static std::vector<wxString> messages = {
        _T("Take a deep breath. The solution is often simpler than it seems."),
        _T("Consider taking a 2-minute break to clear your mind."),
        _T("Try breaking the problem down into smaller parts."),
        _T("Sometimes walking away for a moment helps. Why not stretch?"),
        _T("You've solved harder problems before. You can do this!"),
        _T("Take a moment to review your logic step by step."),
        _T("Remember: every expert was once a beginner."),
        _T("Try explaining the problem to someone (or to a rubber duck)."),
        _T("Your brain needs rest. A short break will help."),
        _T("Progress, not perfection. You're getting there!")
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, messages.size() - 1);
    
    return messages[dis(gen)];
}

wxString InterventionManager::GetRandomEncouragement()
{
    static std::vector<wxString> messages = {
        _T("You're making great progress!"),
        _T("Keep up the good work!"),
        _T("Every error is a learning opportunity."),
        _T("You've got this!"),
        _T("Persistence pays off!"),
        _T("Your code is getting better with every line!"),
        _T("Debugging is just problem-solving in disguise."),
        _T("You're building something great!"),
        _T("Small steps lead to big achievements.")
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, messages.size() - 1);
    
    return messages[dis(gen)];
}

void InterventionManager::PlayNotificationSound()
{
#ifdef __WXMSW__
    // Play Windows default notification sound
    MessageBeep(MB_ICONASTERISK);
#else
    wxBell();
#endif
}

void InterventionManager::PlaySuccessSound()
{
#ifdef __WXMSW__
    MessageBeep(MB_OK);
#else
    wxBell();
#endif
}

// InterventionDialog implementation
InterventionDialog::InterventionDialog(wxWindow* parent, const wxString& title,
                                       const wxString& message, const wxString& hint,
                                       InterventionType type)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(450, 350))
    , m_accepted(false)
    , m_rating(3)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Message
    wxStaticText* msgText = new wxStaticText(this, wxID_ANY, message);
    msgText->Wrap(400);
    mainSizer->Add(msgText, 0, wxALL | wxEXPAND, 10);
    
    // Hint (if provided)
    if (!hint.IsEmpty())
    {
        wxStaticText* hintText = new wxStaticText(this, wxID_ANY, hint);
        hintText->SetForegroundColour(wxColour(0, 100, 0));
        hintText->Wrap(400);
        mainSizer->Add(hintText, 0, wxALL | wxEXPAND, 10);
    }
    
    mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 5);
    
    // Rating
    wxStaticText* ratingLabel = new wxStaticText(this, wxID_ANY, _T("How helpful was this?"));
    mainSizer->Add(ratingLabel, 0, wxALL, 5);
    
    wxString ratingChoices[] = {_T("1 - Not helpful"), _T("2"), _T("3 - Somewhat"), 
                                 _T("4"), _T("5 - Very helpful")};
    m_ratingBox = new wxRadioBox(this, wxID_ANY, _T("Rating"), wxDefaultPosition, 
                                  wxDefaultSize, 5, ratingChoices, 1, wxRA_SPECIFY_COLS);
    mainSizer->Add(m_ratingBox, 0, wxALL | wxEXPAND, 5);
    
    // Comment
    wxStaticText* commentLabel = new wxStaticText(this, wxID_ANY, _T("Additional comments (optional):"));
    mainSizer->Add(commentLabel, 0, wxALL, 5);
    
    m_commentCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, 
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE);
    mainSizer->Add(m_commentCtrl, 1, wxALL | wxEXPAND, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* acceptBtn = new wxButton(this, wxID_OK, _T("Accept & Continue"));
    wxButton* dismissBtn = new wxButton(this, wxID_CANCEL, _T("Dismiss"));
    
    acceptBtn->Bind(wxEVT_BUTTON, &InterventionDialog::OnAccept, this);
    dismissBtn->Bind(wxEVT_BUTTON, &InterventionDialog::OnDismiss, this);
    
    buttonSizer->Add(acceptBtn, 0, wxALL, 5);
    buttonSizer->Add(dismissBtn, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizerAndFit(mainSizer);
    Centre();
}

void InterventionDialog::OnAccept(wxCommandEvent& event)
{
    m_accepted = true;
    m_rating = m_ratingBox->GetSelection() + 1;
    m_comment = m_commentCtrl->GetValue();
    EndModal(wxID_OK);
}

void InterventionDialog::OnDismiss(wxCommandEvent& event)
{
    m_accepted = false;
    EndModal(wxID_CANCEL);
}

void InterventionDialog::OnRate(wxCommandEvent& event)
{
    // Handle rating change if needed
}

void InterventionDialog::OnSubmitFeedback(wxCommandEvent& event)
{
    // Handle feedback submission
}
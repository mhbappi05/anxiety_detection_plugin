#ifndef INTERVENTION_MANAGER_H
#define INTERVENTION_MANAGER_H

#include <wx/string.h>
#include <wx/datetime.h>
#include <wx/timer.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <vector>
#include <map>
#include <memory>
#include <atomic>

// Forward declarations
class wxFrame;
class wxPanel;
class wxStaticText;
class wxButton;
class wxBoxSizer;

// Anxiety levels (matching Python backend)
enum class AnxietyLevel
{
    LOW,
    MODERATE,
    HIGH,
    EXTREME,
    UNKNOWN
};

// Intervention types
enum class InterventionType
{
    ERROR_HINT,
    BREAK_SUGGESTION,
    ENCOURAGEMENT,
    SUCCESS_CELEBRATION,
    CALIBRATION_REQUEST,
    STATISTICS_SHOW
};

// Intervention severity
enum class InterventionSeverity
{
    INFO,
    SUGGESTION,
    WARNING,
    CRITICAL
};

// Structure to store intervention data
struct Intervention
{
    wxString id;
    wxDateTime timestamp;
    AnxietyLevel anxietyLevel;
    InterventionType type;
    InterventionSeverity severity;
    wxString title;
    wxString message;
    wxString hint;
    wxString errorType;
    std::vector<wxString> options;
    bool accepted;
    bool dismissed;
    wxDateTime responseTime;
    int reliefScore;  // User-reported relief (1-10)
    double confidence;
    std::vector<wxString> triggeredFeatures;
};

// Structure for user feedback
struct UserFeedback
{
    wxDateTime timestamp;
    wxString interventionId;
    bool helpful;
    int rating;  // 1-5 stars
    wxString comment;
};

// Callback for intervention actions
typedef std::function<void(const wxString& interventionId, int action)> InterventionCallback;

class InterventionManager : public wxEvtHandler
{
public:
    InterventionManager();
    virtual ~InterventionManager();

    // Initialization
    void Initialize(wxFrame* parentFrame);
    void LoadConfiguration(const wxString& configPath);
    void SaveConfiguration();

    // Settings
    void SetCooldownPeriod(int seconds) { m_cooldownSeconds = seconds; }
    int GetCooldownPeriod() const { return m_cooldownSeconds; }
    
    void SetAnxietyThreshold(float threshold) { m_anxietyThreshold = threshold; }
    float GetAnxietyThreshold() const { return m_anxietyThreshold; }
    
    void SetEnableC(bool enable) { m_enableC = enable; }
    void SetEnableCpp(bool enable) { m_enableCpp = enable; }
    bool IsLanguageEnabled(const wxString& language) const;
    
    void SetShowNotifications(bool show) { m_showNotifications = show; }
    bool GetShowNotifications() const { return m_showNotifications; }
    
    void SetPlaySounds(bool play) { m_playSounds = play; }
    bool GetPlaySounds() const { return m_playSounds; }

    // Intervention logic
    bool ShouldIntervene(AnxietyLevel level, double confidence, 
                         const std::vector<wxString>& triggeredFeatures);
    
    void ShowIntervention(const wxString& title, const wxString& message, 
                          InterventionType type = InterventionType::ENCOURAGEMENT,
                          const wxString& hint = wxEmptyString,
                          const wxString& errorType = wxEmptyString);
    
    void ShowErrorHint(const wxString& errorType, const wxString& errorMessage);
    void ShowBreakSuggestion();
    void ShowEncouragement();
    void ShowSuccessMessage();
    void ShowCalibrationDialog();
    void ShowStatisticsDialog(const wxString& stats);

    // Feedback handling
    void RecordIntervention(const wxString& interventionId, bool accepted, 
                           int reliefScore = -1, bool helpful = true);
    void RecordUserFeedback(const wxString& interventionId, int rating, 
                           const wxString& comment = wxEmptyString);
    
    // History and statistics
    const std::vector<Intervention>& GetInterventionHistory() const { return m_interventionHistory; }
    int GetInterventionCount(AnxietyLevel level = AnxietyLevel::UNKNOWN) const;
    double GetAverageReliefScore() const;
    wxDateTime GetLastInterventionTime() const { return m_lastIntervention; }
    
    // Error hints database
    static wxString GetHintForError(const wxString& errorType);
    static wxString GetRelaxationSuggestion();
    static wxString GetRandomEncouragement();
    
    // Configuration
    void LoadErrorHints(const wxString& configPath);
    void LoadRelaxationMessages(const wxString& configPath);
    void LoadSuccessMessages(const wxString& configPath);

    // Event handlers
    void OnInterventionButton(wxCommandEvent& event);
    void OnInterventionClose(wxCommandEvent& event);
    void OnInterventionTimer(wxTimerEvent& event);
    void OnFeedbackSubmit(wxCommandEvent& event);

private:
    // State
    wxFrame* m_parentFrame;
    std::atomic<bool> m_onCooldown;
    wxDateTime m_lastIntervention;
    int m_cooldownSeconds;
    float m_anxietyThreshold;
    bool m_enableC;
    bool m_enableCpp;
    bool m_showNotifications;
    bool m_playSounds;
    
    // Intervention tracking
    std::vector<Intervention> m_interventionHistory;
    std::vector<UserFeedback> m_userFeedback;
    wxString m_currentInterventionId;
    int m_interventionCounter;
    
    // Message databases
    std::map<wxString, wxString> m_errorHints;
    std::vector<wxString> m_relaxationMessages;
    std::vector<wxString> m_encouragementMessages;
    std::vector<wxString> m_successMessages;
    
    // UI components
    wxPanel* m_notificationPanel;
    wxStaticText* m_titleText;
    wxStaticText* m_messageText;
    wxStaticText* m_hintText;
    wxButton* m_acceptButton;
    wxButton* m_dismissButton;
    wxButton* m_feedbackButton;
    wxBoxSizer* m_notificationSizer;
    wxTimer m_autoHideTimer;
    
    // Helper methods
    bool IsOnCooldown() const;
    void StartCooldown();
    void ResetCooldown();
    
    void CreateNotificationWindow();
    void ShowNotification(const wxString& title, const wxString& message, 
                          const wxString& hint = wxEmptyString,
                          const std::vector<wxString>& options = {});
    void HideNotification();
    void UpdateNotificationPosition();
    
    wxString GenerateInterventionId();
    void SaveInterventionToHistory(const Intervention& intervention);
    void LogIntervention(const wxString& message, AnxietyLevel level = AnxietyLevel::UNKNOWN);
    
    // Sound effects
    void PlayNotificationSound();
    void PlaySuccessSound();
    
    // Configuration file handling
    wxString m_configPath;
    wxString m_userDataPath;
    
    // Callback for custom actions
    InterventionCallback m_callback;
    
    DECLARE_EVENT_TABLE()
};

// Custom events for intervention actions
wxDECLARE_EVENT(wxEVT_INTERVENTION_ACTION, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_INTERVENTION_CLOSE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FEEDBACK_SUBMIT, wxCommandEvent);

// Helper class for custom intervention dialog
class InterventionDialog : public wxDialog
{
public:
    InterventionDialog(wxWindow* parent, const wxString& title, 
                       const wxString& message, const wxString& hint,
                       InterventionType type);
    
    bool WasAccepted() const { return m_accepted; }
    int GetUserRating() const { return m_rating; }
    wxString GetUserComment() const { return m_comment; }

private:
    void OnAccept(wxCommandEvent& event);
    void OnDismiss(wxCommandEvent& event);
    void OnRate(wxCommandEvent& event);
    void OnSubmitFeedback(wxCommandEvent& event);
    
    bool m_accepted;
    int m_rating;
    wxString m_comment;
    wxTextCtrl* m_commentCtrl;
    wxRadioBox* m_ratingBox;
};

#endif
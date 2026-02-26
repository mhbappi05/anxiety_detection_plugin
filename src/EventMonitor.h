#ifndef EVENT_MONITOR_H
#define EVENT_MONITOR_H

#include <wx/string.h>
#include <wx/datetime.h>
#include <vector>
#include <deque>
#include <map>
#include <mutex>
#include <atomic>

// Error type enumeration
enum ErrorType
{
    ERROR_UNKNOWN = 0,
    ERROR_SYNTAX,
    ERROR_MISSING_SEMICOLON,
    ERROR_UNDEFINED_REF,
    ERROR_MISSING_HEADER,
    ERROR_SEGFAULT,
    ERROR_NULL_POINTER,
    ERROR_BOUNDS,
    ERROR_UNINITIALIZED,
    ERROR_MEMORY_LEAK,
    ERROR_BUFFER_OVERFLOW,
    ERROR_TYPE_MISMATCH,
    ERROR_NO_MATCH,
    ERROR_AMBIGUOUS,
    ERROR_REDEFINITION,
    ERROR_UNDECLARED,
    ERROR_INCOMPLETE_TYPE
};

// Compile language
enum CompileLanguage
{
    LANG_CPP = 0,
    LANG_C
};

// Keystroke event data
struct KeystrokeEvent
{
    wxDateTime timestamp;
    char key;
    bool isBackspace;
    int keyCode;
    long modifiers;
};

// Compile event data
struct CompileEvent
{
    wxDateTime timestamp;
    wxString output;
    bool success;
    CompileLanguage language;
    int errorCount;
    int warningCount;
    wxString errorMessage;
    ErrorType errorType;

    CompileEvent()
        : success(false), language(LANG_CPP),
          errorCount(0), warningCount(0), errorType(ERROR_UNKNOWN)
    {}
};

// Session data
struct SessionData
{
    wxDateTime sessionStart;
    wxDateTime lastActivity;
    std::vector<KeystrokeEvent> keystrokes;
    std::deque<KeystrokeEvent> rollingKeystrokes;
    std::vector<CompileEvent> compiles;
    std::vector<wxString> errorSequence;

    int totalKeystrokes;
    int totalBackspaces;
    int totalCompiles;
    int failedCompiles;
    int repeatedErrors;

    double realTimeWPM;
    double realTimeBackspaceRate;

    SessionData()
        : totalKeystrokes(0), totalBackspaces(0),
          totalCompiles(0), failedCompiles(0), repeatedErrors(0),
          realTimeWPM(0.0), realTimeBackspaceRate(0.0)
    {}
};

class EventMonitor
{
public:
    EventMonitor();
    ~EventMonitor();

    void StartMonitoring();
    void StopMonitoring();
    bool IsMonitoring() const { return m_isMonitoring; }

    void RecordKeystroke(char key, bool isBackspace, int keyCode = 0, long modifiers = 0);
    void RecordCompile(const wxString& output, bool success, CompileLanguage lang = LANG_CPP);

    std::vector<double> ExtractFeatures() const;
    SessionData GetCurrentSession() const;
    wxString GetSessionJSON() const;
    void SaveSessionData(const wxString& filepath);
    void ResetSession();

private:
    std::atomic<bool> m_isMonitoring;
    mutable std::mutex m_dataMutex;
    SessionData m_currentSession;

    std::map<wxString, ErrorType> m_errorPatterns;

    // Static baseline
    static SessionData s_baselineData;
    static bool s_hasBaseline;

    void InitializeErrorPatterns();
    void UpdateRollingWindow(const KeystrokeEvent& event);
    void CalculateRealTimeMetrics();
    void UpdateBaseline();

    void ParseCppOutput(CompileEvent& event);
    void ParseCOutput(CompileEvent& event);
    ErrorType ClassifyError(const wxString& errorMessage);
    wxString NormalizeErrorMessage(const wxString& error);

    double CalculateTypingVelocity() const;
    double CalculateKeystrokeVariance() const;
    double CalculateBackspaceRate() const;
    double CalculateREDMetric() const;
    double CalculateFocusSwitches() const;
    double CalculateIdleRatio() const;
    double CalculateUndoRedoRate() const;
};

#endif // EVENT_MONITOR_H
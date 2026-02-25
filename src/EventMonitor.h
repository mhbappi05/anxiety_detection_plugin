#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/regex.h>
#include <algorithm>
#include <cmath>
#include <numeric>

SessionData EventMonitor::s_baselineData;
bool EventMonitor::s_hasBaseline = false;
std::map<wxString, int> EventMonitor::s_errorPatterns;

EventMonitor::EventMonitor() 
    : m_isMonitoring(false)
{
    ResetSession();
    InitializeErrorPatterns();
}

EventMonitor::~EventMonitor()
{
    StopMonitoring();
}

void EventMonitor::InitializeErrorPatterns()
{
    // Initialize common C/C++ error patterns
    m_errorPatterns = {
        // C/C++ Common Errors
        {_T("syntax error"), ERROR_SYNTAX},
        {_T("expected ';'"), ERROR_MISSING_SEMICOLON},
        {_T("undefined reference"), ERROR_UNDEFINED_REF},
        {_T("cannot find"), ERROR_MISSING_HEADER},
        {_T("segmentation fault"), ERROR_SEGFAULT},
        {_T("null pointer"), ERROR_NULL_POINTER},
        {_T("out of bounds"), ERROR_BOUNDS},
        {_T("uninitialized"), ERROR_UNINITIALIZED},
        {_T("memory leak"), ERROR_MEMORY_LEAK},
        {_T("buffer overflow"), ERROR_BUFFER_OVERFLOW},
        {_T("type mismatch"), ERROR_TYPE_MISMATCH},
        {_T("no matching function"), ERROR_NO_MATCH},
        {_T("ambiguous"), ERROR_AMBIGUOUS},
        {_T("redefinition"), ERROR_REDEFINITION},
        {_T("not declared"), ERROR_UNDECLARED},
        {_T("incomplete type"), ERROR_INCOMPLETE_TYPE}
    };
}

void EventMonitor::StartMonitoring()
{
    m_isMonitoring = true;
    ResetSession();
    Manager::Get()->GetLogManager()->DebugLog(_T("Anxiety monitoring started"));
}

void EventMonitor::StopMonitoring()
{
    m_isMonitoring = false;
    UpdateBaseline();
    Manager::Get()->GetLogManager()->DebugLog(_T("Anxiety monitoring stopped"));
}

void EventMonitor::RecordKeystroke(char key, bool isBackspace, int keyCode, long modifiers)
{
    if (!m_isMonitoring)
        return;
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    KeystrokeEvent event;
    event.timestamp = wxDateTime::Now();
    event.key = key;
    event.isBackspace = isBackspace;
    event.keyCode = keyCode;
    event.modifiers = modifiers;
    
    m_currentSession.keystrokes.push_back(event);
    m_currentSession.totalKeystrokes++;
    
    if (isBackspace)
        m_currentSession.totalBackspaces++;
    
    m_currentSession.lastActivity = event.timestamp;
    
    // Update rolling window
    UpdateRollingWindow(event);
}

void EventMonitor::RecordCompile(const wxString& output, bool success, CompileLanguage lang)
{
    if (!m_isMonitoring)
        return;
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    CompileEvent event;
    event.timestamp = wxDateTime::Now();
    event.output = output;
    event.success = success;
    event.language = lang;
    
    // Parse compiler output based on language
    if (lang == LANG_CPP)
        ParseCppOutput(event);
    else if (lang == LANG_C)
        ParseCOutput(event);
    
    m_currentSession.compiles.push_back(event);
    m_currentSession.totalCompiles++;
    
    if (!success)
    {
        m_currentSession.failedCompiles++;
        
        // Update RED metric tracking
        if (!event.errorMessage.IsEmpty())
        {
            wxString normalized = NormalizeErrorMessage(event.errorMessage);
            m_currentSession.errorSequence.push_back(normalized);
            
            // Check for repeats
            if (m_currentSession.errorSequence.size() > 1)
            {
                wxString lastError = m_currentSession.errorSequence.back();
                wxString prevError = m_currentSession.errorSequence[m_currentSession.errorSequence.size() - 2];
                if (lastError == prevError)
                {
                    m_currentSession.repeatedErrors++;
                }
            }
        }
    }
    
    m_currentSession.lastActivity = event.timestamp;
}

void EventMonitor::ParseCppOutput(CompileEvent& event)
{
    wxString output = event.output.Lower();
    
    // Count errors and warnings
    wxRegEx errorRegex(_T("error:"), wxRE_ADVANCED);
    wxRegEx warningRegex(_T("warning:"), wxRE_ADVANCED);
    
    size_t pos = 0;
    while (errorRegex.Match(output, &pos))
    {
        event.errorCount++;
        pos += 5; // length of "error"
    }
    
    pos = 0;
    while (warningRegex.Match(output, &pos))
    {
        event.warningCount++;
        pos += 7; // length of "warning"
    }
    
    // Extract first error message
    int errorPos = output.Find(_T("error:"));
    if (errorPos != wxNOT_FOUND)
    {
        int endPos = output.find('\n', errorPos);
        if (endPos != wxString::npos)
            event.errorMessage = output.Mid(errorPos, endPos - errorPos);
        else
            event.errorMessage = output.Mid(errorPos);
        
        // Classify error type
        event.errorType = ClassifyError(event.errorMessage);
    }
}

void EventMonitor::ParseCOutput(CompileEvent& event)
{
    // C compiler output often similar but with slight differences
    wxString output = event.output.Lower();
    
    wxRegEx errorRegex(_T("error:"), wxRE_ADVANCED);
    wxRegEx warningRegex(_T("warning:"), wxRE_ADVANCED);
    
    size_t pos = 0;
    while (errorRegex.Match(output, &pos))
    {
        event.errorCount++;
        pos += 5;
    }
    
    pos = 0;
    while (warningRegex.Match(output, &pos))
    {
        event.warningCount++;
        pos += 7;
    }
    
    // Extract first error
    int errorPos = output.Find(_T("error:"));
    if (errorPos != wxNOT_FOUND)
    {
        int endPos = output.find('\n', errorPos);
        if (endPos != wxString::npos)
            event.errorMessage = output.Mid(errorPos, endPos - errorPos);
        else
            event.errorMessage = output.Mid(errorPos);
        
        event.errorType = ClassifyError(event.errorMessage);
    }
}

ErrorType EventMonitor::ClassifyError(const wxString& errorMessage)
{
    wxString lowerMsg = errorMessage.Lower();
    
    for (const auto& pattern : m_errorPatterns)
    {
        if (lowerMsg.Contains(pattern.first))
            return pattern.second;
    }
    
    return ERROR_UNKNOWN;
}

wxString EventMonitor::NormalizeErrorMessage(const wxString& error)
{
    wxString normalized = error;
    
    // Remove file paths
    wxRegEx pathRegex(_T("[a-zA-Z]:\\\\(?:[^\\\\]+\\\\)*[^\\\\]+\\.(?:cpp|c|h)"), wxRE_ADVANCED);
    pathRegex.Replace(&normalized, _T("file"));
    
    // Remove line numbers
    wxRegEx lineRegex(_T("line\\s+\\d+"), wxRE_ADVANCED);
    lineRegex.Replace(&normalized, _T("line"));
    
    // Remove column numbers
    wxRegEx colRegex(_T("column\\s+\\d+"), wxRE_ADVANCED);
    colRegex.Replace(&normalized, _T("column"));
    
    // Remove memory addresses
    wxRegEx addrRegex(_T("0x[0-9a-f]+"), wxRE_ADVANCED);
    addrRegex.Replace(&normalized, _T("address"));
    
    // Remove numbers
    wxRegEx numRegex(_T("\\b\\d+\\b"), wxRE_ADVANCED);
    numRegex.Replace(&normalized, _T("num"));
    
    return normalized;
}

void EventMonitor::UpdateRollingWindow(const KeystrokeEvent& event)
{
    m_currentSession.rollingKeystrokes.push_back(event);
    if (m_currentSession.rollingKeystrokes.size() > 100)
        m_currentSession.rollingKeystrokes.pop_front();
    
    // Calculate real-time metrics
    if (m_currentSession.rollingKeystrokes.size() > 10)
    {
        CalculateRealTimeMetrics();
    }
}

void EventMonitor::CalculateRealTimeMetrics()
{
    auto& window = m_currentSession.rollingKeystrokes;
    if (window.size() < 2)
        return;
    
    // Calculate typing speed over rolling window
    wxTimeSpan windowTime = window.back().timestamp - window.front().timestamp;
    double minutes = windowTime.GetSeconds().ToDouble() / 60.0;
    if (minutes > 0)
    {
        m_currentSession.realTimeWPM = (window.size() / 5.0) / minutes;
    }
    
    // Calculate backspace rate over window
    int backspaces = 0;
    for (const auto& ks : window)
    {
        if (ks.isBackspace)
            backspaces++;
    }
    m_currentSession.realTimeBackspaceRate = static_cast<double>(backspaces) / window.size();
}

void EventMonitor::ResetSession()
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    m_currentSession.keystrokes.clear();
    m_currentSession.compiles.clear();
    m_currentSession.errorSequence.clear();
    m_currentSession.rollingKeystrokes.clear();
    m_currentSession.sessionStart = wxDateTime::Now();
    m_currentSession.lastActivity = wxDateTime::Now();
    m_currentSession.totalKeystrokes = 0;
    m_currentSession.totalBackspaces = 0;
    m_currentSession.totalCompiles = 0;
    m_currentSession.failedCompiles = 0;
    m_currentSession.repeatedErrors = 0;
    m_currentSession.realTimeWPM = 0;
    m_currentSession.realTimeBackspaceRate = 0;
}

SessionData EventMonitor::GetCurrentSession() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_currentSession;
}

wxString EventMonitor::GetSessionJSON() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    wxString json = _T("{");
    
    // Session info
    json += wxString::Format(_T("\"session_start\":\"%s\","), 
        m_currentSession.sessionStart.FormatISOCombined());
    json += wxString::Format(_T("\"last_activity\":\"%s\","),
        m_currentSession.lastActivity.FormatISOCombined());
    
    // Keystrokes
    json += _T("\"keystrokes\":[");
    for (size_t i = 0; i < m_currentSession.keystrokes.size(); i++)
    {
        const auto& ks = m_currentSession.keystrokes[i];
        json += wxString::Format(_T("{\"timestamp\":\"%s\",\"key\":\"%c\",\"is_backspace\":%s}"),
            ks.timestamp.FormatISOCombined(),
            ks.key,
            ks.isBackspace ? _T("true") : _T("false"));
        if (i < m_currentSession.keystrokes.size() - 1)
            json += _T(",");
    }
    json += _T("],");
    
    // Compiles
    json += _T("\"compiles\":[");
    for (size_t i = 0; i < m_currentSession.compiles.size(); i++)
    {
        const auto& comp = m_currentSession.compiles[i];
        wxString errorMsg = comp.errorMessage;
        errorMsg.Replace(_T("\""), _T("\\\"")); // Escape quotes
        
        json += wxString::Format(_T("{\"timestamp\":\"%s\",\"success\":%s,\"error_count\":%d,\"warning_count\":%d,\"error_message\":\"%s\"}"),
            comp.timestamp.FormatISOCombined(),
            comp.success ? _T("true") : _T("false"),
            comp.errorCount,
            comp.warningCount,
            errorMsg);
        if (i < m_currentSession.compiles.size() - 1)
            json += _T(",");
    }
    json += _T("],");
    
    // Summary stats
    json += wxString::Format(_T("\"total_keystrokes\":%d,"), m_currentSession.totalKeystrokes);
    json += wxString::Format(_T("\"total_backspaces\":%d,"), m_currentSession.totalBackspaces);
    json += wxString::Format(_T("\"total_compiles\":%d,"), m_currentSession.totalCompiles);
    json += wxString::Format(_T("\"failed_compiles\":%d,"), m_currentSession.failedCompiles);
    json += wxString::Format(_T("\"repeated_errors\":%d"), m_currentSession.repeatedErrors);
    
    json += _T("}");
    
    return json;
}

void EventMonitor::SaveSessionData(const wxString& filepath)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    wxTextFile file(filepath);
    if (file.Exists())
        file.Open();
    else
        file.Create();
    
    // Write header if file is empty
    if (file.GetLineCount() == 0)
    {
        file.AddLine(_T("timestamp,type,key,is_backspace,compile_success,error_count,warning_count,error_type,language"));
    }
    
    // Write keystrokes
    for (const auto& ks : m_currentSession.keystrokes)
    {
        wxString line = wxString::Format(_T("%s,keystroke,%c,%d,,,,,"),
            ks.timestamp.FormatISOTime(),
            ks.key,
            ks.isBackspace ? 1 : 0);
        file.AddLine(line);
    }
    
    // Write compiles
    for (const auto& comp : m_currentSession.compiles)
    {
        wxString line = wxString::Format(_T("%s,compile,,,%d,%d,%d,%d,%d"),
            comp.timestamp.FormatISOTime(),
            comp.success ? 1 : 0,
            comp.errorCount,
            comp.warningCount,
            static_cast<int>(comp.errorType),
            static_cast<int>(comp.language));
        file.AddLine(line);
    }
    
    file.Write();
    file.Close();
}

void EventMonitor::UpdateBaseline()
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    if (!s_hasBaseline)
    {
        s_baselineData = m_currentSession;
        s_hasBaseline = true;
    }
    else
    {
        // Exponential moving average for baseline
        const double alpha = 0.1; // Weight for new data
        
        s_baselineData.totalKeystrokes = static_cast<int>(
            (1 - alpha) * s_baselineData.totalKeystrokes + 
            alpha * m_currentSession.totalKeystrokes);
        s_baselineData.totalBackspaces = static_cast<int>(
            (1 - alpha) * s_baselineData.totalBackspaces + 
            alpha * m_currentSession.totalBackspaces);
        s_baselineData.totalCompiles = static_cast<int>(
            (1 - alpha) * s_baselineData.totalCompiles + 
            alpha * m_currentSession.totalCompiles);
        s_baselineData.failedCompiles = static_cast<int>(
            (1 - alpha) * s_baselineData.failedCompiles + 
            alpha * m_currentSession.failedCompiles);
    }
}

std::vector<double> EventMonitor::ExtractFeatures() const
{
    std::vector<double> features(8, 0.0);
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // 1. Typing Velocity (normalized)
    features[0] = CalculateTypingVelocity();
    
    // 2. Keystroke Variance
    features[1] = CalculateKeystrokeVariance();
    
    // 3. Backspace Rate
    features[2] = CalculateBackspaceRate();
    
    // 4. RED Metric (using improved calculation)
    features[3] = CalculateREDMetric();
    
    // 5. Compile Error Rate
    if (m_currentSession.totalCompiles > 0)
        features[4] = static_cast<double>(m_currentSession.failedCompiles) / 
                     m_currentSession.totalCompiles;
    
    // 6. Focus Switches
    features[5] = CalculateFocusSwitches();
    
    // 7. Idle to Active Ratio
    features[6] = CalculateIdleRatio();
    
    // 8. Undo/Redo Attempts
    features[7] = CalculateUndoRedoRate();
    
    return features;
}

double EventMonitor::CalculateTypingVelocity() const
{
    if (m_currentSession.keystrokes.size() < 10)
        return 1.0; // Default
    
    wxTimeSpan totalTime = m_currentSession.lastActivity - m_currentSession.sessionStart;
    double minutes = totalTime.GetSeconds().ToDouble() / 60.0;
    
    if (minutes <= 0)
        return 1.0;
    
    double wpm = (m_currentSession.totalKeystrokes / 5.0) / minutes;
    
    // Normalize to baseline
    if (s_hasBaseline)
    {
        double baselineMinutes = (s_baselineData.lastActivity - s_baselineData.sessionStart).GetSeconds().ToDouble() / 60.0;
        if (baselineMinutes > 0)
        {
            double baselineWPM = (s_baselineData.totalKeystrokes / 5.0) / baselineMinutes;
            if (baselineWPM > 0)
                return wpm / baselineWPM;
        }
    }
    
    // Use real-time WPM if available
    if (m_currentSession.realTimeWPM > 0)
    {
        return m_currentSession.realTimeWPM / 40.0; // Normalize to 40 WPM
    }
    
    return wpm / 40.0; // Normalize to average 40 WPM
}

double EventMonitor::CalculateKeystrokeVariance() const
{
    if (m_currentSession.keystrokes.size() < 10)
        return 0.5;
    
    // Use rolling window for variance calculation
    const auto& window = m_currentSession.rollingKeystrokes;
    if (window.size() < 5)
        return 0.5;
    
    std::vector<double> intervals;
    wxDateTime lastTime = window[0].timestamp;
    
    for (size_t i = 1; i < window.size(); ++i)
    {
        double interval = (window[i].timestamp - lastTime).GetMilliseconds().ToDouble();
        if (interval > 0 && interval < 2000) // Ignore pauses > 2s for variance
            intervals.push_back(interval);
        lastTime = window[i].timestamp;
    }
    
    if (intervals.size() < 5)
        return 0.5;
    
    // Calculate coefficient of variation
    double sum = std::accumulate(intervals.begin(), intervals.end(), 0.0);
    double mean = sum / intervals.size();
    
    if (mean <= 0)
        return 0.5;
    
    double sq_sum = 0.0;
    for (double val : intervals)
        sq_sum += (val - mean) * (val - mean);
    double stddev = std::sqrt(sq_sum / intervals.size());
    
    return stddev / mean;
}

double EventMonitor::CalculateBackspaceRate() const
{
    if (m_currentSession.totalKeystrokes < 10)
        return 0.0;
    
    // Use real-time backspace rate if available
    if (m_currentSession.realTimeBackspaceRate > 0)
        return m_currentSession.realTimeBackspaceRate;
    
    return static_cast<double>(m_currentSession.totalBackspaces) / 
           m_currentSession.totalKeystrokes;
}

double EventMonitor::CalculateREDMetric() const
{
    if (m_currentSession.errorSequence.size() < 2)
        return 0.0;
    
    // Calculate repeated error density
    return static_cast<double>(m_currentSession.repeatedErrors) / 
           m_currentSession.errorSequence.size() * 10.0;
}

double EventMonitor::CalculateFocusSwitches() const
{
    if (m_currentSession.keystrokes.size() < 5)
        return 0.0;
    
    int switches = 0;
    wxDateTime lastTime = m_currentSession.keystrokes[0].timestamp;
    
    for (size_t i = 1; i < m_currentSession.keystrokes.size(); ++i)
    {
        double gap = (m_currentSession.keystrokes[i].timestamp - lastTime).GetSeconds().ToDouble();
        if (gap > 30.0) // More than 30 seconds idle
            switches++;
        lastTime = m_currentSession.keystrokes[i].timestamp;
    }
    
    return switches;
}

double EventMonitor::CalculateIdleRatio() const
{
    if (m_currentSession.keystrokes.size() < 5)
        return 0.0;
    
    double totalTime = (m_currentSession.lastActivity - m_currentSession.sessionStart).GetSeconds().ToDouble();
    if (totalTime <= 0)
        return 0.0;
    
    double idleTime = 0.0;
    wxDateTime lastTime = m_currentSession.keystrokes[0].timestamp;
    
    for (size_t i = 1; i < m_currentSession.keystrokes.size(); ++i)
    {
        double gap = (m_currentSession.keystrokes[i].timestamp - lastTime).GetSeconds().ToDouble();
        if (gap > 5.0) // Idle if gap > 5 seconds
            idleTime += gap;
        lastTime = m_currentSession.keystrokes[i].timestamp;
    }
    
    return idleTime / totalTime;
}

double EventMonitor::CalculateUndoRedoRate() const
{
    // Approximate undo/redo by rapid backspace sequences
    if (m_currentSession.keystrokes.size() < 10)
        return 0.0;
    
    int undoSequences = 0;
    int backspaceRun = 0;
    
    for (const auto& ks : m_currentSession.rollingKeystrokes)
    {
        if (ks.isBackspace)
        {
            backspaceRun++;
            if (backspaceRun > 3) // Multiple consecutive backspaces
                undoSequences++;
        }
        else
        {
            backspaceRun = 0;
        }
    }
    
    return static_cast<double>(undoSequences) / m_currentSession.rollingKeystrokes.size() * 10.0;
}
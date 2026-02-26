#include "PythonBridge.h"
#include "InterventionManager.h"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/log.h>
#include <windows.h>
#include <sstream>

// wxJSON - include if available; otherwise replace with simple JSON string building
// If your build environment has wxJSON, ensure it is in the include path.
// #include <wx/jsonwriter.h>
// #include <wx/jsonreader.h>

PythonBridge::PythonBridge()
    : m_process(nullptr)
    , m_isRunning(false)
    , m_pipeHandle(INVALID_HANDLE_VALUE)
{
}

PythonBridge::~PythonBridge()
{
    StopPythonService();
}

bool PythonBridge::StartPythonService(const wxString& pythonPath, const wxString& modelPath)
{
    if (m_isRunning)
        return true;
    
    // Ensure model files exist
    wxFileName modelFile(modelPath);
    if (!modelFile.Exists())
    {
        wxLogError(_T("Model file not found: %s"), modelPath);
        return false;
    }
    
    // Get the directory containing models
    wxString modelDir = modelFile.GetPath();
    
    // Path to Python script (look next to executable)
    wxString pluginDir = wxFileName::DirName(
        wxStandardPaths::Get().GetPluginsDir()).GetPath();
    wxString scriptPath = pluginDir + wxFILE_SEP_PATH + 
                         _T("python") + wxFILE_SEP_PATH + 
                         _T("anxiety_detector.py");
    
    if (!wxFileName::FileExists(scriptPath))
    {
        wxLogError(_T("Python script not found: %s"), scriptPath);
        return false;
    }
    
    // Create process to run Python service
    wxString cmd = wxString::Format(_T("%s \"%s\" \"%s\""), 
        pythonPath, scriptPath, modelDir);
    
    wxLogDebug(_T("Starting Python service: %s"), cmd);
    
    m_process = new wxProcess(this);
    m_process->Redirect();
    
    long pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, m_process);
    
    if (pid <= 0)
    {
        wxLogError(_T("Failed to start Python process"));
        delete m_process;
        m_process = nullptr;
        return false;
    }
    
    // Wait for pipe to be created
    Sleep(2000);
    
    // Connect to named pipe
    if (!ConnectToPipe())
    {
        wxLogError(_T("Failed to connect to Python service pipe"));
        StopPythonService();
        return false;
    }
    
    // Initialize detector
    if (!SendInitialize(modelDir))
    {
        wxLogError(_T("Failed to initialize detector"));
        StopPythonService();
        return false;
    }
    
    m_isRunning = true;
    wxLogMessage(_T("Python anxiety detection service started"));
    
    return true;
}

bool PythonBridge::ConnectToPipe()
{
    const wxString pipeName = _T("\\\\.\\pipe\\AnxietyDetector");
    int attempts = 0;
    
    while (attempts < 30) // Try for 30 seconds
    {
        m_pipeHandle = CreateFile(
            pipeName.wc_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        
        if (m_pipeHandle != INVALID_HANDLE_VALUE)
        {
            DWORD pipeMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
            if (SetNamedPipeHandleState(m_pipeHandle, &pipeMode, NULL, NULL))
                return true;
            else
            {
                CloseHandle(m_pipeHandle);
                m_pipeHandle = INVALID_HANDLE_VALUE;
            }
        }
        
        attempts++;
        Sleep(1000);
    }
    
    return false;
}

// Helper: build a simple JSON string without a library
static std::string BuildFeaturesJSON(const wxString& type, const std::vector<double>& features)
{
    std::ostringstream oss;
    oss << "{\"type\":\"" << type.ToUTF8() << "\",\"features\":[";
    for (size_t i = 0; i < features.size(); ++i)
    {
        if (i > 0) oss << ",";
        oss << features[i];
    }
    oss << "]}";
    return oss.str();
}

static std::string BuildSimpleJSON(const wxString& type)
{
    std::ostringstream oss;
    oss << "{\"type\":\"" << type.ToUTF8() << "\"}";
    return oss.str();
}

static std::string BuildInitJSON(const wxString& modelDir)
{
    std::ostringstream oss;
    // Escape backslashes in Windows paths
    wxString escaped = modelDir;
    escaped.Replace(_T("\\"), _T("\\\\"));
    oss << "{\"type\":\"initialize\",\"model_dir\":\"" << escaped.ToUTF8() << "\"}";
    return oss.str();
}

bool PythonBridge::SendInitialize(const wxString& modelDir)
{
    std::string msg = BuildInitJSON(modelDir);
    
    if (m_pipeHandle == INVALID_HANDLE_VALUE)
        return false;
    
    DWORD bytesWritten;
    BOOL success = WriteFile(
        m_pipeHandle,
        msg.c_str(),
        (DWORD)msg.length(),
        &bytesWritten,
        NULL
    );
    
    return (success && bytesWritten == msg.length());
}

bool PythonBridge::SendMessage(const wxJSONValue& message)
{
    // This function is kept for API compatibility but wxJSONValue is forward-declared only.
    // Actual sending is done via the helper functions above.
    (void)message;
    wxLogWarning(_T("SendMessage(wxJSONValue) called but wxJSON not linked; use SendFeatures instead."));
    return false;
}

bool PythonBridge::ReceiveMessage(wxJSONValue& message)
{
    (void)message;
    wxLogWarning(_T("ReceiveMessage(wxJSONValue) called but wxJSON not linked."));
    return false;
}

PredictionResult PythonBridge::AnalyzeFeatures(const std::vector<double>& features)
{
    PredictionResult result;
    result.level = AnxietyLevel::LOW;
    result.confidence = 0.0;
    result.shouldIntervene = false;
    
    if (!m_isRunning || m_pipeHandle == INVALID_HANDLE_VALUE)
        return result;
    
    // Send analyze request
    std::string msg = BuildFeaturesJSON(_T("analyze"), features);
    DWORD bytesWritten;
    BOOL writeOk = WriteFile(m_pipeHandle, msg.c_str(), (DWORD)msg.length(), &bytesWritten, NULL);
    
    if (!writeOk || bytesWritten != msg.length())
    {
        wxLogError(_T("Failed to write features to pipe"));
        return result;
    }
    
    // Read response
    char buffer[65536];
    DWORD bytesRead = 0;
    BOOL readOk = ReadFile(m_pipeHandle, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
    
    if (!readOk || bytesRead == 0)
        return result;
    
    buffer[bytesRead] = '\0';
    wxString response = wxString::FromUTF8(buffer);
    
    // Simple JSON parsing without a library
    // Expected format: {"status":"ok","prediction":{"level":"High","confidence":0.85,...},"should_intervene":true}
    if (response.Contains(_T("\"status\":\"ok\"")))
    {
        // Parse level
        if (response.Contains(_T("\"level\":\"Low\"")))
            result.level = AnxietyLevel::LOW;
        else if (response.Contains(_T("\"level\":\"Moderate\"")))
            result.level = AnxietyLevel::MODERATE;
        else if (response.Contains(_T("\"level\":\"High\"")))
            result.level = AnxietyLevel::HIGH;
        else if (response.Contains(_T("\"level\":\"Extreme\"")))
            result.level = AnxietyLevel::EXTREME;
        
        // Parse should_intervene
        result.shouldIntervene = response.Contains(_T("\"should_intervene\":true"));
        
        // Parse confidence (simple extraction)
        int confPos = response.Find(_T("\"confidence\":"));
        if (confPos != wxNOT_FOUND)
        {
            wxString confStr = response.Mid(confPos + 13);
            confStr = confStr.BeforeFirst(',');
            confStr = confStr.BeforeFirst('}');
            confStr.ToDouble(&result.confidence);
        }
        
        // Parse triggered_features
        int featPos = response.Find(_T("\"triggered_features\":\""));
        if (featPos != wxNOT_FOUND)
        {
            wxString featStr = response.Mid(featPos + 22);
            result.triggeredFeatures = featStr.BeforeFirst('"');
        }
        
        result.timestamp = wxDateTime::Now();
    }
    
    return result;
}

bool PythonBridge::SendFeatures(const std::vector<double>& features)
{
    auto result = AnalyzeFeatures(features);
    
    if (result.shouldIntervene && m_callback)
        m_callback(result);
    
    return result.shouldIntervene;
}

void PythonBridge::StopPythonService()
{
    if (m_pipeHandle != INVALID_HANDLE_VALUE)
    {
        // Send shutdown message
        std::string shutdownMsg = BuildSimpleJSON(_T("shutdown"));
        DWORD bytesWritten;
        WriteFile(m_pipeHandle, shutdownMsg.c_str(), (DWORD)shutdownMsg.length(), &bytesWritten, NULL);
        
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
    
    if (m_process)
    {
        wxProcess::Kill(m_process->GetPid(), wxSIGTERM);
        m_process = nullptr;
    }
    
    m_isRunning = false;
    wxLogMessage(_T("Python anxiety detection service stopped"));
}

void PythonBridge::OnTerminate(int pid, int status)
{
    (void)pid;
    (void)status;
    m_isRunning = false;
    wxLogWarning(_T("Python service terminated unexpectedly"));
    
    if (m_pipeHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
}
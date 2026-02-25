#include "PythonBridge.h"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <windows.h>
#include <sstream>

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
        Manager::Get()->GetLogManager()->LogError(
            wxString::Format(_T("Model file not found: %s"), modelPath));
        return false;
    }
    
    // Get the directory containing models
    wxString modelDir = modelFile.GetPath();
    
    // Path to Python script
    wxString pluginDir = wxFileName::DirName(
        wxStandardPaths::Get().GetPluginsDir()).GetPath();
    wxString scriptPath = pluginDir + wxFILE_SEP_PATH + 
                         _T("python") + wxFILE_SEP_PATH + 
                         _T("anxiety_detector.py");
    
    if (!wxFileName::FileExists(scriptPath))
    {
        Manager::Get()->GetLogManager()->LogError(
            wxString::Format(_T("Python script not found: %s"), scriptPath));
        return false;
    }
    
    // Create process to run Python service
    wxString cmd = wxString::Format(_T("%s \"%s\" \"%s\""), 
        pythonPath, scriptPath, modelDir);
    
    Manager::Get()->GetLogManager()->LogDebug(
        wxString::Format(_T("Starting Python service: %s"), cmd));
    
    m_process = new wxProcess(this);
    m_process->Redirect();
    
    long pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, m_process);
    
    if (pid <= 0)
    {
        Manager::Get()->GetLogManager()->LogError(_T("Failed to start Python process"));
        delete m_process;
        m_process = nullptr;
        return false;
    }
    
    // Wait for pipe to be created
    Sleep(2000);
    
    // Connect to named pipe
    if (!ConnectToPipe())
    {
        Manager::Get()->GetLogManager()->LogError(_T("Failed to connect to Python service pipe"));
        StopPythonService();
        return false;
    }
    
    // Initialize detector
    if (!SendInitialize(modelDir))
    {
        Manager::Get()->GetLogManager()->LogError(_T("Failed to initialize detector"));
        StopPythonService();
        return false;
    }
    
    m_isRunning = true;
    Manager::Get()->GetLogManager()->Log(_T("Python anxiety detection service started"));
    
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

bool PythonBridge::SendInitialize(const wxString& modelDir)
{
    wxJSONValue initMsg;
    initMsg[_T("type")] = _T("initialize");
    initMsg[_T("model_dir")] = modelDir;
    
    return SendMessage(initMsg);
}

bool PythonBridge::SendMessage(const wxJSONValue& message)
{
    if (m_pipeHandle == INVALID_HANDLE_VALUE)
        return false;
    
    wxJSONWriter writer;
    wxString jsonStr;
    writer.Write(message, jsonStr);
    
    std::string utf8Str = jsonStr.ToUTF8();
    DWORD bytesWritten;
    
    BOOL success = WriteFile(
        m_pipeHandle,
        utf8Str.c_str(),
        utf8Str.length(),
        &bytesWritten,
        NULL
    );
    
    if (!success || bytesWritten != utf8Str.length())
    {
        Manager::Get()->GetLogManager()->LogError(_T("Failed to write to pipe"));
        return false;
    }
    
    return true;
}

bool PythonBridge::ReceiveMessage(wxJSONValue& message)
{
    if (m_pipeHandle == INVALID_HANDLE_VALUE)
        return false;
    
    char buffer[65536];
    DWORD bytesRead;
    
    BOOL success = ReadFile(
        m_pipeHandle,
        buffer,
        sizeof(buffer) - 1,
        &bytesRead,
        NULL
    );
    
    if (!success || bytesRead == 0)
        return false;
    
    buffer[bytesRead] = '\0';
    wxString jsonStr = wxString::FromUTF8(buffer);
    
    wxJSONReader reader;
    reader.Parse(jsonStr, &message);
    
    return true;
}

PredictionResult PythonBridge::AnalyzeFeatures(const std::vector<double>& features)
{
    PredictionResult result;
    result.level = AnxietyLevel::LOW;
    result.confidence = 0.0;
    
    if (!m_isRunning)
        return result;
    
    wxJSONValue msg;
    msg[_T("type")] = _T("analyze");
    
    // Convert features to JSON array
    wxJSONValue featuresArray(wxJSONTYPE_ARRAY);
    for (size_t i = 0; i < features.size(); i++)
    {
        featuresArray.Append(features[i]);
    }
    msg[_T("features")] = featuresArray;
    
    if (!SendMessage(msg))
        return result;
    
    wxJSONValue response;
    if (!ReceiveMessage(response))
        return result;
    
    // Parse response
    if (response[_T("status")].AsString() == _T("ok"))
    {
        const wxJSONValue& pred = response[_T("prediction")];
        
        wxString level = pred[_T("level")].AsString();
        if (level == _T("Low")) result.level = AnxietyLevel::LOW;
        else if (level == _T("Moderate")) result.level = AnxietyLevel::MODERATE;
        else if (level == _T("High")) result.level = AnxietyLevel::HIGH;
        else if (level == _T("Extreme")) result.level = AnxietyLevel::EXTREME;
        
        result.confidence = pred[_T("confidence")].AsDouble();
        result.triggeredFeatures = pred[_T("triggered_features")].AsString();
        result.timestamp = wxDateTime::Now();
        
        result.shouldIntervene = response[_T("should_intervene")].AsBool();
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
        wxJSONValue msg;
        msg[_T("type")] = _T("shutdown");
        SendMessage(msg);
        
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
    
    if (m_process)
    {
        wxProcess::Kill(m_process->GetPid(), wxSIGTERM);
        m_process = nullptr;
    }
    
    m_isRunning = false;
    Manager::Get()->GetLogManager()->Log(_T("Python anxiety detection service stopped"));
}

void PythonBridge::OnTerminate(int pid, int status)
{
    m_isRunning = false;
    Manager::Get()->GetLogManager()->LogWarning(_T("Python service terminated unexpectedly"));
    
    if (m_pipeHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
}
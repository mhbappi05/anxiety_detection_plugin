#ifndef PYTHON_BRIDGE_H
#define PYTHON_BRIDGE_H

#include <wx/string.h>
#include <wx/process.h>
#include <wx/jsonval.h>
#include <wx/jsonwriter.h>
#include <wx/jsonreader.h>
#include <atomic>
#include <memory>
#include <functional>
#include <windows.h>

enum class AnxietyLevel
{
    LOW,
    MODERATE,
    HIGH,
    EXTREME
};

struct PredictionResult
{
    AnxietyLevel level;
    double confidence;
    wxString triggeredFeatures;
    wxDateTime timestamp;
    bool shouldIntervene;
};

class PythonBridge : public wxProcess
{
    public:
        PythonBridge();
        virtual ~PythonBridge();

        bool StartPythonService(const wxString& pythonPath, const wxString& modelPath);
        void StopPythonService();
        bool IsRunning() const { return m_isRunning; }

        PredictionResult AnalyzeFeatures(const std::vector<double>& features);
        bool SendFeatures(const std::vector<double>& features);
        
        void SetCallback(std::function<void(const PredictionResult&)> callback) 
        { m_callback = callback; }
        
        // wxProcess overrides
        virtual void OnTerminate(int pid, int status) override;

    private:
        wxProcess* m_process;
        HANDLE m_pipeHandle;
        std::atomic<bool> m_isRunning;
        std::function<void(const PredictionResult&)> m_callback;
        
        bool ConnectToPipe();
        bool SendInitialize(const wxString& modelDir);
        bool SendMessage(const wxJSONValue& message);
        bool ReceiveMessage(wxJSONValue& message);
};

#endif
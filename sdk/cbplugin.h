#ifndef CBPLUGIN_H
#define CBPLUGIN_H

/**
 * Minimal Code::Blocks plugin SDK stub.
 * Provides just enough of the cbPlugin interface for the DLL to export the
 * three symbols Code::Blocks requires:
 *   CreatePlugin, FreePlugin, GetPluginSDKVersion
 *
 * Real SDK headers are not shipped with the CB installer; these stubs let us
 * compile a fully-compatible plugin DLL without the full CB source tree.
 */

#include <wx/event.h>
#include <wx/string.h>
#include <wx/menu.h>
#include <wx/toolbar.h>

// ---------------------------------------------------------------------------
// DLL export macro (matches what CB itself uses)
// ---------------------------------------------------------------------------
#ifndef DLL_EXPORT
#  ifdef __WXMSW__
#    define DLL_EXPORT __declspec(dllexport)
#  else
#    define DLL_EXPORT __attribute__((visibility("default")))
#  endif
#endif

// ---------------------------------------------------------------------------
// cbPlugin — minimal base class
// ---------------------------------------------------------------------------
class cbPlugin : public wxEvtHandler
{
public:
    cbPlugin() : m_IsAttached(false) {}
    virtual ~cbPlugin() {}

    virtual void OnAttach() {}
    virtual void OnRelease(bool appShutDown) {}
    virtual void BuildMenu(wxMenuBar* menuBar) {}
    virtual bool BuildToolBar(wxToolBar* toolBar) { return false; }

    bool IsAttached() const { return m_IsAttached; }

protected:
    bool m_IsAttached;
};

// ---------------------------------------------------------------------------
// PluginRegistrant<T>
//
// Instantiating this template (in an anonymous namespace in a .cpp file)
// causes the three required C exports to be linked into the DLL.
// ---------------------------------------------------------------------------
template<class T>
class PluginRegistrant
{
public:
    PluginRegistrant(const wxString& name) : m_name(name) {}

    // These static functions are exposed as C symbols below.
    static cbPlugin* CreatePlugin_impl()  { return new T(); }
    static void      FreePlugin_impl(cbPlugin* p) { delete p; }
    static int       GetSDKVersion_impl() { return 2;  }   // SDK major version

private:
    wxString m_name;
};

// ---------------------------------------------------------------------------
// The three C exports Code::Blocks' plugin-loader looks for.
//
// We define them as weak, so that exactly ONE translation unit (the one that
// instantiates PluginRegistrant<T>) provides the real bodies via a linker
// alias trick — but the simplest portable approach for MinGW is to put the
// real definitions right here inside the template specialization.  Because
// every plugin includes this header in exactly one .cpp that contains the
// PluginRegistrant<> instantiation, the linker sees the symbols once.
// ---------------------------------------------------------------------------
extern "C"
{
    // Declared but NOT defined here — the definitions come from the macro below.
    DLL_EXPORT cbPlugin* CreatePlugin();
    DLL_EXPORT void      FreePlugin(cbPlugin* plugin);
    DLL_EXPORT int       GetPluginSDKVersion();
}

// ---------------------------------------------------------------------------
// CB_PLUGIN_REGISTRANT(ClassName)
//
// Place this macro ONCE in the .cpp file that registers the plugin.
// It provides the three C-linkage function bodies and instantiates the
// PluginRegistrant<> object.
// ---------------------------------------------------------------------------
#define CB_PLUGIN_REGISTRANT(T)                              \
    namespace {                                              \
        PluginRegistrant<T> _reg_##T(wxT(#T));              \
    }                                                        \
    extern "C" {                                             \
        DLL_EXPORT cbPlugin* CreatePlugin()                  \
            { return new T(); }                              \
        DLL_EXPORT void FreePlugin(cbPlugin* plugin)         \
            { delete plugin; }                               \
        DLL_EXPORT int GetPluginSDKVersion()                 \
            { return 2; }                                    \
    }

#endif // CBPLUGIN_H

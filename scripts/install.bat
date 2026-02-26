@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Anxiety Detection Plugin Installer
echo for Code::Blocks IDE
echo ========================================
echo.

:: Default Code::Blocks installation paths
set "CB_PATHS[0]=C:\Program Files\CodeBlocks"
set "CB_PATHS[1]=C:\Program Files (x86)\CodeBlocks"
set "CB_PATHS[2]=%USERPROFILE%\AppData\Local\Programs\CodeBlocks"
set "CB_PATHS[3]=%USERPROFILE%\CodeBlocks"

:: Find Code::Blocks installation
set "CB_PATH="
for %%p in (0 1 2 3) do (
    if exist "!CB_PATHS[%%p]!\codeblocks.exe" (
        set "CB_PATH=!CB_PATHS[%%p]!"
        echo Found Code::Blocks at: !CB_PATH!
        goto :found
    )
)

:not_found
echo Could not find Code::Blocks installation.
echo Please enter the path to your Code::Blocks installation:
set /p CB_PATH="Path: "
if not exist "!CB_PATH!\codeblocks.exe" (
    echo Invalid path. Please run again with correct path.
    pause
    exit /b 1
)

:found
echo.
echo Installing plugin to: %CB_PATH%
echo.

:: Set paths
set "PLUGIN_DIR=%CB_PATH%\share\CodeBlocks\plugins"
set "PYTHON_DIR=%CB_PATH%\share\CodeBlocks\python"
set "MODELS_DIR=%CB_PATH%\share\CodeBlocks\anxiety_models"
set "CONFIG_DIR=%APPDATA%\CodeBlocks"

:: Create directories
echo Creating directories...
if not exist "%PLUGIN_DIR%" mkdir "%PLUGIN_DIR%"
if not exist "%PYTHON_DIR%" mkdir "%PYTHON_DIR%"
if not exist "%MODELS_DIR%" mkdir "%MODELS_DIR%"
if not exist "%CONFIG_DIR%" mkdir "%CONFIG_DIR%"

:: Copy plugin DLL
echo.
echo Copying plugin files...
if exist "..\anxiety_plugin.dll" (
    copy "..\anxiety_plugin.dll" "%PLUGIN_DIR%\" /Y
    echo [OK] Plugin DLL copied
) else (
    echo [WARNING] Plugin DLL not found. Please build the plugin first by running build_simple.bat
)

:: Copy Python files
echo.
echo Copying Python backend...
if exist "..\python\*.py" (
    copy "..\python\*.py" "%PYTHON_DIR%\" /Y
    echo [OK] Python files copied
) else (
    echo [WARNING] Python files not found
)

:: Copy model files
echo.
echo Copying ML models...
if exist "..\models\*.pkl" (
    copy "..\models\*.pkl" "%MODELS_DIR%\" /Y
    echo [OK] Model files copied
) else (
    echo [WARNING] Model files not found. Please place your trained models in the models folder.
)

:: Copy configuration
echo.
echo Copying configuration...
if exist "..\config\config.xml" (
    copy "..\config\config.xml" "%CONFIG_DIR%\anxiety_plugin_config.xml" /Y
    echo [OK] Configuration copied
) else (
    echo [WARNING] Config file not found, creating default...
    (
        echo ^<?xml version="1.0" encoding="UTF-8" ?^>
        echo ^<AnxietyPlugin^>
        echo     ^<Settings^>
        echo         ^<PythonPath^>python^</PythonPath^>
        echo         ^<ModelPath^>%MODELS_DIR%^</ModelPath^>
        echo         ^<AnxietyThreshold^>0.7^</AnxietyThreshold^>
        echo         ^<InterventionCooldown^>300^</InterventionCooldown^>
        echo         ^<EnableC^>true^</EnableC^>
        echo         ^<EnableCpp^>true^</EnableCpp^>
        echo         ^<AutoCalibrate^>true^</AutoCalibrate^>
        echo         ^<ShowNotifications^>true^</ShowNotifications^>
        echo     ^</Settings^>
        echo ^</AnxietyPlugin^>
    ) > "%CONFIG_DIR%\anxiety_plugin_config.xml"
)

:: Copy manifest
echo.
echo Copying plugin manifest...
if exist "..\resources\manifest.xml" (
    copy "..\resources\manifest.xml" "%PLUGIN_DIR%\anxiety_plugin.xml" /Y
    echo [OK] Manifest copied
)

:: Check Python installation
echo.
echo Checking Python installation...
python --version >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Python found
    echo Installing required Python packages...
    pip install numpy pandas scikit-learn joblib pywin32
) else (
    echo [WARNING] Python not found in PATH. Please install Python 3.10+ and required packages:
    echo pip install numpy pandas scikit-learn joblib pywin32
)

:: Create uninstall script
echo.
echo Creating uninstall script...
(
    echo @echo off
    echo echo Uninstalling Anxiety Detection Plugin...
    echo.
    echo del "%PLUGIN_DIR%\anxiety_plugin.dll" 2^>nul
    echo del "%PLUGIN_DIR%\anxiety_plugin.xml" 2^>nul
    echo rmdir /s /q "%PYTHON_DIR%" 2^>nul
    echo rmdir /s /q "%MODELS_DIR%" 2^>nul
    echo del "%CONFIG_DIR%\anxiety_plugin_config.xml" 2^>nul
    echo.
    echo echo Uninstall complete.
    echo pause
) > "%CB_PATH%\uninstall_anxiety_plugin.bat"

echo.
echo ========================================
echo Installation Complete!
echo ========================================
echo.
echo Next steps:
echo 1. Place your trained model files in: %MODELS_DIR%
echo    Required files:
echo      - best_anxiety_model.pkl
echo      - label_encoder.pkl
echo      - scaler.pkl
echo.
echo 2. Make sure Python 3.10+ is installed and in PATH
echo.
echo 3. Restart Code::Blocks
echo.
echo 4. The plugin will be available under Tools -> Anxiety Detection
echo.
echo To uninstall, run: %CB_PATH%\uninstall_anxiety_plugin.bat
echo.
pause
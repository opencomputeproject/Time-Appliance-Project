
cls
:: Check if the first argument is "start"
if "%1"=="compile" (
    echo Starting compile
	call :compile
)

if "%1"=="compilew" (
    echo Starting compile and waiting
	call :compile_wait
)

if "%1"=="compileu" (
    echo Starting compile and upload
	call :compile_upload
)

if "%1"=="upload" (
    echo Starting upload only
	call :upload
)


if "%1"=="full" (
    echo Starting compile, upload, and full run
	call :compile_upload_fullrun
)

if "%1"=="urun" (
    echo Starting compile, upload, and full run
	call :upload_fullrun
)

:: Default case if no valid argument is provided
if "%1"=="" (
    echo No argument provided. Please use "compile", "compilew", "compileu", "upload", or "full".
	echo Running with full by default 
	call :compile_upload_fullrun
)




:: end of script
EXIT /B %ERRORLEVEL% 




:compile
:: COMPILE 
:: arduino-cli compile -b arduino:mbed_giga:giga RCB_WWVB.ino --clean
arduino-cli compile -b arduino:mbed_giga:giga RCB_WWVB.ino
EXIT /B 0

:compile_wait
call :compile
pause
EXIT /B 0

:upload
:: UPLOAD
::for /f "tokens=2 delims==" %%A in ('wmic path Win32_SerialPort get DeviceID /value') do (
::    arduino-cli upload -p %%A -b arduino:mbed_giga:giga
::)
arduino-cli upload -p COM3 -b arduino:mbed_giga:giga
arduino-cli upload -p COM5 -b arduino:mbed_giga:giga
EXIT /B 0

:compile_upload
call :compile
call :upload
EXIT /B 0


:fullrun
:: Delete all log files so tera term log is fresh 
del "C:\Julians\Projects\RCB WWVB\Logs\masterAnchor.txt"
del "C:\Julians\Projects\RCB WWVB\Logs\clientAnchor0.txt"
start "" ttermpro /C=3 /L="C:\Julians\Projects\RCB WWVB\Logs\masterAnchor.txt" ^
	/M="C:\Julians\Projects\RCB WWVB\Logs\master_wiwi.ttl"
start "" ttermpro /C=5 /L="C:\Julians\Projects\RCB WWVB\Logs\clientAnchor0.txt" ^
	/M="C:\Julians\Projects\RCB WWVB\Logs\clientAnchor0_wiwi.ttl"
start "" python3 "C:\Julians\Projects\RCB WWVB\Logs\wiwi_live_monitor.py"
	
	

:: Wait for all Tera Term processes to complete
echo Waiting for Tera Term sessions to complete...
:WAIT
tasklist /FI "IMAGENAME eq ttermpro.exe" 2>NUL | find /I "ttermpro.exe" >NUL
if %ERRORLEVEL% equ 0 (
    timeout /t 2 /nobreak
    goto WAIT
)

start "" notepad "C:\Julians\Projects\RCB WWVB\Logs\clientAnchor0.txt"
:: Once you close tera term, open the log file in notepad
notepad "C:\Julians\Projects\RCB WWVB\Logs\masterAnchor.txt"

EXIT /B 0


:upload_fullrun
call :upload
call :fullrun
EXIT /B 0 



:compile_upload_fullrun
call :compile_upload
call :fullrun
EXIT /B 0



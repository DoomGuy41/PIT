#include "HPIA.h"
#include "PowerShellUtils.h"
void RunHPIA()
{
	RunPowerShellAsync(
        //When updating HPIA link, do not forget to change the folder where it install - c:/SWSetup/spXXXXX
        L"Invoke-WebRequest -Uri 'https://hpia.hpcloud.hp.com/downloads/hpia/hp-hpia-5.3.3.exe' "
        L"-OutFile $env:TEMP\\hpia.exe; "
        L"Start-Process $env:TEMP\\hpia.exe -ArgumentList '-s -e' -Wait; "
        L"Start-Process 'C:\\SWSetup\\sp165759\\HPImageAssistant.exe' "
        L"-ArgumentList '/Operation:Analyze /Category:All /selection:All /action:install /silent'"

	);
}
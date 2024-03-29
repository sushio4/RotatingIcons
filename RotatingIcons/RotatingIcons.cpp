#define _USE_MATH_DEFINES

#include <windows.h>
#include <commctrl.h>
#include <math.h>
#include <chrono>
#include <fstream>
#include <string>
#include <iomanip>

void LogMessage(std::string message, bool closeProgram = false)
{
	using namespace std::chrono;

	std::ofstream LogFile("RotatingIcons_Log.txt", std::ios::app);

	if (!LogFile.is_open())
		LogFile.open("RotatingIcons_Log.txt", std::ios::out);

	if (!LogFile.is_open())
		exit(0x0BADF11E);

	auto now = time_point<system_clock>( system_clock::now() );
	auto in_time_t = system_clock::to_time_t(now);

	tm time;
	localtime_s(&time, &in_time_t);

	LogFile << std::put_time(&time, "[%d.%m.%Y - %X]: ");
	LogFile << message << std::endl;

	if (closeProgram)
	{
		LogFile << "[Program terminated]" << std::endl;
		LogFile.close();
		exit(-1);
	}
	LogFile.close();
}

void GetDesktopResolution(int& horizontal, int& vertical)
{
	RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

auto getTime() -> decltype(auto)
{ 
	return std::chrono::high_resolution_clock::now(); 
}

int main()
{
	HWND hd;

	HANDLE he;
	DWORD Pi;

	//getting all the important stuff
	hd = FindWindowA("Progman", NULL);
	hd = FindWindowEx(hd, 0, L"SHELLDLL_DefView", NULL);
	hd = FindWindowEx(hd, 0, L"SysListView32", NULL);

	GetWindowThreadProcessId(hd, &Pi);
	he = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, Pi);

	POINT* pC = (POINT*)VirtualAllocEx(he, NULL, sizeof(POINT), MEM_COMMIT, PAGE_READWRITE);

	if (pC == nullptr)
		LogMessage("Couldn't allocate memory to SysListView32", true);

	WriteProcessMemory(he, pC, &pC, sizeof(POINT), NULL);

	int count;
	int centerid = 0;
	int centerx, centery;
	const int radius = 240;
	const double omega = 0.0001;

	count = SendMessage(hd, LVM_GETITEMCOUNT, 0, 0);
	if (count < 2) LogMessage("Not enough icons to rotate", true);

	int leastDist = INT_MAX;
	GetDesktopResolution(centerx, centery);
	centerx *= 0.5;
	centery *= 0.5;

	for (int i = 0; i < count; i++)
	{
		POINT pt;
		ListView_GetItemPosition(hd, i, pC);
		ReadProcessMemory(he, pC, &pt, sizeof(POINT), NULL);

		int dist = (pt.x - centerx) * (pt.x - centerx) + (pt.y - centery) * (pt.y - centery);
		if (dist < leastDist)
		{
			leastDist = dist;
			centerid = i;
		}
	}

	auto startTime = getTime();

	for(long int j = 0;;j++)
	{
		//time
		double currentTime = (getTime() - startTime)/std::chrono::milliseconds(1);

		//center
		POINT pt;
		ListView_GetItemPosition(hd, centerid, pC);
		ReadProcessMemory(he, pC, &pt, sizeof(POINT), NULL);
		centerx = pt.x, centery = pt.y;

		for (int i = 0; i < count; i++)
		{
			if (i == centerid) continue;

			int x = centerx + radius * cos(currentTime * omega - 2 * M_PI * (double)(i > centerid ? i - 1 : i) / ((double)count - 1));
			int y = centery + radius * sin(currentTime * omega - 2 * M_PI * (double)(i > centerid ? i - 1 : i) / ((double)count - 1));
			ListView_SetItemPosition(hd, i, x, y);
		}

		if (j % 100 == 0)
			count = SendMessage(hd, LVM_GETITEMCOUNT, 0, 0);
	}

	VirtualFreeEx(he, pC, 0, MEM_RELEASE);
}

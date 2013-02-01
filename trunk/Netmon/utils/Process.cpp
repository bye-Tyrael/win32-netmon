#include "stdafx.h"
#include "Process.h"
#include "ProcessCache.h"
#include "Utils.h"

#pragma region Members of Process

std::vector<Process::ProcessItem> Process::_processes;
HWND Process::_hList;

#pragma endregion

// Initialize process list & ListView control
void Process::Init(HWND hList)
{
	_hList = hList;

	Utils::ListViewInit(_hList, FALSE, 5, 
		TEXT("UID"), TEXT("Process"), TEXT("Tx Rate"), TEXT("Rx Rate"), TEXT("Full Path"), 
		50, 140, 70, 70, 300);

	TCHAR command[256] = TEXT("Select * From Process;");

	SQLiteRow row;
	row.InsertType(SQLiteRow::TYPE_INT32);
	row.InsertType(SQLiteRow::TYPE_STRING);

	SQLite::Select(command, &row, InitCallback);
}

void Process::InitCallback(SQLiteRow *row)
{
	// Create a ProcessItem and Fill
	ProcessItem item;

	RtlZeroMemory(&item, sizeof(item));

	// - active & uid
	item.active = false;
	item.puid = row->GetDataInt32(0);

	// - name & fullPath
	_tcscpy_s(item.name, MAX_PATH, row->GetDataStr(1));
	_tcscpy_s(item.fullPath, MAX_PATH, TEXT("-"));

	// Add Process
	_processes.push_back(item);

	// Add to ListView
	ListViewInsert(_processes.size() - 1, _hList);
}

// ListView operations - insert an item & update an item
void Process::ListViewInsert(int index, HWND hList)
{
	// Prepare Columns
	ProcessItem &item = _processes[index];

	TCHAR szColumn[5][MAX_PATH];

	_stprintf_s(szColumn[0], MAX_PATH, TEXT("%d"), item.puid);
	_stprintf_s(szColumn[1], MAX_PATH, TEXT("%s"), item.name);

	if( item.active )
	{
		_stprintf_s(szColumn[2], MAX_PATH, TEXT("%d"), item.prevTxRate);
		_stprintf_s(szColumn[3], MAX_PATH, TEXT("%d"), item.prevRxRate);
		_tcscpy_s(szColumn[4], MAX_PATH, item.fullPath);
	}
	else
	{
		_tcscpy_s(szColumn[2], MAX_PATH, TEXT("-"));
		_tcscpy_s(szColumn[3], MAX_PATH, TEXT("-"));
		_tcscpy_s(szColumn[4], MAX_PATH, TEXT("-"));
	}

	Utils::ListViewAppend(hList, 5, 
		szColumn[0], szColumn[1], szColumn[2], szColumn[3], szColumn[4]);
}

void Process::ListViewUpdate(int index)
{
	// Prepare Columns
	ProcessItem &item = _processes[index];

	TCHAR szColumn[5][MAX_PATH];

	if( item.active )
	{
		_stprintf_s(szColumn[2], MAX_PATH, TEXT("%d.%d"), item.prevTxRate / 1024, (item.prevTxRate % 1024 + 51) / 108);
		_stprintf_s(szColumn[3], MAX_PATH, TEXT("%d.%d"), item.prevRxRate / 1024, (item.prevRxRate % 1024 + 51) / 108);
		_tcscpy_s(szColumn[4], MAX_PATH, item.fullPath);
	}
	else
	{
		_tcscpy_s(szColumn[2], MAX_PATH, TEXT("-"));
		_tcscpy_s(szColumn[3], MAX_PATH, TEXT("-"));
		_tcscpy_s(szColumn[4], MAX_PATH, TEXT("-"));
	}

	Utils::ListViewUpdate(_hList, index, 5, 
		FALSE, FALSE, TRUE, TRUE, TRUE,
		szColumn[0], szColumn[1], szColumn[2], szColumn[3], szColumn[4]);
}

// Event handlers
void Process::OnPacket(PacketInfoEx *pi)
{
	int index = GetProcessIndex(pi->puid);

	if( index == -1 ) // A new process
	{
		// Insert a ProcessItem
		ProcessItem item;

		RtlZeroMemory(&item, sizeof(item));

		item.active = true;
		item.dirty = false;
		item.pid = pi->pid; // The first pid is logged
		item.puid = pi->puid;

		_tcscpy_s(item.name, MAX_PATH, pi->name);
		_tcscpy_s(item.fullPath, MAX_PATH, pi->fullPath);

		item.txRate = 0;
		item.rxRate = 0;
		item.prevTxRate = 0;
		item.prevRxRate = 0;

		// Process Activity
		item.startTime = (int) time(0);
		item.endTime = -1;
		item.pauid = Utils::InsertProcessActivity(item.puid, item.startTime, item.endTime);
		pi->pauid = item.pauid;

		// Add to process list
		_processes.push_back(item);

		// Add to ListView
		ListViewInsert(_processes.size() - 1, _hList);
	}
	else
	{
		// Update the ProcessItem that already Exists
		ProcessItem &item = _processes[index];

		if( !item.active )
		{
			item.active = true;
			item.pid = pi->pid; // The first pid is logged

			_tcscpy_s(item.fullPath, MAX_PATH, pi->fullPath);

			item.txRate = 0;
			item.rxRate = 0;
			item.prevTxRate = 0;
			item.prevRxRate = 0;

			// Process Activity
			item.startTime = (int) time(0);
			item.endTime = -1;
			item.pauid = Utils::InsertProcessActivity(item.puid, item.startTime, item.endTime);
			pi->pauid = item.pauid;
		}

		if( pi->dir == DIR_UP )
		{
			item.txRate += pi->size;
		}
		else if( pi->dir == DIR_DOWN )
		{
			item.rxRate += pi->size;
		}

		item.dirty = true;
	}
}

void Process::OnTimer()
{
	// See if a Process Ends
	for(unsigned int i = 0; i < _processes.size(); i++)
	{
		ProcessItem &item = _processes[i];
		if( item.active && item.pid != -1 ) // Skip the "Unknown" process
		{
			if( !ProcessCache::instance()->IsProcessAlive(item.pid, item.name))
			{
				Utils::UpdateProcessActivity(item.pauid, (int)time(0));
				item.active = false;
				ListViewUpdate(i);
			}
		}
	}

	// Update those Active if Necessary
	for(unsigned int i = 0; i < _processes.size(); i++)
	{
		ProcessItem &item = _processes[i];

		item.prevTxRate = item.txRate;
		item.prevRxRate = item.rxRate;

		item.txRate = 0;
		item.rxRate = 0;

		if( item.active && item.dirty )
		{
			ListViewUpdate(i);
		}

		if( item.prevRxRate == 0 && item.prevTxRate == 0)
		{
			item.dirty = false;
		}
	}
}

void Process::OnExit()
{
	for(unsigned int i = 0; i < _processes.size(); i++)
	{
		if( _processes[i].active == true )
		{
			Utils::UpdateProcessActivity(_processes[i].pauid, (int)time(0));
		}
	}
}

// Utils
int Process::GetProcessUid(int index)
{
	return _processes[index].puid;
}

int Process::GetProcessCount()
{
	return _processes.size();
}

int Process::GetProcessUid(const TCHAR *name)
{
	for(unsigned int i = 0; i < _processes.size(); i++)
	{
		if( _tcscmp(_processes[i].name, name) == 0 )
		{
			return _processes[i].puid;
		}
	}
	return -1;
}

bool Process::GetProcessName(int puid, TCHAR *buf, int len)
{
	for(unsigned int i = 0; i < _processes.size(); i++)
	{
		if( _processes[i].puid == puid )
		{
			_tcscpy_s(buf, len, _processes[i].name);
			return true;
		}
	}
	return false;
}

int  Process::GetProcessIndex(int puid)
{
	for(unsigned int i = 0; i < _processes.size(); i++)
	{
		if( _processes[i].puid == puid )
		{
			return i;
		}
	}
	return -1;
}

bool Process::GetProcessRate(int puid, int *txRate, int *rxRate)
{
	int index = GetProcessIndex(puid);

	if( index == -1 )
	{
		return false;
	}
	else
	{
		*txRate = _processes[index].prevTxRate;
		*rxRate = _processes[index].prevRxRate;
		return true;
	}
}

bool Process::IsProcessActive(int puid)
{
	for(unsigned int i = 0; i < _processes.size(); i++)
	{
		if( _processes[i].puid == puid )
		{
			return _processes[i].active;
		}
	}
	return false;
}
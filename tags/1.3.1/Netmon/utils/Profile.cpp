#include "stdafx.h"
#include "Profile.h"

void Profile::Init(const TCHAR *szFileName, const TCHAR *szSectionName)
{
	_tcscpy_s(_szFileName, MAX_PATH, szFileName);
	_tcscpy_s(_szSectionName, MAX_PATH, szSectionName);
}

BOOL Profile::GetString(const TCHAR *szOption, TCHAR *buf, int cchLen)
{
	return GetPrivateProfileString(_szSectionName, szOption, 0, buf, cchLen, _szFileName) != 0;
}

BOOL Profile::GetInt(const TCHAR *szOption, int *pValue)
{
	TCHAR buf[256];
	if( GetPrivateProfileString(_szSectionName, szOption, 0, buf, 256, _szFileName) == 0 ) // Key empty or not exist
	{
		return FALSE;
	}
	else // Key not empty
	{
		if( _stscanf_s(buf, TEXT("%d"), pValue) == 1) // Numeric
		{
			return TRUE;
		}
		return FALSE;
	}
}

BOOL Profile::SetString(const TCHAR *szOption, const TCHAR *szValue)
{
	return WritePrivateProfileString(_szSectionName, szOption, szValue, _szFileName) != 0;
}

BOOL Profile::SetInt(const TCHAR *szOption, int iValue)
{
	TCHAR buf[256];
	_stprintf_s(buf, TEXT("%d"), iValue);
	return WritePrivateProfileString(_szSectionName, szOption, buf, _szFileName) != 0;
}

BOOL NetmonProfile::Load(const TCHAR *szDefaultAdapter)
{
	TCHAR szCurrentExe[MAX_PATH];
	TCHAR szCurrentDir[MAX_PATH];
	TCHAR szProfile[MAX_PATH];
	TCHAR *pFilePart;

	// Get full path name of Netmon.exe
	GetModuleFileName(0, szCurrentExe, MAX_PATH);

	// Get base directory
	GetFullPathName(szCurrentExe, MAX_PATH, szCurrentDir, &pFilePart);
	*pFilePart = TEXT('\0');

	// Get full path name of Netmon.ini
	_tcscpy_s(szProfile, MAX_PATH, szCurrentDir);
	_tcscat_s(szProfile, MAX_PATH, TEXT("Netmon.ini"));

	// Init
	_pf.Init(szProfile, TEXT("Netmon Profile v1"));

	// Load preferences
	// If the key doesn't exist, a default value is written to the ini file
	if( _pf.GetString(TEXT("Adapter"), _szAdapter, 256) == FALSE )
	{
		SetAdapter(szDefaultAdapter);
	}

	if( _pf.GetString(TEXT("AutoStart"), _szAutoStart, MAX_PATH) == FALSE )
	{
		SetAutoStart(TEXT(""));
	}

	if( _pf.GetInt(TEXT("AutoCapture"), &_bAutoCapture) == FALSE )
	{
		SetAutoCapture(FALSE);
	}

	if( _pf.GetInt(TEXT("DtViewEnable"), &_bDtViewEnable) == FALSE )
	{
		SetDtViewEnable(FALSE);
	}

	if( _pf.GetInt(TEXT("DtViewMaxSpace"), &_iDtViewMaxSpace) == FALSE )
	{
		SetDtViewMaxSpace(0); // No limit
	}

	return TRUE;
}

BOOL NetmonProfile::GetAdapter(TCHAR *szAdapter, int cchLen)
{
	_tcscpy_s(szAdapter, cchLen, _szAdapter);
	return TRUE;
}

BOOL NetmonProfile::SetAdapter(const TCHAR *szAdapter)
{
	if( _pf.SetString(TEXT("Adapter"), szAdapter) == TRUE )
	{
		_tcscpy_s(_szAdapter, 256, szAdapter);
		return TRUE;
	}
	return FALSE;
}

BOOL NetmonProfile::GetAutoStart(TCHAR *szAutoStart, int cchLen)
{
	_tcscpy_s(szAutoStart, cchLen, _szAutoStart);
	return TRUE;
}

BOOL NetmonProfile::SetAutoStart(const TCHAR *szAutoStart)
{
	if( _pf.SetString(TEXT("AutoStart"), szAutoStart) == TRUE )
	{
		_tcscpy_s(_szAutoStart, MAX_PATH, szAutoStart);
		return TRUE;
	}
	return FALSE;
}

BOOL NetmonProfile::GetAutoCapture(BOOL *pAutoCapture)
{
	*pAutoCapture = _bAutoCapture;
	return TRUE;
}

BOOL NetmonProfile::SetAutoCapture(BOOL bAutoCapture)
{
	if( _pf.SetInt(TEXT("AutoCapture"), (int)bAutoCapture) == TRUE )
	{
		_bAutoCapture = bAutoCapture;
		return TRUE;
	}
	return FALSE;
}

BOOL NetmonProfile::GetDtViewEnable(BOOL *pEnable)
{
	*pEnable = _bDtViewEnable;
	return TRUE;
}

BOOL NetmonProfile::SetDtViewEnable(BOOL bEnable)
{
	if( _pf.SetInt(TEXT("DtViewEnable"), (int)bEnable) == TRUE )
	{
		_bDtViewEnable = bEnable;
		return TRUE;
	}
	return FALSE;
}

BOOL NetmonProfile::GetDtViewMaxSpace(int *pMaxSpace)
{
	*pMaxSpace = _iDtViewMaxSpace;
	return TRUE;
}

BOOL NetmonProfile::SetDtViewMaxSpace(int iMaxSpace)
{
	if( _pf.SetInt(TEXT("DtViewMaxSpace"), (int)iMaxSpace) == TRUE )
	{
		_iDtViewMaxSpace = iMaxSpace;
		return TRUE;
	}
	return FALSE;
}
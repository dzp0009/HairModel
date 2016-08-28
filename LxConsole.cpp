//----------------------------------------------------------------------
// File:			LxConsole.cpp
// Programmer:		Lvdi Wang
// Last modified:	09/17/2008 (Revision 0.31)
// Description:		Methods for LxConsole.h
//----------------------------------------------------------------------
// Copyright (c) 2008 Microsoft Research Asia and Lvdi Wang.
// All Rights Reserved.
// 
// This software and related documentation is part of LX Library.
//----------------------------------------------------------------------


#include "LxConsole.h"

namespace lvdiw {

//----------------------------------------------------------------------
// Static variables.
//----------------------------------------------------------------------

static HANDLE	s_hLxConsoleOut				= NULL;
static WCHAR*	s_pLxConsoleBuffer			= NULL;
static SHORT	s_nLxConsoleIndent			= 0;
static SHORT	s_nLxConsoleIndentSize		= 2;
static CONSOLE_SCREEN_BUFFER_INFO	s_LxLastCsbInfo;


//----------------------------------------------------------------------
// Create a console for the current process.
//----------------------------------------------------------------------
bool LxCreateConsole(
	const WCHAR*		wcsTitle
	)
{
	// Try allocate console for this process
	if (AllocConsole() == FALSE)
		return false;

	// Get output handle
	s_hLxConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!s_hLxConsoleOut)
		return false;

	// Set console properties
	SetConsoleTitle(wcsTitle); 
	//SetConsoleMode(s_hLxConsoleOut, ENABLE_WRAP_AT_EOL_OUTPUT);
	if (s_pLxConsoleBuffer != NULL)
		delete[] s_pLxConsoleBuffer;
	s_pLxConsoleBuffer = new WCHAR[LX_CONSOLE_BUFFER_SIZE];
	s_pLxConsoleBuffer[LX_CONSOLE_BUFFER_SIZE-1] = L'\0';

	// Redirect unbuffered STDOUT to the console
	FILE* fp = _wfdopen(_open_osfhandle((intptr_t)s_hLxConsoleOut, 
		_O_TEXT), L"w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	return true;
}

//----------------------------------------------------------------------
// Free the console and related resources.
//----------------------------------------------------------------------
bool LxFreeConsole()
{
	if (s_pLxConsoleBuffer)
		delete[] s_pLxConsoleBuffer;

	if (FreeConsole() == FALSE)
		return false;

	return true;
}


//----------------------------------------------------------------------
// Print text to the console.
//----------------------------------------------------------------------
bool LxPrint(const WCHAR* wcsText, ...)
{
	if (!s_hLxConsoleOut || !s_pLxConsoleBuffer)
		return false;

	// Write text to the console
    va_list args;
    va_start(args, wcsText);
	vswprintf_s(s_pLxConsoleBuffer, LX_CONSOLE_BUFFER_SIZE - 1,
		wcsText, args);
    va_end(args);
	WriteConsole(s_hLxConsoleOut, s_pLxConsoleBuffer, 
		(DWORD)wcslen(s_pLxConsoleBuffer), NULL, NULL);

	return true;
}


//----------------------------------------------------------------------
// Print text with given attributes to the console.
//----------------------------------------------------------------------
bool LxPrint(const WORD wAttributes, const WCHAR* wcsText, ...)
{
	if (!s_hLxConsoleOut || !s_pLxConsoleBuffer ||
		!GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
		return false;

	// Write text to the console
    va_list args;
    va_start(args, wcsText);
	vswprintf_s(s_pLxConsoleBuffer, LX_CONSOLE_BUFFER_SIZE - 1,
		wcsText, args);
    va_end(args);

	SetConsoleTextAttribute(s_hLxConsoleOut, wAttributes);
	WriteConsole(s_hLxConsoleOut, s_pLxConsoleBuffer, 
		(DWORD)wcslen(s_pLxConsoleBuffer), NULL, NULL);
	SetConsoleTextAttribute(s_hLxConsoleOut, s_LxLastCsbInfo.wAttributes);

	return true;
}


//----------------------------------------------------------------------
// Print a line of text to the console.
//----------------------------------------------------------------------
bool LxPrintLine(const WCHAR* wcsText, ...)
{
	if (!s_hLxConsoleOut || !s_pLxConsoleBuffer)
		return false;

	// Write text to the console
    va_list args;
    va_start(args, wcsText);
	vswprintf_s(s_pLxConsoleBuffer, LX_CONSOLE_BUFFER_SIZE - 1,
		wcsText, args);
    va_end(args);
	WriteConsole(s_hLxConsoleOut, s_pLxConsoleBuffer, 
		(DWORD)wcslen(s_pLxConsoleBuffer), NULL, NULL);

	// Start a new line
	return LxNewLine();
}


//----------------------------------------------------------------------
// Print a line of text with given attributes to the console.
//----------------------------------------------------------------------
bool LxPrintLine(const WORD wAttributes, const WCHAR* wcsText, ...)
{
	if (!s_hLxConsoleOut || !s_pLxConsoleBuffer ||
		!GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
		return false;

	// Write text to the console
    va_list args;
    va_start(args, wcsText);
	vswprintf_s(s_pLxConsoleBuffer, LX_CONSOLE_BUFFER_SIZE - 1,
		wcsText, args);
    va_end(args);

	SetConsoleTextAttribute(s_hLxConsoleOut, wAttributes);
	WriteConsole(s_hLxConsoleOut, s_pLxConsoleBuffer, 
		(DWORD)wcslen(s_pLxConsoleBuffer), NULL, NULL);
	SetConsoleTextAttribute(s_hLxConsoleOut, s_LxLastCsbInfo.wAttributes);

	// Start a new line
	return LxNewLine();
}


//----------------------------------------------------------------------
// Print a line of text in red color.
//----------------------------------------------------------------------
bool LxErr(const WCHAR* wcsText, ...)
{
	if (!s_hLxConsoleOut || !s_pLxConsoleBuffer ||
		!GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
		return false;

	// Write text to the console
	va_list args;
	va_start(args, wcsText);
	vswprintf_s(s_pLxConsoleBuffer, LX_CONSOLE_BUFFER_SIZE - 1,
		wcsText, args);
	va_end(args);

	SetConsoleTextAttribute(s_hLxConsoleOut, LX_TEXT_RED);
	WriteConsole(s_hLxConsoleOut, s_pLxConsoleBuffer, 
		(DWORD)wcslen(s_pLxConsoleBuffer), NULL, NULL);
	SetConsoleTextAttribute(s_hLxConsoleOut, s_LxLastCsbInfo.wAttributes);

	// Start a new line
	return LxNewLine();
}

//----------------------------------------------------------------------
// Print a line of text to the console.
//----------------------------------------------------------------------
bool LxNewLine()
{
	if (!GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
		return false;

	s_LxLastCsbInfo.dwCursorPosition.X = s_nLxConsoleIndent;
	if ((s_LxLastCsbInfo.dwSize.Y - 1) == s_LxLastCsbInfo.dwCursorPosition.Y)
	{
		// Scroll the buffer up.
		SMALL_RECT rectScroll;
		rectScroll.Left = 0;
		rectScroll.Top = 1;
		rectScroll.Right = s_LxLastCsbInfo.dwSize.X - 1;
		rectScroll.Bottom = s_LxLastCsbInfo.dwSize.Y - 1;

		COORD coordDest;
		coordDest.X = 0;
		coordDest.Y = 0;

		CHAR_INFO chiFill;
		chiFill.Attributes = LX_TEXT_GREY;
		chiFill.Char.UnicodeChar = L' ';

		ScrollConsoleScreenBuffer(s_hLxConsoleOut, &rectScroll, NULL,
			coordDest, &chiFill);
	}
	else s_LxLastCsbInfo.dwCursorPosition.Y++;

	SetConsoleCursorPosition(s_hLxConsoleOut,
		s_LxLastCsbInfo.dwCursorPosition);

	return true;
}


//----------------------------------------------------------------------
// Increase line indent by s_nLxConsoleIndentSize
//----------------------------------------------------------------------
void LxIncreaseIndent()
{
	if (s_nLxConsoleIndent < LX_CONSOLE_MAX_INDENT &&
		GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
	{
		s_nLxConsoleIndent = s_nLxConsoleIndent + s_nLxConsoleIndentSize;
		if (s_LxLastCsbInfo.dwCursorPosition.X < s_nLxConsoleIndent)
		{
			s_LxLastCsbInfo.dwCursorPosition.X = s_nLxConsoleIndent;
			SetConsoleCursorPosition(s_hLxConsoleOut,
				s_LxLastCsbInfo.dwCursorPosition);
		}
	}
}

//----------------------------------------------------------------------
// Decrease line indent by s_nLxConsoleIndentSize
//----------------------------------------------------------------------
void LxDecreaseIndent()
{
	if (s_nLxConsoleIndent > 0 &&
		GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
	{
		s_nLxConsoleIndent = s_nLxConsoleIndent - s_nLxConsoleIndentSize;
		if (s_LxLastCsbInfo.dwCursorPosition.X > s_nLxConsoleIndent)
		{
			s_LxLastCsbInfo.dwCursorPosition.X = s_nLxConsoleIndent;
			SetConsoleCursorPosition(s_hLxConsoleOut,
				s_LxLastCsbInfo.dwCursorPosition);
		}
	}
}

//----------------------------------------------------------------------
// Set console text attributes
//----------------------------------------------------------------------
bool LxSetConsoleTextColor(const WORD wAttributes)
{
	if (SetConsoleTextAttribute(s_hLxConsoleOut, wAttributes))
		return true;

	return false;
}


//----------------------------------------------------------------------
// Start a new text-based progress bar
//----------------------------------------------------------------------
bool LxCreateProgressBar(LX_PROGRESS_BAR* pProgressBar)
{
	// create a progress bar with default parameters
	return LxCreateProgressBar(32, 100, LX_TEXT_LIME,
		LX_TEXT_GREEN, L'D', L')', pProgressBar);
}

bool LxCreateProgressBar(
	const SHORT			sWidth, 
	const UINT			uMax, 
	const WORD			wAttribFore, 
	const WORD			wAttribBack, 
	const WCHAR			wCharFore, 
	const WCHAR			wCharBack, 
	LX_PROGRESS_BAR*	pProgressBar
	)
{
	if (!pProgressBar || !s_hLxConsoleOut || !s_pLxConsoleBuffer ||
		!GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
		return false;

	// validate the width of the progress bar
	pProgressBar->sWidth = sWidth > 3 ? 
		(sWidth < s_LxLastCsbInfo.dwSize.X ? sWidth : s_LxLastCsbInfo.dwSize.X - 1) : 3;

	// position the progress bar so that it remains in one row
	if ((s_LxLastCsbInfo.dwCursorPosition.X + pProgressBar->sWidth) >= 
		s_LxLastCsbInfo.dwSize.X)
	{
		LxNewLine();
		GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo);
	}
	pProgressBar->dwPosition = s_LxLastCsbInfo.dwCursorPosition;
	if ((s_nLxConsoleIndent + pProgressBar->sWidth) >= s_LxLastCsbInfo.dwSize.X)
		pProgressBar->dwPosition.X = 0;

	// assign other parameters
	pProgressBar->uMax = uMax > 1 ? uMax : 1;
	pProgressBar->wAttribFore = wAttribFore;
	pProgressBar->wAttribBack = wAttribBack;
	pProgressBar->wCharFore = wCharFore;
	pProgressBar->wCharBack = wCharBack;

	// draw initial progress bar
	SetConsoleTextAttribute(s_hLxConsoleOut, LX_TEXT_WHITE);
	SetConsoleCursorPosition(s_hLxConsoleOut, pProgressBar->dwPosition);
	WriteConsole(s_hLxConsoleOut, L"[", 1, NULL, NULL);
	SetConsoleTextAttribute(s_hLxConsoleOut, pProgressBar->wAttribBack);
	for (SHORT i = 0; i < pProgressBar->sWidth - 7; i++)
		LxPrint(L"%c", pProgressBar->wCharBack);
	SetConsoleTextAttribute(s_hLxConsoleOut, LX_TEXT_WHITE);
	WriteConsole(s_hLxConsoleOut, L"] 0%", 4, NULL, NULL);

	SetConsoleTextAttribute(s_hLxConsoleOut, s_LxLastCsbInfo.wAttributes);

	return true;
}


//----------------------------------------------------------------------
// Update the progress bar with given progress
//----------------------------------------------------------------------
bool LxUpdateProgress(const LX_PROGRESS_BAR* pProgressBar, const UINT uProgress)
{
	if (!pProgressBar || !s_hLxConsoleOut || !s_pLxConsoleBuffer ||
		!GetConsoleScreenBufferInfo(s_hLxConsoleOut, &s_LxLastCsbInfo))
		return false;

	// compute number of "ignited" cells
	float fPercent = uProgress >= pProgressBar->uMax ? 
		1.0f : (float)uProgress / (float)pProgressBar->uMax;
	SHORT sCell = (SHORT)((float)(pProgressBar->sWidth - 7) * fPercent);

	// draw cells
	COORD nextCoord = pProgressBar->dwPosition;
	nextCoord.X += 1;
	SetConsoleCursorPosition(s_hLxConsoleOut, nextCoord);
	SetConsoleTextAttribute(s_hLxConsoleOut, pProgressBar->wAttribFore);
	SHORT i = 0;
	for (; i < sCell; i++)
		LxPrint(L"%c", pProgressBar->wCharFore);
	SetConsoleTextAttribute(s_hLxConsoleOut, pProgressBar->wAttribBack);
	for (; i < pProgressBar->sWidth - 7; i++)
		LxPrint(L"%c", pProgressBar->wCharBack);

	// draw text
	nextCoord.X += pProgressBar->sWidth - 5;
	SetConsoleCursorPosition(s_hLxConsoleOut, nextCoord);
	SetConsoleTextAttribute(s_hLxConsoleOut, LX_TEXT_WHITE);
	LxPrint(L"    ");	// clear previous text
	SetConsoleCursorPosition(s_hLxConsoleOut, nextCoord);
	int nPercent = (int)(fPercent*100.0f + 0.5f);
	LxPrint(L"%d%%", nPercent);

	SetConsoleTextAttribute(s_hLxConsoleOut, s_LxLastCsbInfo.wAttributes);

	return true;
}


}  // namespace lvdiw

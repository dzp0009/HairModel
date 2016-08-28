//----------------------------------------------------------------------
// File:			LxConsole.h
// Programmer:		Lvdi Wang
// Last modified:	10/13/2008 (Revision 0.22)
// Description:		Encapsulate some high-level console functions.
//----------------------------------------------------------------------
// Copyright (c) 2008 Microsoft Research Asia and Lvdi Wang.
// All Rights Reserved.
// 
// This software and related documentation is part of LX Library.
//----------------------------------------------------------------------
// History:
//	Revision 0.22	10/13/2008
//		Redirect stdout to console.
//	Revision 0.21	09/17/2008
//		Fixed a minor bug in progress bar.
//		Added a new text-based progress bar.
//	Revision 0.2	06/04/2008
//		Use Windows high-level console I/O instead of redirecting STDIO.
//		Added control over text color and cursor position.
//	Revision 0.1	06/02/2008
//		Copied directly from one of Xi's project.
//----------------------------------------------------------------------

#ifndef LVDIW_LX_CONSOLE_H_
#define LVDIW_LX_CONSOLE_H_

#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

namespace lvdiw {

//----------------------------------------------------------------------
// Color constants in the console.
//----------------------------------------------------------------------

const WORD	LX_TEXT_GREY	= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
const WORD	LX_TEXT_WHITE	= LX_TEXT_GREY | FOREGROUND_INTENSITY;
const WORD	LX_TEXT_MAROON	= FOREGROUND_RED;
const WORD	LX_TEXT_RED		= FOREGROUND_RED | FOREGROUND_INTENSITY;
const WORD	LX_TEXT_GREEN	= FOREGROUND_GREEN;
const WORD	LX_TEXT_LIME	= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD	LX_TEXT_NAVY	= FOREGROUND_BLUE;
const WORD	LX_TEXT_BLUE	= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
const WORD	LX_TEXT_OLIVE	= FOREGROUND_RED | FOREGROUND_GREEN;
const WORD	LX_TEXT_YELLOW	= LX_TEXT_OLIVE | FOREGROUND_INTENSITY;
const WORD	LX_TEXT_PURPLE	= FOREGROUND_RED | FOREGROUND_BLUE;
const WORD	LX_TEXT_FUCHSIA	= LX_TEXT_PURPLE | FOREGROUND_INTENSITY;
const WORD	LX_TEXT_TEAL	= FOREGROUND_GREEN | FOREGROUND_BLUE;
const WORD	LX_TEXT_AQUA	= LX_TEXT_TEAL | FOREGROUND_INTENSITY;
const WORD	LX_TEXTBG_GREEN	= BACKGROUND_GREEN;
const WORD	LX_TEXTBG_LIME	= BACKGROUND_GREEN | BACKGROUND_INTENSITY;


//----------------------------------------------------------------------
// Other constants for console output.
//----------------------------------------------------------------------

const int	LX_CONSOLE_BUFFER_SIZE	= 512;
const SHORT	LX_CONSOLE_MAX_INDENT	= 16;


//----------------------------------------------------------------------
// The creation and release of console window.
//----------------------------------------------------------------------

bool LxCreateConsole(const WCHAR*);
bool LxFreeConsole();


//----------------------------------------------------------------------
// Text output functions.
//----------------------------------------------------------------------

bool LxPrint(const WCHAR*, ...);
bool LxPrint(const WORD, const WCHAR*, ...);
bool LxPrintLine(const WCHAR*, ...);
bool LxPrintLine(const WORD, const WCHAR*, ...);
bool LxErr(const WCHAR*, ...);
bool LxNewLine();
void LxIncreaseIndent();
void LxDecreaseIndent();
bool LxSetConsoleTextColor(const WORD);


//----------------------------------------------------------------------
// Text-based progress bar widget.
//----------------------------------------------------------------------

struct LX_PROGRESS_BAR
{
	COORD	dwPosition;
	SHORT	sWidth;
	UINT	uMax;
	WORD	wAttribFore;
	WORD	wAttribBack;
	WCHAR	wCharFore;
	WCHAR	wCharBack;
};

bool LxCreateProgressBar(LX_PROGRESS_BAR* pProgressBar);
bool LxCreateProgressBar(const SHORT sWidth, const UINT uMax, const WORD wAttribFore, const WORD wAttribBack, const WCHAR wCharFore, const WCHAR wCharBack, LX_PROGRESS_BAR* pProgressBar);
bool LxUpdateProgress(const LX_PROGRESS_BAR* pProgressBar, const UINT uProgress);


}  // namespace lvdiw


#endif
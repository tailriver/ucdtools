@echo off
setlocal
set ROOTDIR=%cd%
set SRCDIR=src
set BUILDDIR=build

cd %SRCDIR%
nmake /nologo /f Makefile.nmake %*
if errorlevel 1 ( timeout -1 & exit /b 1 )
cd %ROOTDIR%

rmdir /S /Q %BUILDDIR% 2>NUL

if exist %SRCDIR%\ucd.lib (
  mkdir %BUILDDIR%
  copy %SRCDIR%\ucd.h %BUILDDIR% >NUL
  copy %SRCDIR%\ucd.lib %BUILDDIR% >NUL
  copy %SRCDIR%\*.exe %BUILDDIR% >NUL
  echo.
  echo Built files are copied to `%BUILDDIR%'.
)

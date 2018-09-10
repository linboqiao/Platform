@echo off
REM 
REM   Project:  Embedded Learning Library (ELL)
REM   File:     buildtask.cmd
REM   Authors:  Chris Lovett
REM 
REM   Requires: Anaconda or Miniconda with Python 3
REM 
Setlocal 

REM first 3 arguments are fixed
set ELL_SRC=%1
set CONDA_PATH=%2
set VS_VERSION=%3

shift
shift
shift

REM these arguments are optional named arguments
set RPI_CLUSTER=
set RPI_PASSWORD=
set RPI_APIKEY=
set GIT_REPO=

:parse
if "%1"=="" goto :start
set name=%1
shift
set value=%1
shift
set %name:~1%=%value%
goto :parse

:start
pushd %ELL_SRC%

call %CONDA_PATH%\Scripts\activate.bat ell

REM this needs to be run under VSTS agent user account (NTSERVICE account)
git lfs install

echo ===================================== BUILD ==================================
call .\rebuild.cmd %VS_VERSION%
if ERRORLEVEL 1 exit /B 1

cd build
call openblas.cmd

echo ===================================== CMAKE with additional options ==================================
cmake .. -DRPI_CLUSTER=%RPI_CLUSTER% -DRPI_PASSWORD=%RPI_PASSWORD% -DRPI_KEY=%RPI_APIKEY% -DGIT_REPO=%GIT_REPO%
if ERRORLEVEL 1 exit /B 1

echo ===================================== TEST ==================================
if "%RPI_CLUSTER%"=="" goto :fulltest

ctest . --build-config release -R pitest_test -VV
if ERRORLEVEL 1 exit /B 1
goto :eof
goto :done

:fulltest
ctest . --build-config release -VV
if ERRORLEVEL 1 exit /B 1
goto :eof

:done
endlocal
popd
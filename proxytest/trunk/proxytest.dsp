# Microsoft Developer Studio Project File - Name="proxytest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=proxytest - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "proxytest.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "proxytest.mak" CFG="proxytest - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "proxytest - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "proxytest - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "proxytest - Win32 VTune" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "proxytest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /Ox /Ot /Oa /Og /Oi /I "g:\openssl-0.9.6a\inc32" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_REENTRANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ws2_32.lib kernel32.lib libeay32.lib gdi32.lib /nologo /subsystem:console /machine:I386 /libpath:"g:\openssl-0.9.6a\out32dll.dbg"

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W4 /Gm /GX /ZI /Od /I "g:\openssl-0.9.6a\inc32" /I ".\pthread\include" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_DEBUG" /D "_REENTRANT" /FAcs /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib kernel32.lib libeay32.lib gdi32.lib pthreadvce.lib /nologo /subsystem:console /map /debug /machine:I386 /pdbtype:sept /libpath:"g:\openssl-0.9.6a\out32dll.dbg" /libpath:"pthread\lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "proxytest___Win32_VTune"
# PROP BASE Intermediate_Dir "proxytest___Win32_VTune"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "proxytest___Win32_VTune"
# PROP Intermediate_Dir "proxytest___Win32_VTune"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W4 /GX /Ox /Ot /Oa /Og /Oi /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MT /W4 /GX /Zi /Ox /Ot /Oa /Og /Oi /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386

!ENDIF 

# Begin Target

# Name "proxytest - Win32 Release"
# Name "proxytest - Win32 Debug"
# Name "proxytest - Win32 VTune"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "popt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\popt\popt.c

!IF  "$(CFG)" == "proxytest - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\popt\popt.h
# End Source File
# Begin Source File

SOURCE=.\popt\popthelp.c

!IF  "$(CFG)" == "proxytest - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\popt\poptint.h
# End Source File
# Begin Source File

SOURCE=.\popt\poptparse.c

!IF  "$(CFG)" == "proxytest - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\popt\system.h
# End Source File
# End Group
# Begin Group "xml"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\xml\xmlconfig.h
# End Source File
# Begin Source File

SOURCE=.\xml\xmlinput.cpp

!IF  "$(CFG)" == "proxytest - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xml\xmlinput.h
# End Source File
# Begin Source File

SOURCE=.\xml\xmlinput_c.c

!IF  "$(CFG)" == "proxytest - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xml\xmlinputp.h
# End Source File
# Begin Source File

SOURCE=.\xml\xmloutput.cpp

!IF  "$(CFG)" == "proxytest - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xml\xmloutput.h
# End Source File
# Begin Source File

SOURCE=.\xml\xmlstream.h
# End Source File
# End Group
# Begin Group "aes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\aes\boxes-fst.dat.h"
# End Source File
# Begin Source File

SOURCE=.\aes\encrypt.cpp

!IF  "$(CFG)" == "proxytest - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\aes\rijndael-api-fst.h"
# End Source File
# End Group
# Begin Source File

SOURCE=.\CAASymCipher.cpp
# End Source File
# Begin Source File

SOURCE=.\CABase64.cpp
# End Source File
# Begin Source File

SOURCE=.\CACacheLoadBalancing.cpp
# End Source File
# Begin Source File

SOURCE=.\CACmdLnOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\CAConditionVariable.cpp
# End Source File
# Begin Source File

SOURCE=.\CADatagramSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\CAFirstMix.cpp
# End Source File
# Begin Source File

SOURCE=.\CAFirstMixChannelList.cpp
# End Source File
# Begin Source File

SOURCE=.\CAInfoService.cpp
# End Source File
# Begin Source File

SOURCE=.\CAIPList.cpp
# End Source File
# Begin Source File

SOURCE=.\CALastMix.cpp
# End Source File
# Begin Source File

SOURCE=.\CALocalProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\CAMiddleMix.cpp
# End Source File
# Begin Source File

SOURCE=.\CAMix.cpp
# End Source File
# Begin Source File

SOURCE=.\CAMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\CAMutex.cpp
# End Source File
# Begin Source File

SOURCE=.\CAMuxSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\CAQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\CARoundTripTime.cpp
# End Source File
# Begin Source File

SOURCE=.\CASemaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\CASignature.cpp
# End Source File
# Begin Source File

SOURCE=.\CASingleSocketGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\CASocket.cpp
# End Source File
# Begin Source File

SOURCE=.\CASocketAddrINet.cpp
# End Source File
# Begin Source File

SOURCE=.\CASocketAddrUnix.cpp
# End Source File
# Begin Source File

SOURCE=.\CASocketGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\CASocketList.cpp
# End Source File
# Begin Source File

SOURCE=.\CASymCipher.cpp

!IF  "$(CFG)" == "proxytest - Win32 Release"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CAThread.cpp
# End Source File
# Begin Source File

SOURCE=.\CAUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\proxytest.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "proxytest - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "proxytest - Win32 Debug"

# ADD CPP /W3 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "proxytest - Win32 VTune"

# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CAASymCipher.hpp
# End Source File
# Begin Source File

SOURCE=.\CABase64.hpp
# End Source File
# Begin Source File

SOURCE=.\CACacheLoadBalancing.hpp
# End Source File
# Begin Source File

SOURCE=.\CACallback.hpp
# End Source File
# Begin Source File

SOURCE=.\CACmdLnOptions.hpp
# End Source File
# Begin Source File

SOURCE=.\CAConditionVariable.hpp
# End Source File
# Begin Source File

SOURCE=.\CADatagramSocket.hpp
# End Source File
# Begin Source File

SOURCE=.\CAFirstMix.hpp
# End Source File
# Begin Source File

SOURCE=.\CAFirstMixChannelList.hpp
# End Source File
# Begin Source File

SOURCE=.\CAInfoService.hpp
# End Source File
# Begin Source File

SOURCE=.\CAIPList.hpp
# End Source File
# Begin Source File

SOURCE=.\CALastMix.hpp
# End Source File
# Begin Source File

SOURCE=.\CALocalProxy.hpp
# End Source File
# Begin Source File

SOURCE=.\CAMiddleMix.hpp
# End Source File
# Begin Source File

SOURCE=.\CAMix.hpp
# End Source File
# Begin Source File

SOURCE=.\CAMsg.hpp
# End Source File
# Begin Source File

SOURCE=.\CAMutex.hpp
# End Source File
# Begin Source File

SOURCE=.\CAMuxSocket.hpp
# End Source File
# Begin Source File

SOURCE=.\CAQueue.hpp
# End Source File
# Begin Source File

SOURCE=.\CARoundTripTime.hpp
# End Source File
# Begin Source File

SOURCE=.\CASemaphore.hpp
# End Source File
# Begin Source File

SOURCE=.\CASignature.hpp
# End Source File
# Begin Source File

SOURCE=.\CASingleSocketGroup.hpp
# End Source File
# Begin Source File

SOURCE=.\CASocket.hpp
# End Source File
# Begin Source File

SOURCE=.\CASocketAddr.hpp
# End Source File
# Begin Source File

SOURCE=.\CASocketAddrINet.hpp
# End Source File
# Begin Source File

SOURCE=.\CASocketAddrUnix.hpp
# End Source File
# Begin Source File

SOURCE=.\CASocketGroup.hpp
# End Source File
# Begin Source File

SOURCE=.\CASocketList.hpp
# End Source File
# Begin Source File

SOURCE=.\CASymCipher.hpp
# End Source File
# Begin Source File

SOURCE=.\CAThread.hpp
# End Source File
# Begin Source File

SOURCE=.\CAUtil.hpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# End Target
# End Project

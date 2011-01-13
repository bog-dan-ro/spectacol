## NSIS (nsis.sf.net) script to produce installer for win32 platform
## Copyright (c) 2009 Marek Januszewski

## $Id$

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

!define FUSE_VERSION "1.0.0.1" ; can contain letters like -RC1
!define FUSE_FULL_VERSION "1.0.0.1" ; must contain four numeric tokens
!define DISPLAY_NAME "Free Unix Spectrum Emulator (Fuse) ${FUSE_VERSION}"
!define SETUP_FILENAME "fuse-${FUSE_VERSION}-setup"
!define SETUP_FILE "${SETUP_FILENAME}.exe"
!define HKLM_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fuse"

;Include Modern UI
!include "MUI2.nsh"
!include "Util.nsh"

;--------------------------------
;General

Name "${DISPLAY_NAME}"
outFile "${SETUP_FILE}"
Caption "${DISPLAY_NAME}"
 
installDir "$PROGRAMFILES\Fuse"

; [Additional Installer Settings ]
XPStyle on
SetCompress force
SetCompressor lzma

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "COPYING"
  !define MUI_COMPONENTSPAGE_SMALLDESC
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !define MUI_FINISHPAGE_RUN "$INSTDIR\fuse.exe"
  !define MUI_FINISHPAGE_NOREBOOTSUPPORT
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !define MUI_FINISHPAGE_SHOWREADME_CHECKED
  !define MUI_FINISHPAGE_SHOWREADME ""
  !define MUI_FINISHPAGE_SHOWREADME_TEXT "Delete configuration file"
  !define MUI_FINISHPAGE_SHOWREADME_FUNCTION un.DeleteConfigFile
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Version Information

  VIProductVersion ${FUSE_FULL_VERSION}
  VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" ""
  VIAddVersionKey /LANG=${LANG_ENGLISH} "InternalName" "${SETUP_FILENAME}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright (c) 1999-2011 Philip Kendall and others; see the file 'AUTHORS' for more details."
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Fuse"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${FUSE_VERSION}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "OriginalFilename" "${SETUP_FILE}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "Fuse - the Free Unix Spectrum Emulator"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${FUSE_VERSION}"

;--------------------------------
;File association functions

!macro RegisterExtensionCall _EXECUTABLE _EXTENSION _DESCRIPTION
  Push `${_DESCRIPTION}`
  Push `${_EXTENSION}`
  Push `${_EXECUTABLE}`
  ${CallArtificialFunction} RegisterExtension_
!macroend

!macro UnRegisterExtensionCall _EXTENSION _DESCRIPTION
  Push `${_EXTENSION}`
  Push `${_DESCRIPTION}`
  ${CallArtificialFunction} UnRegisterExtension_
!macroend

!macro RegisterExtension_
  Exch $R2 ;exe
  Exch
  Exch $R1 ;ext
  Exch
  Exch 2
  Exch $R0 ;desc
  Exch 2
  Push $0
  Push $1

  ReadRegStr $1 HKCR $R1 ""  ; read current file association
  StrCmp "$1" "" NoBackup  ; is it empty
  StrCmp "$1" "$R0" NoBackup  ; is it our own
    WriteRegStr HKCR $R1 "backup_val" "$1"  ; backup current value
NoBackup:
  WriteRegStr HKCR $R1 "" "$R0"  ; set our file association

  ReadRegStr $0 HKCR $R0 ""
  StrCmp $0 "" 0 Skip
    WriteRegStr HKCR "$R0" "" "$R0"
    WriteRegStr HKCR "$R0\shell" "" "open"
    WriteRegStr HKCR "$R0\DefaultIcon" "" "$R2,0"
Skip:
  WriteRegStr HKCR "$R0\shell\open\command" "" '"$R2" "%1"'
  WriteRegStr HKCR "$R0\shell\edit" "" "Edit $R0"
  WriteRegStr HKCR "$R0\shell\edit\command" "" '"$R2" "%1"'
 
  Pop $1
  Pop $0
  Pop $R2
  Pop $R1
  Pop $R0
!macroend

!macro UnRegisterExtension_
  Exch $R1 ;desc
  Exch
  Exch $R0 ;ext
  Exch
  Push $0
  Push $1

  ReadRegStr $1 HKCR $R0 ""
  StrCmp $1 $R1 0 NoOwn ; only do this if we own it
  ReadRegStr $1 HKCR $R0 "backup_val"
  StrCmp $1 "" 0 Restore ; if backup="" then delete the whole key
  DeleteRegKey HKCR $R0
  Goto NoOwn

Restore:
  WriteRegStr HKCR $R0 "" $1
  DeleteRegValue HKCR $R0 "backup_val"
  DeleteRegKey HKCR $R1 ;Delete key with association name settings

NoOwn: 
  Pop $1
  Pop $0
  Pop $R1
  Pop $R0
!macroend

!define RegisterExtension `!insertmacro RegisterExtensionCall`
!define UnRegisterExtension `!insertmacro UnRegisterExtensionCall`

;--------------------------------
; Uninstall previous version

 Section "" SecUninstallPrevious

    Push $R0
    ReadRegStr $R0 HKLM "${HKLM_REG_KEY}" "UninstallString"

    ; Check if we are upgrading a previous installation
    ${If} $R0 == '"$INSTDIR\uninstall.exe"'
        DetailPrint "Removing previous installation..."

        ; Run the uninstaller silently
        ExecWait '"$INSTDIR\uninstall.exe" /S _?=$INSTDIR'
    ${EndIf}

    Pop $R0

SectionEnd

;--------------------------------
; start default section

section "!Fuse Core Files (required)" SecCore

    SectionIn RO
    DetailPrint "Installing Fuse Core Files..."
 
    ; set the installation directory as the destination for the following actions
    setOutPath "$INSTDIR"

    ; Installation files
    File "AUTHORS"
    File "COPYING"
    File "fuse.exe"
    File "fuse.html"
    File "*.dll"
    SetOutPath $INSTDIR\lib
    File "lib\*"
    SetOutPath "$INSTDIR\roms"
    File "roms\*"
 
    ; create the uninstaller
    writeUninstaller "$INSTDIR\uninstall.exe"

    ; Write the uninstall keys for Windows
    WriteRegStr HKLM "${HKLM_REG_KEY}" "DisplayName" "${DISPLAY_NAME}"
    WriteRegStr HKLM "${HKLM_REG_KEY}" "DisplayVersion" "${FUSE_VERSION}"
    WriteRegStr HKLM "${HKLM_REG_KEY}" "HelpLink" "http://fuse-emulator.sourceforge.net"
    WriteRegStr HKLM "${HKLM_REG_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "${HKLM_REG_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${HKLM_REG_KEY}" "NoRepair" 1

sectionEnd

;--------------------------------
; Create shortcuts

section "Start Menu and Desktop links" SecShortcuts

    DetailPrint "Creating Shortcuts..."
    CreateDirectory "$SMPROGRAMS\Fuse"
    CreateShortCut "$SMPROGRAMS\Fuse\Fuse.lnk" "$INSTDIR\fuse.exe"
    CreateShortCut "$SMPROGRAMS\Fuse\Manual.lnk" "$INSTDIR\fuse.html"
    CreateShortCut "$SMPROGRAMS\Fuse\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$DESKTOP\Fuse.lnk" "$INSTDIR\fuse.exe"

sectionEnd

;--------------------------------
; Register common file extesions

section "Register File Extensions"  SecFileExt

    DetailPrint "Registering File Extensions..."
    ${registerExtension} "$INSTDIR\fuse.exe" ".rzx" "Fuse RZX File"
    ${registerExtension} "$INSTDIR\fuse.exe" ".sna" "Fuse SNA File"
    ${registerExtension} "$INSTDIR\fuse.exe" ".szx" "Fuse SZX File"
    ${registerExtension} "$INSTDIR\fuse.exe" ".tap" "Fuse TAP File"
    ${registerExtension} "$INSTDIR\fuse.exe" ".tzx" "Fuse TZX File"
    ${registerExtension} "$INSTDIR\fuse.exe" ".z80" "Fuse Z80 File"
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'

sectionEnd

;--------------------------------
; uninstaller section start

section "uninstall"

    ; Unregister file extensions association
    DetailPrint "Deleting Registry Keys..."
    ${unregisterExtension} ".rzx" "Fuse RZX File"
    ${unregisterExtension} ".sna" "Fuse SNA File"
    ${unregisterExtension} ".szx" "Fuse SZX File"
    ${unregisterExtension} ".tap" "Fuse TAP File"
    ${unregisterExtension} ".tzx" "Fuse TZX File"
    ${unregisterExtension} ".z80" "Fuse Z80 File"
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
	
    ; Delete the links
    DetailPrint "Deleting Shortcuts..."
    Delete "$SMPROGRAMS\Fuse\Fuse.lnk"
    Delete "$SMPROGRAMS\Fuse\Manual.lnk"
    Delete "$SMPROGRAMS\Fuse\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\Fuse"
    Delete "$DESKTOP\Fuse.lnk"

    ; Installation files
    DetailPrint "Deleting Files..."
    Delete "$INSTDIR\lib\*"
    RMDir  "$INSTDIR\lib"
    Delete "$INSTDIR\roms\*"
    RMDir  "$INSTDIR\roms"
    Delete "$INSTDIR\AUTHORS"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\fuse.exe"
    Delete "$INSTDIR\fuse.html"
    Delete "$INSTDIR\*.dll"
    RMDir "$INSTDIR"

    ; Delete the uninstaller and remove the uninstall keys for Windows
    DetailPrint "Deleting Uninstaller..."
    Delete "$INSTDIR\uninstall.exe"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Fuse"

sectionEnd

Function un.DeleteConfigFile
    Delete "$PROFILE\fuse.cfg"
FunctionEnd

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "The core files required to use Fuse (program, libraries, ROMs, etc.)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} "Adds icons to your start menu and your desktop for easy access"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFileExt} "Register common file extensions with Fuse: rzx, sna, szx, tap, tzx and z80"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

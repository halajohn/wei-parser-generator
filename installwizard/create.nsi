!include Sections.nsh
!include env_mani.nsh

!define VERSION 1.0.0
Name "WeiParserGenerator"

OutFile ".\install.exe"
InstallDir "$PROGRAMFILES\WeiParserGenerator"

ShowInstDetails show
ShowUninstDetails show
XPStyle on

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Variable
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Var WPG_DIR

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Pages
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PageEx Directory
  DirText "Choose the WeiParserGenerator installation directory" "WeiParserGenerator Installation Folder"
  DirVar $WPG_DIR
  PageCallbacks wcs_directory_pre
PageExEnd

PageEx instfiles
PageExEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

InstType "Full"

Section "WeiParserGenerator" wcs_section_idx
  SectionIn 1
  WriteRegStr HKLM "Software\WeiParserGenerator" "Installdir" "$WPG_DIR"
  CreateDirectory $WPG_DIR\bin
  SetOutPath $WPG_DIR\bin
  File "..\release\wpg.exe"
  File "..\debug\wpg_d.exe"
  CreateDirectory $SMPROGRAMS\WeiParserGenerator
  Push $WPG_DIR\bin
  Call AddToPath
  writeUninstaller "$WPG_DIR\uninstall.exe"
  CreateDirectory $SMPROGRAMS\WeiParserGenerator
  CreateShortCut "$SMPROGRAMS\WeiParserGenerator\uninstall.lnk" "$WPG_DIR\uninstall.exe"
sectionend

section "Uninstall"
  ReadRegStr $WPG_DIR HKLM Software\WeiParserGenerator "Installdir"
  DeleteRegKey HKLM "Software\WeiParserGenerator"
  delete "$WPG_DIR\uninstall.exe"
  delete "$SMPROGRAMS\WeiParserGenerator\uninstall.lnk"
  RMDir /r "$WPG_DIR\bin\"
  RMDir /r "$WPG_DIR\"
  RMDir /r "$SMPROGRAMS\WeiParserGenerator\"
  RMDir "$SMPROGRAMS\WeiParserGenerator\"
  Push $WPG_DIR\bin
  Call un.RemoveFromPath  
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Function wcs_directory_pre
  Push $0
  StrCpy $WPG_DIR "C:\Program Files\WeiParserGenerator\"
  Pop $0
FunctionEnd

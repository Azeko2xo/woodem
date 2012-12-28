# mostly copied from http://nsis.sourceforge.net/A_simple_installer_with_start_menu_shortcut_and_uninstaller
!define APPNAME "Woo"
##
##  COMPONENT, VERSION must be given on the commandline via -D...
##  
##  COMPONENT is like wooExtra.bar
##
# used to write installation directory
!define ARP "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${COMPONENT}"
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-libs" "InstallLocation"

SetCompressor /solid lzma

RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)
 
# name on the title bar
Name "${APPNAME} - ${COMPONENT}"
outFile "${APPNAME}-${COMPONENT}-${VERSION}-installer.exe"
 
!include LogicLib.nsh
!include "nsis-wwoo-admincheck.nsh"
function .onInit
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

Function .onVerifyInstDir
	IfFileExists $INSTDIR\python27.dll PathGood
		MessageBox MB_OK "Must be installed into the same directory as ${APPNAME}-libs" /SD IDOK
		Abort
	PathGood:
FunctionEnd


# Just three pages - license agreement, install location, and installation
page license
page directory
Page instfiles

LicenseText "You must agree to licensing terms of the software provided."
LicenseData "license-wooExtra.rtf"

 
section "install"
	IfFileExists $INSTDIR\eggs\${COMPONENT}-*.egg 0 doInstall
		MessageBox MB_OKCANCEL|MB_ICONINFORMATION "Older version of ${COMPONENT} exists and will be deleted first." /SD IDOK IDOK delInstalled
			Abort
		delInstalled:
			delete $INSTDIR\${COMPONENT}-*.egg
	doInstall:
	setOutPath $INSTDIR\eggs
	File ${COMPONENT}-*.egg

	# Uninstaller - See function un.onInit and section "uninstall" for configuration
	writeUninstaller "$INSTDIR\uninstall-${COMPONENT}.exe"
	# Registry information for add/remove programs
	WriteRegStr HKLM "${ARP}" "DisplayName" "${APPNAME}-${COMPONENT}"
	WriteRegStr HKLM "${ARP}" "UninstallString" "$\"$INSTDIR\uninstall-${COMPONENT}.exe$\""
	WriteRegStr HKLM "${ARP}" "QuietUninstallString" "$\"$INSTDIR\uninstall-${COMPONENT}.exe$\" /S"
sectionEnd

function un.onInit
	SetShellVarContext all
	MessageBox MB_OKCANCEL "Permanantly remove ${APPNAME}-${COMPONENT}?" /SD IDOK IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
functionEnd

section "uninstall"
	delete $INSTDIR\eggs\${COMPONENT}*.egg
	delete $INSTDIR\uninstall-${COMPONENT}.exe
	DeleteRegKey HKLM "${ARP}"
sectionEnd


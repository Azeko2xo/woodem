# mostly copied from http://nsis.sourceforge.net/A_simple_installer_with_start_menu_shortcut_and_uninstaller
!define APPNAME "WooDEM"
!define COMPONENT "libs"
#
# VERSION must be defined on the command-line
#
!define DESCRIPTION "Discrete dynamic computations (support libraries)"
# used to write installation directory
!define ARPBASE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!define ARP "${ARPBASE}-${COMPONENT}"
InstallDir "$PROGRAMFILES\${APPNAME}"

SetCompressor /solid lzma

RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)
 
# rtf or txt file - remember if it is txt, it must be in the DOS text format (\r\n)
# LicenseData "license.rtf"
# This will be in the installer/uninstaller's title bar
Name "${APPNAME} - ${DESCRIPTION}"
#Icon "logo.ico"
outFile "${APPNAME}-${COMPONENT}-${VERSION}-installer.exe"
 
!include LogicLib.nsh
!include "nsis-wwoo-admincheck.nsh"
function .onInit
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

# Just three pages - license agreement, install location, and installation
page license
page directory
Page instfiles

LicenseText "Libraries provided in this package are licensed differently. All of them are open-source licenses which allow using respective libraries with other open-source projects. They are compatible with Woo itself (distributed separately), which is released under the GNU General Public License, version 2 or later."
 
section "install"
	setOutPath $INSTDIR
	# don't install woo itself, and skip wooExtra's, if installed by accident
	file /r /x wooExtra.* /x wwoo* /x woo.* /x *.nsh *
 
	# Uninstaller - See function un.onInit and section "uninstall" for configuration
	writeUninstaller "$INSTDIR\uninstall-${COMPONENT}.exe"
 
	# Registry information for add/remove programs
	WriteRegStr HKLM "${ARP}" "DisplayName" "${APPNAME}-${COMPONENT} - ${DESCRIPTION}"
	WriteRegStr HKLM "${ARP}" "UninstallString" "$\"$INSTDIR\uninstall-${COMPONENT}.exe$\""
	WriteRegStr HKLM "${ARP}" "QuietUninstallString" "$\"$INSTDIR\uninstall-${COMPONENT}.exe$\" /S"
sectionEnd
 
# Uninstaller
 
function un.onInit
	SetShellVarContext all
	IfFileExists $INSTDIR\wwoo.exe 0 PathGood
		MessageBox MB_OK "WooDEM-main is still installed; uninstall it first."
		Abort
	PathGood:
	MessageBox MB_OKCANCEL "Permanantly remove ${APPNAME}-${COMPONENT}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
functionEnd
 
section "uninstall"
	# Remove files
	delete $INSTDIR\*.pyd
	delete $INSTDIR\*.dll
 
	# Remove uninstaller information from the registry
	DeleteRegKey HKLM "${ARP}"
sectionEnd

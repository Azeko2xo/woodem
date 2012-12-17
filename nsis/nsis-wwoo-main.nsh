# mostly copied from http://nsis.sourceforge.net/A_simple_installer_with_start_menu_shortcut_and_uninstaller
!define APPNAME "WooDEM"
!define COMPONENT "main"
#
# VERSION must be given on the command-line
#
!define DESCRIPTION "Discrete dynamic computations (main modules)"
!define ARP "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}-${COMPONENT}"
InstallDir "$PROGRAMFILES\${APPNAME}"
# use by default where libs are already installed
# http://stackoverflow.com/questions/13775288/installer-adding-files-to-an-already-installed-packages-dir
InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WooDEM-libs" "InstallLocation"

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
!include "nsis-EnvVarUpdate.nsh"
function .onInit
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

Function .onVerifyInstDir
	IfFileExists $INSTDIR\python27.dll PathGood
		MessageBox MB_OK "Must be installed into the same directory as WooDEM-libs"
		Abort ;
	PathGood:
FunctionEnd

# Just three pages - license agreement, install location, and installation
page license
page directory
Page instfiles

LicenseText "WooDEM is distributed under the term of the GNU General Public License, version 2 or later. Please refer to its text http://opensource.org/licenses/GPL-2.0 for details."
 
section "install"
	setOutPath $INSTDIR
	file /x *.egg /x *.nsh /x *-installer.exe wwoo* woo* 

	${EnvVarUpdate} $0 "PATH" "P" "HKLM" "$\"$INSTDIR$\""
 
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
	MessageBox MB_OKCANCEL "Permanantly remove ${APPNAME}-${COMPONENT}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
functionEnd
 
section "uninstall"
	# Remove files
	delete $INSTDIR\wwoo*.*
	delete $INSTDIR\woo.*

	# remove path again
	${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$\"$INSTDIR$\""

	# Remove uninstaller information from the registry
	DeleteRegKey HKLM "${ARP}"
sectionEnd

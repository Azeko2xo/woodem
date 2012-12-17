# mostly copied from http://nsis.sourceforge.net/A_simple_installer_with_start_menu_shortcut_and_uninstaller
!define APPNAME "WooDEM"
##
##  COMPONENT, VERSION must be given on the commandline via -D...
##  
##  COMPONENT is like wooExtra.bar
##
# used to write installation directory
InstallDir "$PROGRAMFILES\${APPNAME}"
InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WooDEM-libs" "InstallLocation"

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
		MessageBox MB_OK "Must be installed into the same directory as WooDEM-libs"
		Abort
	PathGood:
FunctionEnd


# Just three pages - license agreement, install location, and installation
page license
page directory
Page instfiles

LicenseText "This module is licensed the same as WooDEM itself. WooDEM is distributed under the term of the GNU General Public License, version 2 or later. Please refer to its text http://opensource.org/licenses/GPL-2.0 for details."
 
section "install"
	setOutPath $INSTDIR\eggs
	File ${COMPONENT}-*.egg
sectionEnd

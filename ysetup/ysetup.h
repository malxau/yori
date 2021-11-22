
__success(return)
BOOL
SetupFindLocalPkgPath(
    __out PYORI_STRING LocalPath
    );

BOOL
SetupPlatformSupportsShortcuts(VOID);

BOOL
SetupWriteTerminalProfile(
    __in PYORI_STRING ProfileFileName,
    __in PYORI_STRING YoriExeFullPath
    );

BOOL
SetupGetDefaultInstallDir(
    __out PYORI_STRING InstallDir
    );

LPCSTR
SetupGetDllMissingMessage(VOID);

/**
 The set of installation types supported by this program.
 */
typedef enum _YSETUP_INSTALL_TYPE {
    InstallTypeCore = 1,
    InstallTypeTypical = 2,
    InstallTypeComplete = 3
} YSETUP_INSTALL_TYPE;

/**
 If set, debugging symbols should be installed.
 */
#define YSETUP_INSTALL_SYMBOLS           0x00000001

/**
 If set, source code should be installed.
 */
#define YSETUP_INSTALL_SOURCE            0x00000002

/**
 If set, a desktop shortcut should be installed.
 */
#define YSETUP_INSTALL_DESKTOP_SHORTCUT  0x00000004

/**
 If set, a start menu shortcut should be installed.
 */
#define YSETUP_INSTALL_START_SHORTCUT    0x00000008

/**
 If set, a Windows Terminal profile should be installed.
 */
#define YSETUP_INSTALL_TERMINAL_PROFILE  0x00000010

/**
 If set, the user path environment should include the install directory.
 */
#define YSETUP_INSTALL_USER_PATH         0x00000020

/**
 If set, the system path environment should include the install directory.
 */
#define YSETUP_INSTALL_SYSTEM_PATH       0x00000040

/**
 If set, an uninstall entry should be installed.
 */
#define YSETUP_INSTALL_UNINSTALL         0x00000080

/**
 A callback function that can be invoked to indicate status as installation
 progresses.
 */
typedef VOID YSETUP_STATUS_CALLBACK(PCYORI_STRING, PVOID);

/**
 A pointer to a callback function that can be invoked to indicate status as
 installation progresses.
 */
typedef YSETUP_STATUS_CALLBACK *PYSETUP_STATUS_CALLBACK;

__success(return)
BOOL
SetupInstallSelectedWithOptions(
    __in PYORI_STRING InstallDir,
    __in YSETUP_INSTALL_TYPE InstallType,
    __in DWORD InstallOptions,
    __in PYSETUP_STATUS_CALLBACK StatusCallback,
    __in PVOID StatusContext,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorText
    );

BOOL
SetupGuiDisplayUi(VOID);


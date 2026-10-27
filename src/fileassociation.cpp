#include "fileassociation.h"
#include "core.h"
#include <QDir>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#endif

namespace moly {
void registerMolyPackFiles(const QString &executable) {
#ifdef Q_OS_WIN
    const auto path=QDir::toNativeSeparators(QFileInfo(executable).absoluteFilePath());
    if(!QFileInfo(path).isFile() || path.contains('"'))throw Error("The launcher executable could not be registered.");
    auto put=[](const wchar_t *key,const QString &value) {
        HKEY handle=nullptr;
        if(RegCreateKeyExW(HKEY_CURRENT_USER,key,0,nullptr,0,KEY_SET_VALUE,nullptr,&handle,nullptr)!=ERROR_SUCCESS)throw Error("Windows could not register Moly pack files for this user.");
        const auto result=RegSetValueExW(handle,nullptr,0,REG_SZ,reinterpret_cast<const BYTE*>(value.utf16()),DWORD((value.size()+1)*sizeof(wchar_t)));
        RegCloseKey(handle);
        if(result!=ERROR_SUCCESS)throw Error("Windows could not save the file association.");
    };
    put(L"Software\\Classes\\MolyLauncher.Pack","Moly setup");
    put(L"Software\\Classes\\MolyLauncher.Pack\\DefaultIcon","\""+path+"\",0");
    put(L"Software\\Classes\\MolyLauncher.Pack\\shell\\open\\command","\""+path+"\" --import-pack \"%1\"");
    put(L"Software\\Classes\\.mlypack","MolyLauncher.Pack");
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);
#else
    Q_UNUSED(executable)
    throw Error("File registration is currently available on Windows.");
#endif
}
}

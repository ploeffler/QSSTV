// Device-free child used to check the real SFX/launcher lifecycle on Windows.
#include <windows.h>
#include <string>
int wmain(int argc,wchar_t **argv) {
  if(argc!=3 || std::wstring(argv[2])!=L"argument with spaces")return 2;
  wchar_t module[32768],home[32768],cwd[32768];
  if(!GetModuleFileNameW(nullptr,module,32768) || !GetEnvironmentVariableW(L"HOME",home,32768) || !GetCurrentDirectoryW(32768,cwd))return 3;
  if(std::wstring(home)!=cwd)return 4;
  const std::wstring path(module);const auto directory=path.substr(0,path.find_last_of(L"\\/"));
  if(directory==home)return 5;
  HANDLE file=CreateFileW(argv[1],GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file==INVALID_HANDLE_VALUE)return 6;
  const std::wstring text=std::wstring(home)+L"\r\n"+directory+L"\r\n";
  DWORD written=0;const BOOL result=WriteFile(file,text.data(),DWORD(text.size()*sizeof(wchar_t)),&written,nullptr);
  CloseHandle(file);Sleep(1500);return result?0:7;
}

// Starts the bundled application and waits so the outer SFX can clean up safely.
#include <windows.h>
#include <string>
#include <vector>
static std::wstring environment(const wchar_t *name) {
  const DWORD size=GetEnvironmentVariableW(name,nullptr,0);
  if(!size)return {};
  std::vector<wchar_t> value(size);
  if(!GetEnvironmentVariableW(name,value.data(),size))return {};
  return value.data();
}
static int failure(const wchar_t *message) {
  MessageBoxW(nullptr,message,L"QSSTV portable",MB_OK|MB_ICONERROR);return 1;
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR arguments,int) {
  std::vector<wchar_t> module(32768);
  const DWORD count=GetModuleFileNameW(nullptr,module.data(),DWORD(module.size()));
  if(!count || count>=module.size())return failure(L"Cannot locate the portable application.");
  const std::wstring path(module.data(),count);
  const std::wstring executable=path.substr(0,path.find_last_of(L"\\/"))+L"\\qsstv.exe";
  // QSSTV uses HOME for persistent images/templates. Never default to the
  // disposable SFX directory, and do not replace an explicit user setting.
  std::wstring home=environment(L"HOME");
  if(home.empty()) {
    home=environment(L"USERPROFILE");
    if(home.empty() || !SetEnvironmentVariableW(L"HOME",home.c_str()))
      return failure(L"Cannot determine a persistent user directory.");
  }
  std::wstring command=L"\""+executable+L"\"";
  if(arguments && *arguments){command+=L" ";command+=arguments;}
  std::vector<wchar_t> mutableCommand(command.begin(),command.end());mutableCommand.push_back(0);
  STARTUPINFOW startup={};startup.cb=sizeof(startup);PROCESS_INFORMATION child={};
  if(!CreateProcessW(executable.c_str(),mutableCommand.data(),nullptr,nullptr,FALSE,0,nullptr,home.c_str(),&startup,&child))
    return failure(L"Cannot start QSSTV. The portable payload may be incomplete.");
  CloseHandle(child.hThread);
  const DWORD wait=WaitForSingleObject(child.hProcess,INFINITE);DWORD result=1;
  if(wait==WAIT_OBJECT_0)GetExitCodeProcess(child.hProcess,&result);
  CloseHandle(child.hProcess);return int(result);
}

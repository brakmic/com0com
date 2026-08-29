#include "comdb_mock.h"
#include <vector>
#define COMDB_MAX_PORTS_ARBITRATED 4096
static std::vector<BYTE> g_busyMask;
static bool g_openCalled;
void MockComDb::Reset() { g_busyMask.clear(); g_openCalled=false; }
void MockComDb::SetBusy(int n, bool b) {
 if(n<=0)return; int i=n-1,need=(i/8)+1;
 if((int)g_busyMask.size()<need) g_busyMask.resize(need,0);
 if(b) g_busyMask[i/8]|=(1<<(i%8)); else g_busyMask[i/8]&=~(1<<(i%8));
}
bool MockComDb::IsBusy(int n) {
 if(n<=0)return false; int i=n-1,bi=i/8;
 if(bi>=(int)g_busyMask.size())return false;
 return (g_busyMask[bi]&(1<<(i%8)))!=0;
}
bool MockComDb::Claim(int n) { if(IsBusy(n))return false; SetBusy(n,true); return true; }
bool MockComDb::Release(int n) { if(!IsBusy(n))return false; SetBusy(n,false); return true; }
extern "C" {
LONG APIENTRY Mock_ComDBOpen(PHANDLE p) { if(!p)return 87; g_openCalled=true; *p=(HANDLE)1; return 0; }
LONG APIENTRY Mock_ComDBClose(HANDLE h) { (void)h; g_openCalled=false; return 0; }
LONG APIENTRY Mock_ComDBGetCurrentPortUsage(HANDLE h,PBYTE b,DWORD s,ULONG f,LPDWORD m) {
 (void)h;(void)f; DWORD c=(DWORD)g_busyMask.size(); if(!c)c=1;
 DWORD mx=c*8; if(mx>COMDB_MAX_PORTS_ARBITRATED)mx=COMDB_MAX_PORTS_ARBITRATED;
 if(m)*m=mx; if(b&&s>=c){for(DWORD i=0;i<c;i++)b[i]=g_busyMask[i];} return 0;
}
LONG APIENTRY Mock_ComDBClaimPort(HANDLE h,DWORD n,BOOL f,PBYTE r) {
 (void)h;(void)f;(void)r; if(MockComDb::IsBusy(n))return 32; MockComDb::SetBusy(n,true); return 0;
}
LONG APIENTRY Mock_ComDBReleasePort(HANDLE h,DWORD n) {
 (void)h; if(!MockComDb::IsBusy(n))return 2; MockComDb::SetBusy(n,false); return 0;
}
}

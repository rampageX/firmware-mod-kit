
void WINAPI RtlSecondsSince1970ToFileTime( DWORD Seconds, LPFILETIME ft );

#ifdef NEED_NAME_WINDOWS_TO_UNIX
static inline const char * nameWindowToUnix(const char * lpFileName) {
  if ((lpFileName[0] == 'c') && (lpFileName[1] == ':')) return lpFileName+2;
  return lpFileName;
}
#endif


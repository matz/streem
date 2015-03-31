#ifdef _WIN32

/* It really works? */
#ifdef WIN_KERNEL_BUILD

#include <wdm.h>

int
cpu_count()
{
  return (int)KeQueryActiveProcessorCount(NULL);
}

#else

#include <windows.h>

int
cpu_count()
{
  SYSTEM_INFO si;

  GetNativeSystemInfo( &si );
  return (int)si.dwNumberOfProcessors;
}

#endif  /* WIN_KERNEL_BUILD */

#else  /* POSIX */
#include <unistd.h>

int
cpu_count()
{
  return (int)sysconf(_SC_NPROCESSORS_ONLN);
}
#endif

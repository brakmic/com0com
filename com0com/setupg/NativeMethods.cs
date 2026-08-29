using System.Runtime.InteropServices;

namespace com0com.Setup;

internal static class NativeMethods
{
    [DllImport("setup.dll", EntryPoint = "MainA", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern int MainA(string progName, string cmdLine);

    [DllImport("kernel32.dll")]
    internal static extern uint SetErrorMode(uint uMode);

    internal const uint SEM_FAILCRITICALERRORS = 0x0001;
    internal const uint SEM_NOGPFAULTERRORBOX = 0x0002;
    internal const uint SEM_NOOPENFILEERRORBOX = 0x8000;
}

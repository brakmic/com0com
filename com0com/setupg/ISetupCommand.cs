namespace com0com.Setup;

/// <summary>
/// Abstraction over setupc.exe operations so the GUI can be tested
/// without a real driver installation.
/// </summary>
public interface ISetupCommand
{
    PortPairs ListAll();
    string AddPair();
    void RemovePair(string key);
    void ChangePort(string portId, PortParams changes);
    void WaitForInstall(int seconds = 30);
    string[] GetBusyNames(string pattern = "*");
}

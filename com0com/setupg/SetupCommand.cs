using System.Diagnostics;

namespace com0com.Setup;

/// <summary>
/// Wraps calls to setupc.exe for all driver management operations.
/// Communicates via process stdout (same model as the original C++/CLI GUI).
/// </summary>
public class SetupCommand : ISetupCommand
{
    private readonly Control? _owner;

    public SetupCommand(Control? owner)
    {
        _owner = owner;
    }

    /// <summary>
    /// Executes a setupc.exe command and returns stdout lines.
    /// Throws SetupException on nonzero exit code or process failure.
    /// </summary>
    public string[] Execute(string arguments)
    {
        var lines = new List<string>();
        uint oldErrorMode = 0;

        if (_owner != null) _owner.Cursor = Cursors.WaitCursor;
        try
        {
            // Suppress system-level error dialogs (msports.dll MessageBox etc.).
            oldErrorMode = NativeMethods.SetErrorMode(
                NativeMethods.SEM_FAILCRITICALERRORS |
                NativeMethods.SEM_NOGPFAULTERRORBOX |
                NativeMethods.SEM_NOOPENFILEERRORBOX);

            using var proc = new Process
            {
                StartInfo = new ProcessStartInfo("setupc.exe", arguments)
                {
                    CreateNoWindow = true,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    WorkingDirectory = AppContext.BaseDirectory,
                    ErrorDialog = false,
                }
            };

            proc.Start();

            string? line;
            while ((line = proc.StandardOutput.ReadLine()) != null)
                lines.Add(line.Trim());

            proc.WaitForExit();

            if (proc.ExitCode != 0)
            {
                var stderr = proc.StandardError.ReadToEnd().Trim();
                var msg = stderr.Length > 0
                    ? $"setupc.exe failed with exit code {proc.ExitCode}:\n{stderr}"
                    : $"setupc.exe failed with exit code {proc.ExitCode}.";
                throw new SetupException(msg);
            }
        }
        catch (SetupException)
        {
            throw;
        }
        catch (Exception ex)
        {
            throw new SetupException($"Failed to execute setupc.exe: {ex.Message}", ex);
        }
        finally
        {
            NativeMethods.SetErrorMode(oldErrorMode);
            if (_owner != null) _owner.Cursor = Cursors.Default;
        }

        return lines.ToArray();
    }

    // ── convenience methods ──────────────────────────────────────────

    public PortPairs ListAll()
    {
        var lines = Execute("--detail-prms list");
        return SetupOutputParser.ParseListAll(lines);
    }

    public string AddPair()
    {
        var lines = Execute("--detail-prms install - -");
        return SetupOutputParser.ParseInstallResult(lines);
    }

    public void WaitForInstall(int seconds = 30)
    {
        Execute($"--wait +{seconds}");
    }

    public void RemovePair(string key)
    {
        Execute($"remove {key}");
    }

    public void ChangePort(string portId, PortParams changes)
    {
        var prms = changes.ToParamString();
        var result = Execute($"change {portId} {prms}");
        // Verify the change was applied by checking output
        if (result.Length == 0)
            throw new SetupException($"Change to {portId} produced no output.");
    }

    public string[] GetBusyNames(string pattern = "*")
    {
        return Execute($"busynames {pattern}");
    }
}

/// <summary>
/// Thrown when a setupc.exe operation fails.
/// </summary>
public class SetupException : Exception
{
    public SetupException() { }
    public SetupException(string message) : base(message) { }
    public SetupException(string message, Exception inner) : base(message, inner) { }
}

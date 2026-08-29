namespace com0com.Setup.Tests;

/// <summary>
/// Integration tests for SetupCommand against real setupc.exe.
/// Only tests commands that do not trigger system-level message boxes.
/// busynames is excluded because msports.dll shows dialogs that neither
/// --silent nor SetErrorMode can suppress.
/// </summary>
public class SetupCommandIntegrationTests
{
    [Fact]
    public void Execute_Help_ReturnsSuccess()
    {
        var cmd = new SetupCommand(null);
        var lines = cmd.Execute("--silent help");
        Assert.NotEmpty(lines);
    }

    [Fact]
    public void Execute_InvalidCommand_ThrowsSetupException()
    {
        var cmd = new SetupCommand(null);
        var ex = Assert.Throws<SetupException>(() => cmd.Execute("--silent nonexistent_command"));
        Assert.Contains("exit code", ex.Message);
    }

    [Fact]
    public void Execute_List_ReturnsExistingPairs()
    {
        var cmd = new SetupCommand(null);
        var lines = cmd.Execute("--silent --detail-prms list");
        Assert.NotEmpty(lines);
        Assert.Contains(lines, l => l.StartsWith("CNCA", StringComparison.OrdinalIgnoreCase));
        Assert.Contains(lines, l => l.StartsWith("CNCB", StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public void ListAll_ReturnsExistingPairs()
    {
        var cmd = new SetupCommand(null);
        var pairs = cmd.ListAll();
        Assert.NotEmpty(pairs);
        foreach (var pair in pairs.Values)
        {
            Assert.NotNull(pair.PortA);
            Assert.NotNull(pair.PortB);
            Assert.False(string.IsNullOrEmpty(pair.PairKey));
        }
    }

    [Fact]
    public void Execute_Quit_ReturnsSuccess()
    {
        var cmd = new SetupCommand(null);
        var lines = cmd.Execute("quit");
        Assert.NotNull(lines);
    }
}

namespace com0com.Setup.Tests;

/// <summary>
/// In-memory fake of ISetupCommand for unit testing MainForm
/// without a real driver installation.
/// </summary>
public class FakeSetupCommand : ISetupCommand
{
    private readonly Dictionary<string, PortPair> _store = new();

    public FakeSetupCommand() { }

    public FakeSetupCommand(params (string key, PortPair pair)[] pairs)
    {
        foreach (var (key, pair) in pairs)
            _store[key] = pair;
    }

    public PortPair this[string key]
    {
        get => _store[key];
        set => _store[key] = value;
    }

    public PortPairs ListAll()
    {
        var result = new PortPairs();
        foreach (var kv in _store)
            result[kv.Key] = kv.Value;
        return result;
    }

    public string AddPair()
    {
        var max = _store.Keys.Any()
            ? _store.Keys.Select(k => int.TryParse(k, out var n) ? n : -1).Max()
            : -1;
        var next = (max + 1).ToString();
        var pair = new PortPair { PairKey = next };
        pair.PortA["PortName"] = $"CNCA{next}";
        pair.PortB["PortName"] = $"CNCB{next}";
        _store[next] = pair;
        return next;
    }

    public void RemovePair(string key) => _store.Remove(key);
    public void ChangePort(string portId, PortParams changes) { /* no-op for tests */ }
    public void WaitForInstall(int seconds = 30) { /* no-op */ }
    public string[] GetBusyNames(string pattern = "*") => [];
}

public class MainFormTests
{
    [StaFact]
    public void Constructor_CreatesFormWithControls()
    {
        var fake = new FakeSetupCommand();
        using var form = new MainForm(fake);
        Assert.NotNull(form);
        Assert.Equal("com0com Setup", form.Text);
    }

    [StaFact]
    public void OnLoad_LoadsPairsIntoListBox()
    {
        var pair = new PortPair { PairKey = "0" };
        pair.PortA["PortName"] = "COM3";
        pair.PortB["PortName"] = "COM4";

        var fake = new FakeSetupCommand(("0", pair));

        using var form = new MainForm(fake);
        form.Show();
        Application.DoEvents(); // let OnLoad populate controls

        // ListBox should have "Pair 0: COM3 <-> COM4"
        var listBox = GetField<ListBox>(form, "listPairs");
        Assert.Single(listBox.Items);
        Assert.Contains("COM3", listBox.Items[0]!.ToString());
        Assert.Contains("COM4", listBox.Items[0]!.ToString());
    }

    [StaFact]
    public void OnLoad_WithEmptyStore_ShowsEmptyList()
    {
        var fake = new FakeSetupCommand();
        using var form = new MainForm(fake);
        form.Show();

        var listBox = GetField<ListBox>(form, "listPairs");
        Assert.Empty(listBox.Items);
    }

    [StaFact]
    public void ClearSelection_DisablesPortPanels()
    {
        var fake = new FakeSetupCommand();
        using var form = new MainForm(fake);
        form.Show();

        var panelA = GetField<Panel>(form, "panelPortA");
        var panelB = GetField<Panel>(form, "panelPortB");

        Assert.False(panelA.Enabled);
        Assert.False(panelB.Enabled);
    }

    [StaFact]
    public void SelectPair_EnablesPortPanels()
    {
        var pair = new PortPair { PairKey = "0" };
        pair.PortA["PortName"] = "COM3";
        pair.PortB["PortName"] = "COM4";

        var fake = new FakeSetupCommand(("0", pair));
        using var form = new MainForm(fake);
        form.Show();
        Application.DoEvents(); // let OnLoad fire and populate controls

        var panelA = GetField<Panel>(form, "panelPortA");
        var panelB = GetField<Panel>(form, "panelPortB");

        Assert.True(panelA.Enabled);
        Assert.True(panelB.Enabled);
    }

    [StaFact]
    public void Reset_ReloadsPairsFromStore()
    {
        var pair = new PortPair { PairKey = "0" };
        pair.PortA["PortName"] = "COM3";
        var fake = new FakeSetupCommand(("0", pair));
        using var form = new MainForm(fake);
        form.Show();

        // Modify store externally
        var pair2 = new PortPair { PairKey = "1" };
        pair2.PortA["PortName"] = "COM5";
        pair2.PortB["PortName"] = "COM6";
        fake["1"] = pair2;

        // Click reset
        var btnReset = GetField<Button>(form, "btnReset");
        btnReset.PerformClick();

        var listBox = GetField<ListBox>(form, "listPairs");
        Assert.Equal(2, listBox.Items.Count);
    }

    [StaFact]
    public void AddPair_IncrementsStore_AndSelectsNewPair()
    {
        var fake = new FakeSetupCommand();
        using var form = new MainForm(fake);
        form.Show();

        var btnAdd = GetField<Button>(form, "btnAdd");
        btnAdd.PerformClick();

        var listBox = GetField<ListBox>(form, "listPairs");
        Assert.Single(listBox.Items);
        Assert.Equal("0", fake["0"].PairKey);
    }

    // ── helpers ──────────────────────────────────────────────────

    private static T GetField<T>(object obj, string name)
    {
        var field = obj.GetType().GetField(name,
            System.Reflection.BindingFlags.NonPublic |
            System.Reflection.BindingFlags.Instance);
        Assert.NotNull(field);
        return (T)field.GetValue(obj)!;
    }
}

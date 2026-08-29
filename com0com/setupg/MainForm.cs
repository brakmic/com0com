using System.ComponentModel;

namespace com0com.Setup;

public partial class MainForm : Form
{
    private readonly ISetupCommand _setup;
    private PortPairs _pairs = new();
    private PortPair? _selectedPair;
    private bool _loading;
    private bool _dirty;

    // Pin source options for dropdowns
    private static readonly string[] PinSourcesA =
    {
        "",           // disconnected
        "RRTS", "RDTR", "ROUT1", "ROUT2", "ROPEN",   // remote
        "LRTS", "LDTR", "LOUT1", "LOUT2", "LOPEN",   // local
        "ON",
        "!RRTS", "!RDTR", "!ROUT1", "!ROUT2", "!ROPEN",
        "!LRTS", "!LDTR", "!LOUT1", "!LOUT2", "!LOPEN",
        "!ON",
    };

    private static readonly string[] PinSourcesB =
    {
        "",
        "LRTS", "LDTR", "LOUT1", "LOUT2", "LOPEN",
        "RRTS", "RDTR", "ROUT1", "ROUT2", "ROPEN",
        "ON",
        "!LRTS", "!LDTR", "!LOUT1", "!LOUT2", "!LOPEN",
        "!RRTS", "!RDTR", "!ROUT1", "!ROUT2", "!ROPEN",
        "!ON",
    };

    public MainForm() : this(new SetupCommand(null!))
    {
    }

    /// <summary>Test constructor. Accepts a fake ISetupCommand for unit testing.</summary>
    public MainForm(ISetupCommand setup)
    {
        InitializeComponent();
        _setup = setup;
    }

    protected override void OnLoad(EventArgs e)
    {
        base.OnLoad(e);
        LoadPairs();
    }

    // ── data loading ─────────────────────────────────────────────────

    private void LoadPairs()
    {
        _loading = true;
        try
        {
            _pairs = _setup.ListAll();
            listPairs.Items.Clear();
            foreach (var key in _pairs.Keys.OrderBy(k => int.TryParse(k, out var n) ? n : int.MaxValue))
            {
                var pair = _pairs[key];
                var nameA = pair.PortA["PortName"] ?? $"CNCA{key}";
                var nameB = pair.PortB["PortName"] ?? $"CNCB{key}";
                listPairs.Items.Add($"Pair {key}: {nameA} <-> {nameB}");
            }
            if (listPairs.Items.Count > 0)
                listPairs.SelectedIndex = 0;
            else
                ClearPanels();
        }
        catch (SetupException ex)
        {
            ShowError("Failed to load port pairs", ex);
        }
        finally
        {
            _loading = false;
            _dirty = false;
        }

        // Manually update selection state since events were suppressed during load.
        if (listPairs.Items.Count > 0 && listPairs.SelectedIndex >= 0)
            SelectCurrentPair();
    }

    private void SelectCurrentPair()
    {
        var keys = _pairs.Keys.OrderBy(k => int.TryParse(k, out var n) ? n : int.MaxValue).ToArray();
        var idx = listPairs.SelectedIndex;
        if (idx < 0 || idx >= keys.Length) return;

        var key = keys[idx];
        _selectedPair = _pairs[key];
        panelPortA.Enabled = true;
        panelPortB.Enabled = true;
        btnRemove.Enabled = true;
        PopulateSide(_selectedPair, 0);
        PopulateSide(_selectedPair, 1);
        _dirty = false;
    }

    private void ClearPanels()
    {
        _selectedPair = null;
        panelPortA.Enabled = false;
        panelPortB.Enabled = false;
        btnRemove.Enabled = false;
        PopulateSide(null, 0);
        PopulateSide(null, 1);
    }

    // ── pair selection ───────────────────────────────────────────────

    private void listPairs_SelectedIndexChanged(object? sender, EventArgs e)
    {
        if (_loading) return;

        var idx = listPairs.SelectedIndex;
        if (idx < 0)
        {
            ClearPanels();
            return;
        }

        SelectCurrentPair();
    }

    // ── populate controls from data ──────────────────────────────────

    private void PopulateSide(PortPair? pair, int side)
    {
        var prms = pair?[side];
        var isA = side == 0;
        var boxName = isA ? txtNameA : txtNameB;
        var chkBr = isA ? chkEmuBrA : chkEmuBrB;
        var chkOverrun = isA ? chkOverrunA : chkOverrunB;
        var chkPlugIn = isA ? chkPlugInA : chkPlugInB;
        var chkExclusive = isA ? chkExclusiveA : chkExclusiveB;
        var chkHidden = isA ? chkHiddenA : chkHiddenB;
        var chkPortsClass = isA ? chkPortsClassA : chkPortsClassB;
        var cmbCts = isA ? cmbCtsA : cmbCtsB;
        var cmbDsr = isA ? cmbDsrA : cmbDsrB;
        var cmbDcd = isA ? cmbDcdA : cmbDcdB;
        var cmbRi = isA ? cmbRiA : cmbRiB;
        var sources = isA ? PinSourcesA : PinSourcesB;

        if (prms == null)
        {
            boxName.Text = "";
            chkBr.Checked = chkOverrun.Checked = chkPlugIn.Checked = false;
            chkExclusive.Checked = chkHidden.Checked = chkPortsClass.Checked = false;
            cmbCts.DataSource = null;
            cmbDsr.DataSource = null;
            cmbDcd.DataSource = null;
            cmbRi.DataSource = null;
            return;
        }

        boxName.Text = prms["PortName"] ?? (isA ? $"CNCA{pair!.PairKey}" : $"CNCB{pair!.PairKey}");
        chkBr.Checked = IsYes(prms["EmuBR"]);
        chkOverrun.Checked = IsYes(prms["EmuOverrun"]);
        chkPlugIn.Checked = IsYes(prms["PlugInMode"]);
        chkExclusive.Checked = IsYes(prms["ExclusiveMode"]);
        chkHidden.Checked = IsYes(prms["HiddenMode"]);
        chkPortsClass.Checked = prms["PortName"] == "COM#";

        SetupPinCombo(cmbCts, prms["cts"], sources);
        SetupPinCombo(cmbDsr, prms["dsr"], sources);
        SetupPinCombo(cmbDcd, prms["dcd"], sources);
        SetupPinCombo(cmbRi, prms["ri"], sources);
    }

    private static void SetupPinCombo(ComboBox cmb, string? value, string[] sources)
    {
        cmb.DataSource = sources.Clone();
        if (value != null)
        {
            var idx = Array.IndexOf(sources, value.ToUpperInvariant());
            if (idx >= 0) cmb.SelectedIndex = idx;
        }
        else cmb.SelectedIndex = 0;
    }

    // ── actions ──────────────────────────────────────────────────────

    private void btnAdd_Click(object? sender, EventArgs e)
    {
        try
        {
            Cursor = Cursors.WaitCursor;
            var key = _setup.AddPair();
            _setup.WaitForInstall();
            LoadPairs();
            // Select the new pair
            var keys = _pairs.Keys.OrderBy(k => int.TryParse(k, out var n) ? n : int.MaxValue).ToArray();
            var idx = Array.IndexOf(keys, key);
            if (idx >= 0) listPairs.SelectedIndex = idx;
        }
        catch (SetupException ex)
        {
            ShowError("Failed to add port pair", ex);
        }
        finally { Cursor = Cursors.Default; }
    }

    private void btnRemove_Click(object? sender, EventArgs e)
    {
        if (_selectedPair == null) return;
        var key = _selectedPair.PairKey;
        var nameA = _selectedPair.PortA["PortName"] ?? $"CNCA{key}";
        var nameB = _selectedPair.PortB["PortName"] ?? $"CNCB{key}";

        var result = MessageBox.Show(
            $"Remove pair {key}: {nameA} <-> {nameB}?",
            "Remove Pair", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);

        if (result != DialogResult.Yes) return;

        try
        {
            Cursor = Cursors.WaitCursor;
            _setup.RemovePair(key);
            LoadPairs();
        }
        catch (SetupException ex)
        {
            ShowError("Failed to remove port pair", ex);
        }
        finally { Cursor = Cursors.Default; }
    }

    private void btnApply_Click(object? sender, EventArgs e)
    {
        if (_selectedPair == null) return;

        if (!_dirty)
        {
            lblStatus.Text = "Nothing to apply.";
            return;
        }

        try
        {
            Cursor = Cursors.WaitCursor;
            var pair = _selectedPair;

            var changesA = CollectChanges(pair, 0);
            if (changesA.Count > 0)
                _setup.ChangePort($"CNCA{pair.PairKey}", changesA);

            var changesB = CollectChanges(pair, 1);
            if (changesB.Count > 0)
                _setup.ChangePort($"CNCB{pair.PairKey}", changesB);

            if (changesA.Count > 0 || changesB.Count > 0)
                _setup.WaitForInstall();

            LoadPairs();
            lblStatus.Text = "Changes applied.";
        }
        catch (SetupException ex)
        {
            ShowError("Failed to apply changes", ex);
        }
        finally { Cursor = Cursors.Default; }
    }

    private void btnReset_Click(object? sender, EventArgs e)
    {
        if (_dirty)
        {
            LoadPairs();
            lblStatus.Text = "Changes discarded.";
        }
        else
        {
            LoadPairs();
            lblStatus.Text = "Already up to date.";
        }
    }

    /// <summary>Called by all input controls when the user changes a value.</summary>
    internal void InputChanged(object? sender, EventArgs e)
    {
        if (!_loading)
            _dirty = true;
    }

    // ── helpers ──────────────────────────────────────────────────────

    private PortParams CollectChanges(PortPair pair, int side)
    {
        var isA = side == 0;
        var changes = new PortParams();
        var prms = pair[side];
        var boxName = isA ? txtNameA : txtNameB;

        var newName = boxName.Text.Trim();
        var oldName = prms?["PortName"];
        if (newName.Length > 0 && !string.Equals(newName, oldName, StringComparison.OrdinalIgnoreCase))
            changes["PortName"] = newName;

        void AddIfChanged(string key, bool newVal, string yesVal = "yes", string noVal = "no")
        {
            var oldVal = prms?[key];
            var newStr = newVal ? yesVal : noVal;
            if (!string.Equals(newStr, oldVal, StringComparison.OrdinalIgnoreCase))
                changes[key] = newStr;
        }

        AddIfChanged("EmuBR", isA ? chkEmuBrA.Checked : chkEmuBrB.Checked);
        AddIfChanged("EmuOverrun", isA ? chkOverrunA.Checked : chkOverrunB.Checked);
        AddIfChanged("PlugInMode", isA ? chkPlugInA.Checked : chkPlugInB.Checked);
        AddIfChanged("ExclusiveMode", isA ? chkExclusiveA.Checked : chkExclusiveB.Checked);
        AddIfChanged("HiddenMode", isA ? chkHiddenA.Checked : chkHiddenB.Checked);

        void AddPinIfChanged(string pinKey, ComboBox cmb)
        {
            var oldVal = prms?[pinKey];
            var newVal = cmb.SelectedItem?.ToString() ?? "";
            if (!string.Equals(newVal, oldVal, StringComparison.OrdinalIgnoreCase))
                changes[pinKey] = newVal;
        }

        AddPinIfChanged("cts", isA ? cmbCtsA : cmbCtsB);
        AddPinIfChanged("dsr", isA ? cmbDsrA : cmbDsrB);
        AddPinIfChanged("dcd", isA ? cmbDcdA : cmbDcdB);
        AddPinIfChanged("ri", isA ? cmbRiA : cmbRiB);

        return changes;
    }

    private static bool IsYes(string? value) =>
        string.Equals(value, "YES", StringComparison.OrdinalIgnoreCase);

    private void ShowError(string context, Exception ex)
    {
        lblStatus.Text = $"{context}: {ex.Message}";
        MessageBox.Show($"{context}:\n\n{ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
    }
}

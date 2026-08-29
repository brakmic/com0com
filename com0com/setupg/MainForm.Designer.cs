#nullable enable

namespace com0com.Setup;

partial class MainForm
{
    private System.ComponentModel.IContainer? components = null;

    private ListBox listPairs = null!;
    private Button btnAdd = null!, btnRemove = null!, btnApply = null!, btnReset = null!;
    private Panel panelPortA = null!, panelPortB = null!;
    private Label lblStatus = null!;

    // Port A controls
    private Label lblNameA = null!;
    private TextBox txtNameA = null!;
    private GroupBox grpEmuA = null!, grpModeA = null!, grpPinsA = null!;
    private CheckBox chkEmuBrA = null!, chkOverrunA = null!;
    private CheckBox chkPlugInA = null!, chkExclusiveA = null!, chkHiddenA = null!, chkPortsClassA = null!;
    private ComboBox cmbCtsA = null!, cmbDsrA = null!, cmbDcdA = null!, cmbRiA = null!;

    // Port B controls
    private Label lblNameB = null!;
    private TextBox txtNameB = null!;
    private GroupBox grpEmuB = null!, grpModeB = null!, grpPinsB = null!;
    private CheckBox chkEmuBrB = null!, chkOverrunB = null!;
    private CheckBox chkPlugInB = null!, chkExclusiveB = null!, chkHiddenB = null!, chkPortsClassB = null!;
    private ComboBox cmbCtsB = null!, cmbDsrB = null!, cmbDcdB = null!, cmbRiB = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing && components != null) components.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container();

        // ── form ─────────────────────────────────────────────────
        Text = "com0com Setup";
        Size = new Size(740, 520);
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        Font = new Font("Segoe UI", 9F);

        // ── left panel: pair list ─────────────────────────────────
        var lblPairs = new Label
        {
            Text = "Port Pairs:", Location = new Point(12, 12), AutoSize = true
        };

        listPairs = new ListBox
        {
            Location = new Point(12, 32), Size = new Size(210, 340),
            IntegralHeight = false
        };
        listPairs.SelectedIndexChanged += listPairs_SelectedIndexChanged!;

        btnAdd = new Button
        {
            Text = "Add Pair", Location = new Point(12, 380), Size = new Size(100, 28)
        };
        btnAdd.Click += btnAdd_Click!;

        btnRemove = new Button
        {
            Text = "Remove", Location = new Point(118, 380), Size = new Size(100, 28),
            Enabled = false
        };
        btnRemove.Click += btnRemove_Click!;

        // ── center panel: Port A ─────────────────────────────────
        panelPortA = CreatePortPanel("Port A (CNCA)", 240);
        CreatePortControls(panelPortA, true);

        // ── right panel: Port B ──────────────────────────────────
        panelPortB = CreatePortPanel("Port B (CNCB)", 480);
        CreatePortControls(panelPortB, false);

        // ── bottom bar ───────────────────────────────────────────
        btnApply = new Button
        {
            Text = "Apply", Location = new Point(12, 440), Size = new Size(100, 28)
        };
        btnApply.Click += btnApply_Click!;

        btnReset = new Button
        {
            Text = "Reset", Location = new Point(118, 440), Size = new Size(100, 28)
        };
        btnReset.Click += btnReset_Click!;

        lblStatus = new Label
        {
            Location = new Point(230, 447), Size = new Size(490, 20),
            ForeColor = SystemColors.GrayText
        };

        // ── add controls ─────────────────────────────────────────
        Controls.AddRange(new Control[] {
            lblPairs, listPairs, btnAdd, btnRemove,
            panelPortA, panelPortB,
            btnApply, btnReset, lblStatus
        });

        panelPortA.Enabled = false;
        panelPortB.Enabled = false;
    }

    private Panel CreatePortPanel(string title, int x)
    {
        var panel = new Panel
        {
            Location = new Point(x, 12),
            Size = new Size(230, 420),
            BorderStyle = BorderStyle.FixedSingle
        };
        var lbl = new Label
        {
            Text = title, Location = new Point(8, 8), AutoSize = true,
            Font = new Font(Font, FontStyle.Bold)
        };
        panel.Controls.Add(lbl);
        return panel;
    }

    private void CreatePortControls(Panel panel, bool isA)
    {
        var y = 30;
        var suffix = isA ? "A" : "B";

        // Port name
        var lblName = new Label { Text = "Port Name:", Location = new Point(8, y), AutoSize = true };
        y += 20;
        var txtName = new TextBox { Location = new Point(8, y), Size = new Size(200, 23) };
        y += 32;

        if (isA) { lblNameA = lblName; txtNameA = txtName; }
        else { lblNameB = lblName; txtNameB = txtName; }

        // Emulation group
        var grpEmu = new GroupBox { Text = "Emulation", Location = new Point(8, y), Size = new Size(210, 70) };
        var chkBr = new CheckBox { Text = "Emulate Baud Rate", Location = new Point(10, 18), AutoSize = true };
        var chkOv = new CheckBox { Text = "Emulate Overrun", Location = new Point(10, 40), AutoSize = true };
        grpEmu.Controls.AddRange([chkBr, chkOv]);
        y += 78;

        if (isA) { grpEmuA = grpEmu; chkEmuBrA = chkBr; chkOverrunA = chkOv; }
        else { grpEmuB = grpEmu; chkEmuBrB = chkBr; chkOverrunB = chkOv; }

        // Mode group
        var grpMode = new GroupBox { Text = "Mode", Location = new Point(8, y), Size = new Size(210, 115) };
        var chkPlug = new CheckBox { Text = "Plug-in Mode", Location = new Point(10, 18), AutoSize = true };
        var chkExcl = new CheckBox { Text = "Exclusive Mode", Location = new Point(10, 40), AutoSize = true };
        var chkHid = new CheckBox { Text = "Hidden Mode", Location = new Point(10, 62), AutoSize = true };
        var chkPC = new CheckBox { Text = "Use Ports Class (COM#)", Location = new Point(10, 84), AutoSize = true };
        grpMode.Controls.AddRange([chkPlug, chkExcl, chkHid, chkPC]);
        y += 123;

        if (isA) { grpModeA = grpMode; chkPlugInA = chkPlug; chkExclusiveA = chkExcl; chkHiddenA = chkHid; chkPortsClassA = chkPC; }
        else { grpModeB = grpMode; chkPlugInB = chkPlug; chkExclusiveB = chkExcl; chkHiddenB = chkHid; chkPortsClassB = chkPC; }

        // Pin Routing group
        var grpPins = new GroupBox { Text = "Pin Routing", Location = new Point(8, y), Size = new Size(210, 124) };
        var (cmbCts, lblCts) = MakePinRow("CTS:", 18);
        var (cmbDsr, lblDsr) = MakePinRow("DSR:", 43);
        var (cmbDcd, lblDcd) = MakePinRow("DCD:", 68);
        var (cmbRi, lblRi) = MakePinRow("RI:", 93);
        grpPins.Controls.AddRange([lblCts, cmbCts, lblDsr, cmbDsr, lblDcd, cmbDcd, lblRi, cmbRi]);

        if (isA) { grpPinsA = grpPins; cmbCtsA = cmbCts; cmbDsrA = cmbDsr; cmbDcdA = cmbDcd; cmbRiA = cmbRi; }
        else { grpPinsB = grpPins; cmbCtsB = cmbCts; cmbDsrB = cmbDsr; cmbDcdB = cmbDcd; cmbRiB = cmbRi; }

        panel.Controls.AddRange([lblName, txtName, grpEmu, grpMode, grpPins]);

        // Wire all input controls to dirty-state tracking
        txtName.TextChanged += InputChanged;
        chkBr.CheckedChanged += InputChanged;
        chkOv.CheckedChanged += InputChanged;
        chkPlug.CheckedChanged += InputChanged;
        chkExcl.CheckedChanged += InputChanged;
        chkHid.CheckedChanged += InputChanged;
        chkPC.CheckedChanged += InputChanged;
        cmbCts.SelectedIndexChanged += InputChanged;
        cmbDsr.SelectedIndexChanged += InputChanged;
        cmbDcd.SelectedIndexChanged += InputChanged;
        cmbRi.SelectedIndexChanged += InputChanged;
    }

    private static (ComboBox, Label) MakePinRow(string label, int y)
    {
        var lbl = new Label { Text = label, Location = new Point(10, y), AutoSize = true };
        var cmb = new ComboBox
        {
            Location = new Point(50, y - 2), Size = new Size(150, 23),
            DropDownStyle = ComboBoxStyle.DropDownList
        };
        return (cmb, lbl);
    }
}

using System.Collections.Generic;
using System.Linq;

namespace com0com.Setup;

/// <summary>
/// Holds parameters for a single virtual COM port (one side of a pair).
/// Parameters are stored as key-value pairs with case-insensitive keys.
/// </summary>
public class PortParams
{
    private readonly Dictionary<string, string> _values = new(StringComparer.OrdinalIgnoreCase);

    public PortParams() { }

    public PortParams(string paramString)
    {
        if (string.IsNullOrWhiteSpace(paramString)) return;
        foreach (var pair in paramString.Split(','))
        {
            var eq = pair.IndexOf('=');
            if (eq <= 0) continue;
            var key = pair[..eq].Trim();
            var val = pair[(eq + 1)..].Trim();
            if (key.Length > 0) _values[key] = val;
        }
    }

    public string? this[string key]
    {
        get => _values.TryGetValue(key, out var v) ? v : null;
        set { if (value != null) _values[key] = value; else _values.Remove(key); }
    }

    public int Count => _values.Count;

    /// <summary>
    /// Builds a comma-separated key=value string for passing to setupc.exe.
    /// </summary>
    public string ToParamString()
    {
        if (_values.Count == 0) return "-";
        return string.Join(",", _values.Select(kv => $"{kv.Key}={kv.Value}"));
    }
}

/// <summary>
/// Represents a virtual COM port pair (CNCAx &lt;-&gt; CNCBx).
/// </summary>
public class PortPair
{
    public PortParams PortA { get; set; } = new();
    public PortParams PortB { get; set; } = new();
    public string PairKey { get; set; } = "";

    public bool IsEmpty => PortA.Count == 0 && PortB.Count == 0;

    /// <summary>Returns PortA for side 0, PortB for side 1.</summary>
    public PortParams this[int side] => side == 0 ? PortA : PortB;
}

/// <summary>
/// Maps pair keys (e.g. "0", "1") to PortPair objects.
/// </summary>
public class PortPairs : Dictionary<string, PortPair>
{
    public PortPairs() : base(StringComparer.Ordinal) { }
}

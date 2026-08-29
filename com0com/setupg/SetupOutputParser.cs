namespace com0com.Setup;

/// <summary>
/// Parses raw stdout lines from setupc.exe --detail-prms output into
/// the data model. This is the unit-testable half of SetupCommand.
/// </summary>
public static class SetupOutputParser
{
    /// <summary>
    /// Parses --detail-prms list output.
    /// Each line has the form: "CNCA0 PortName=COM3,EmuBR=yes,..."
    /// </summary>
    public static PortPairs ParseListAll(string[] lines)
    {
        var pairs = new PortPairs();

        foreach (var line in lines)
        {
            var space = line.IndexOf(' ');
            if (space <= 0) continue;

            var id = line[..space];
            var prms = line[(space + 1)..];

            if (!TryParsePortId(id, out var key, out var side))
                continue;

            if (!pairs.TryGetValue(key, out var pair))
            {
                pair = new PortPair { PairKey = key };
                pairs[key] = pair;
            }

            var portParams = new PortParams(prms);
            if (side == 0)
                pair.PortA = portParams;
            else
                pair.PortB = portParams;
        }

        return pairs;
    }

    /// <summary>
    /// Parses the pair key from --detail-prms install output.
    /// First line starting with "CNCA" yields the pair number.
    /// </summary>
    public static string ParseInstallResult(string[] lines)
    {
        foreach (var line in lines)
        {
            if (line.StartsWith("CNCA", StringComparison.OrdinalIgnoreCase))
            {
                var space = line.IndexOf(' ');
                if (space > 4)
                    return line[4..space];
            }
        }
        throw new SetupException("Could not determine new pair number from install output.");
    }

    /// <summary>
    /// Parses CNCA&lt;n&gt; or CNCB&lt;n&gt; into (key, side).
    /// Returns false if the format does not match.
    /// </summary>
    public static bool TryParsePortId(string id, out string key, out int side)
    {
        key = "";
        side = 0;

        if (id.StartsWith("CNCA", StringComparison.OrdinalIgnoreCase) && id.Length > 4)
        {
            key = id[4..];
            side = 0;
            return true;
        }
        if (id.StartsWith("CNCB", StringComparison.OrdinalIgnoreCase) && id.Length > 4)
        {
            key = id[4..];
            side = 1;
            return true;
        }
        return false;
    }
}

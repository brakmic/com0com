namespace com0com.Setup.Tests;

public class PortParamsTests
{
    [Fact]
    public void Constructor_Default_CreatesEmptyCollection()
    {
        var p = new PortParams();
        Assert.Equal(0, p.Count);
    }

    [Fact]
    public void Constructor_ParsesKeyValueString_WithSinglePair()
    {
        var p = new PortParams("PortName=COM3");
        Assert.Equal("COM3", p["PortName"]);
        Assert.Equal(1, p.Count);
    }

    [Fact]
    public void Constructor_ParsesMultipleKeyValuePairs()
    {
        var p = new PortParams("PortName=COM3,EmuBR=yes,EmuOverrun=no");
        Assert.Equal("COM3", p["PortName"]);
        Assert.Equal("yes", p["EmuBR"]);
        Assert.Equal("no", p["EmuOverrun"]);
        Assert.Equal(3, p.Count);
    }

    [Fact]
    public void Constructor_IgnoresEmptyInput()
    {
        var p = new PortParams("");
        Assert.Equal(0, p.Count);

        var p2 = new PortParams("   ");
        Assert.Equal(0, p2.Count);
    }

    [Fact]
    public void Constructor_IgnoresMalformedEntriesWithoutEquals()
    {
        var p = new PortParams("garbage,PortName=COM3,junk");
        Assert.Equal("COM3", p["PortName"]);
        Assert.Equal(1, p.Count);
    }

    [Fact]
    public void Constructor_IgnoresEntriesWithEmptyKey()
    {
        var p = new PortParams("=value,PortName=COM3");
        Assert.Equal("COM3", p["PortName"]);
        Assert.Equal(1, p.Count);
    }

    [Fact]
    public void Indexer_Get_ReturnsNullForMissingKey()
    {
        var p = new PortParams();
        Assert.Null(p["Nonexistent"]);
    }

    [Fact]
    public void Indexer_Get_IsCaseInsensitive()
    {
        var p = new PortParams("PortName=COM3");
        Assert.Equal("COM3", p["portname"]);
        Assert.Equal("COM3", p["PORTNAME"]);
        Assert.Equal("COM3", p["PortName"]);
    }

    [Fact]
    public void Indexer_Set_StoresValue()
    {
        var p = new PortParams();
        p["EmuBR"] = "yes";
        Assert.Equal("yes", p["EmuBR"]);
        Assert.Equal(1, p.Count);
    }

    [Fact]
    public void Indexer_Set_NullRemovesKey()
    {
        var p = new PortParams("EmuBR=yes");
        p["EmuBR"] = null;
        Assert.Null(p["EmuBR"]);
        Assert.Equal(0, p.Count);
    }

    [Fact]
    public void Indexer_Set_OverwritesExistingValue()
    {
        var p = new PortParams("EmuBR=yes");
        p["EmuBR"] = "no";
        Assert.Equal("no", p["EmuBR"]);
        Assert.Equal(1, p.Count);
    }

    [Fact]
    public void ToParamString_ReturnsDash_WhenEmpty()
    {
        var p = new PortParams();
        Assert.Equal("-", p.ToParamString());
    }

    [Fact]
    public void ToParamString_FormatsKeyValuePairs()
    {
        var p = new PortParams("PortName=COM3,EmuBR=yes");
        var result = p.ToParamString();
        Assert.Contains("PortName=COM3", result);
        Assert.Contains("EmuBR=yes", result);
        Assert.Contains(",", result);
    }

    [Fact]
    public void RoundTrip_ParseThenFormat_ProducesEquivalentData()
    {
        var original = "PortName=COM5,EmuBR=yes,EmuOverrun=no,cts=RRTS";
        var parsed = new PortParams(original);
        var formatted = parsed.ToParamString();

        // Reparse and verify all keys match
        var reparsed = new PortParams(formatted);
        Assert.Equal(parsed["PortName"], reparsed["PortName"]);
        Assert.Equal(parsed["EmuBR"], reparsed["EmuBR"]);
        Assert.Equal(parsed["EmuOverrun"], reparsed["EmuOverrun"]);
        Assert.Equal(parsed["cts"], reparsed["cts"]);
    }
}

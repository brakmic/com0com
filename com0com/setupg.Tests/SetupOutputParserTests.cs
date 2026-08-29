namespace com0com.Setup.Tests;

public class SetupOutputParserTests
{
    [Fact]
    public void ParseListAll_ParsesSinglePair()
    {
        var lines = new[]
        {
            "CNCA0 PortName=COM3,EmuBR=yes",
            "CNCB0 PortName=COM4,EmuBR=no",
        };

        var pairs = SetupOutputParser.ParseListAll(lines);

        Assert.Single(pairs);
        var pair = pairs["0"];
        Assert.Equal("0", pair.PairKey);
        Assert.Equal("COM3", pair.PortA["PortName"]);
        Assert.Equal("yes", pair.PortA["EmuBR"]);
        Assert.Equal("COM4", pair.PortB["PortName"]);
        Assert.Equal("no", pair.PortB["EmuBR"]);
    }

    [Fact]
    public void ParseListAll_ParsesMultiplePairs()
    {
        var lines = new[]
        {
            "CNCA0 PortName=COM3",
            "CNCB0 PortName=COM4",
            "CNCA1 PortName=COM5",
            "CNCB1 PortName=COM6",
        };

        var pairs = SetupOutputParser.ParseListAll(lines);

        Assert.Equal(2, pairs.Count);
        Assert.True(pairs.ContainsKey("0"));
        Assert.True(pairs.ContainsKey("1"));
        Assert.Equal("COM3", pairs["0"].PortA["PortName"]);
        Assert.Equal("COM5", pairs["1"].PortA["PortName"]);
        Assert.Equal("COM6", pairs["1"].PortB["PortName"]);
    }

    [Fact]
    public void ParseListAll_ParsesPairWithFullPortParams()
    {
        var lines = new[]
        {
            "CNCA2 PortName=COM7,EmuBR=no,EmuOverrun=no,PlugInMode=yes,ExclusiveMode=no,HiddenMode=no,cts=RRTS,dsr=RDTR,dcd=,ri=LRTS",
        };

        var pairs = SetupOutputParser.ParseListAll(lines);

        var port = pairs["2"].PortA;
        Assert.Equal("COM7", port["PortName"]);
        Assert.Equal("no", port["EmuBR"]);
        Assert.Equal("yes", port["PlugInMode"]);
        Assert.Equal("RRTS", port["cts"]);
        Assert.Equal("RDTR", port["dsr"]);
        Assert.Equal("", port["dcd"]);
        Assert.Equal("LRTS", port["ri"]);
    }

    [Fact]
    public void ParseListAll_ReturnsAOnly_WhenSideBIsMissing()
    {
        var lines = new[] { "CNCA0 PortName=COM3" };

        var pairs = SetupOutputParser.ParseListAll(lines);

        Assert.Single(pairs);
        Assert.Equal("COM3", pairs["0"].PortA["PortName"]);
        Assert.Equal(0, pairs["0"].PortB.Count);
    }

    [Fact]
    public void ParseListAll_IgnoresLinesWithoutSpace()
    {
        var lines = new[] { "garbage", "CNCA0 PortName=COM3" };

        var pairs = SetupOutputParser.ParseListAll(lines);

        Assert.Single(pairs);
    }

    [Fact]
    public void ParseListAll_IgnoresLinesWithUnrecognizedPrefix()
    {
        var lines = new[]
        {
            "XYZA0 PortName=COM3",
            "CNCA0 PortName=COM5",
        };

        var pairs = SetupOutputParser.ParseListAll(lines);

        Assert.Single(pairs);
        Assert.Equal("COM5", pairs["0"].PortA["PortName"]);
    }

    [Fact]
    public void ParseListAll_HandlesEmptyInput()
    {
        var pairs = SetupOutputParser.ParseListAll([]);
        Assert.Empty(pairs);
    }

    [Fact]
    public void ParseListAll_BuildsPairKeyFromPortIdSuffix()
    {
        // Port IDs like CNCA42, CNCB42 -> pair key "42"
        var lines = new[]
        {
            "CNCA42 PortName=COM10",
            "CNCB42 PortName=COM11",
        };

        var pairs = SetupOutputParser.ParseListAll(lines);

        Assert.Single(pairs);
        Assert.True(pairs.ContainsKey("42"));
        Assert.Equal("42", pairs["42"].PairKey);
    }

    [Fact]
    public void ParseInstallResult_ExtractsPairNumber()
    {
        var lines = new[] { "CNCA3 PortName=COM7,EmuBR=yes" };
        var key = SetupOutputParser.ParseInstallResult(lines);
        Assert.Equal("3", key);
    }

    [Fact]
    public void ParseInstallResult_SkipsNonCncaPrefixes()
    {
        var lines = new[]
        {
            "Setup starting...",
            "CNCB1 PortName=COM8",
            "CNCA5 PortName=COM9",
        };
        var key = SetupOutputParser.ParseInstallResult(lines);
        Assert.Equal("5", key);
    }

    [Fact]
    public void ParseInstallResult_Throws_WhenNoCncaFound()
    {
        var lines = new[] { "Setup failed", "no ports" };
        var ex = Assert.Throws<SetupException>(() => SetupOutputParser.ParseInstallResult(lines));
        Assert.Contains("Could not determine", ex.Message);
    }

    [Fact]
    public void ParseInstallResult_Throws_WhenEmptyOutput()
    {
        var ex = Assert.Throws<SetupException>(() => SetupOutputParser.ParseInstallResult([]));
        Assert.Contains("Could not determine", ex.Message);
    }

    [Theory]
    [InlineData("CNCA0", "0", 0)]
    [InlineData("CNCB0", "0", 1)]
    [InlineData("CNCA42", "42", 0)]
    [InlineData("CNCB99", "99", 1)]
    [InlineData("cnca7", "7", 0)]
    [InlineData("cncb7", "7", 1)]
    public void TryParsePortId_ParsesValidIds(string id, string expectedKey, int expectedSide)
    {
        var ok = SetupOutputParser.TryParsePortId(id, out var key, out var side);
        Assert.True(ok);
        Assert.Equal(expectedKey, key);
        Assert.Equal(expectedSide, side);
    }

    [Theory]
    [InlineData("XYZ0")]
    [InlineData("CNC0")]
    [InlineData("CNCA")]
    [InlineData("")]
    [InlineData("   ")]
    public void TryParsePortId_ReturnsFalse_ForInvalidIds(string id)
    {
        var ok = SetupOutputParser.TryParsePortId(id, out _, out _);
        Assert.False(ok);
    }
}

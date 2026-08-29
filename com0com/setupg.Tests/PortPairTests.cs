namespace com0com.Setup.Tests;

public class PortPairTests
{
    [Fact]
    public void Constructor_CreatesEmptyPortAAndPortB()
    {
        var pair = new PortPair();
        Assert.NotNull(pair.PortA);
        Assert.NotNull(pair.PortB);
        Assert.True(pair.IsEmpty);
    }

    [Fact]
    public void IsEmpty_ReturnsTrue_WhenBothSidesEmpty()
    {
        var pair = new PortPair();
        Assert.True(pair.IsEmpty);
    }

    [Fact]
    public void IsEmpty_ReturnsFalse_WhenPortAHasData()
    {
        var pair = new PortPair();
        pair.PortA["PortName"] = "COM3";
        Assert.False(pair.IsEmpty);
    }

    [Fact]
    public void IsEmpty_ReturnsFalse_WhenPortBHasData()
    {
        var pair = new PortPair();
        pair.PortB["PortName"] = "COM4";
        Assert.False(pair.IsEmpty);
    }

    [Fact]
    public void PairKey_StoresAndRetrieves()
    {
        var pair = new PortPair { PairKey = "5" };
        Assert.Equal("5", pair.PairKey);
    }

    [Fact]
    public void Indexer_Side0_ReturnsPortA()
    {
        var pair = new PortPair();
        pair.PortA["PortName"] = "COM3";
        Assert.Same(pair.PortA, pair[0]);
        Assert.Equal("COM3", pair[0]["PortName"]);
    }

    [Fact]
    public void Indexer_Side1_ReturnsPortB()
    {
        var pair = new PortPair();
        pair.PortB["PortName"] = "COM4";
        Assert.Same(pair.PortB, pair[1]);
        Assert.Equal("COM4", pair[1]["PortName"]);
    }

    [Fact]
    public void PortA_CanBeReplaced()
    {
        var pair = new PortPair();
        var newParams = new PortParams("PortName=COM10");
        pair.PortA = newParams;
        Assert.Same(newParams, pair.PortA);
    }

    [Fact]
    public void PortB_CanBeReplaced()
    {
        var pair = new PortPair();
        var newParams = new PortParams("PortName=COM11");
        pair.PortB = newParams;
        Assert.Same(newParams, pair.PortB);
    }
}

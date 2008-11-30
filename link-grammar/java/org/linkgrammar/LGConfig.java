package org.linkgrammar;

/**
 * 
 * <p>
 * A plain Java bean to hold configuration of the Link Grammar parser. Some configuration
 * parameters are not really passed onto the parser, but applied only when constructing
 * a <code>ParseResult</code>. Those are <code>maxLinkages</code> and <code>allowSkippedWords</code>. 
 * </p>
 *
 * @author Borislav Iordanov
 *
 */
public class LGConfig
{
	private int maxLinkages = 25;
	private int maxParseSeconds = 60;
	private int maxCost = -1;
	private boolean allowSkippedWords = true;
	private String dictionaryLocation = null;
	
	public int getMaxLinkages()
	{
		return maxLinkages;
	}
	public void setMaxLinkages(int maxLinkages)
	{
		this.maxLinkages = maxLinkages;
	}
	public int getMaxParseSeconds()
	{
		return maxParseSeconds;
	}
	public void setMaxParseSeconds(int maxParseSeconds)
	{
		this.maxParseSeconds = maxParseSeconds;
	}
	public int getMaxCost()
	{
		return maxCost;
	}
	public void setMaxCost(int maxCost)
	{
		this.maxCost = maxCost;
	}
	public boolean isAllowSkippedWords()
	{
		return allowSkippedWords;
	}
	public void setAllowSkippedWords(boolean allowSkippedWords)
	{
		this.allowSkippedWords = allowSkippedWords;
	}
	public String getDictionaryLocation()
	{
		return dictionaryLocation;
	}
	public void setDictionaryLocation(String dictionaryLocation)
	{
		this.dictionaryLocation = dictionaryLocation;
	}	
}
package org.linkgrammar;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * 
 * <p>
 * Represents the result of parsing a piece of text. The result consists of some global
 * meta information about the whole parse and a list of <code>Linkage</code>s returned
 * by the parser. The original parsed text is available as the <code>text</code> attribute
 * and a tokenized version as the <code>String[] words</code> attribute.  
 * </p>
 *
 * @author Borislav Iordanov
 *
 */
public class ParseResult implements Iterable<Linkage>
{
	String text;
	String [] words;
	boolean [] entityFlags;
	boolean [] pastTenseFlags;
	List<Linkage> linkages = new ArrayList<Linkage>();
	int numSkippedWords;
	
	public String wordAt(int i)
	{
		return words[i];
	}
	
	public boolean isPastTenseForm(int i)
	{
		return pastTenseFlags[i];
	}
	
	public boolean isEntity(int i)
	{
		return entityFlags[i];
	}
	
	public Iterator<Linkage> iterator()
	{
		return linkages.iterator();
	}

	public List<Linkage> getLinkages()
	{
		return linkages;
	}
	
	public String getText()
	{
		return text;
	}

	public void setText(String text)
	{
		this.text = text;
	}

	public String[] getWords()
	{
		return words;
	}

	public void setWords(String[] words)
	{
		this.words = words;
	}

	public boolean[] getEntityFlags()
	{
		return entityFlags;
	}

	public void setEntityFlags(boolean[] entityFlags)
	{
		this.entityFlags = entityFlags;
	}

	public boolean[] getPastTenseFlags()
	{
		return pastTenseFlags;
	}

	public void setPastTenseFlags(boolean[] pastTenseFlags)
	{
		this.pastTenseFlags = pastTenseFlags;
	}

	public int getNumSkippedWords()
	{
		return numSkippedWords;
	}

	public void setNumSkippedWords(int numSkippedWords)
	{
		this.numSkippedWords = numSkippedWords;
	}		
}
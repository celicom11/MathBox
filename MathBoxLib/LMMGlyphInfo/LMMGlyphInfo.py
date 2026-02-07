import json
from fontTools.ttLib import TTFont
import csv
import os

def get_eGlyphClass(codepoint):
    """
Ord = 0,LOP,Bin,Rel,Open,Close,Punct,Accent,Over,Under 
    """
    # Large operators
    if codepoint in (0x2140,0x220F,0x2210,0x2211,0x222B,0x222C,0x222D,0x222E,0x222F,0x2230,0x2231,0x2232,0x2233,0x22C0,0x22C1,0x22C2,
                     0x22C3,0x27D8,0x27D9,0x2A00,0x2A01,0x2A02,0x2A03,0x2A04,0x2A05,0x2A06,0x2A09,0x2A11):
        return 1
    
    # Binary operators
    if codepoint in (0x002B,0x00B1, 
                     0x00D7,0x00F7,0x2020,0x2021,0x2022,0x2040,0x2044,0x214B,0x2212,0x2213,0x2214,0x2215,0x2216,0x2217,0x2218,0x2219,
                     0x2227,0x2228,0x2229,0x222A,0x2238,0x223E,0x2240,0x228C,0x228D,0x228E,0x2293,0x2294,0x2295,0x2296,0x2297,0x2298,
                     0x2299,0x229A,0x229B,0x229C,0x229D,0x229E,0x229F,0x22A0,0x22A1,0x22BA,0x22BB,0x22BC,0x22BD,0x22C4,0x22C5,0x22C6,
                     0x22C7,0x22C9,0x22CA,0x22CB,0x22CC,0x22CE,0x22CF,0x22D2,0x22D3,0x2305,0x2306,0x233D,0x25B3,0x25B7,0x25C1,0x25CB,
                     0x25EB,0x27C7,0x27D1,0x27E0,0x27E1,0x27E2,0x27E3,0x2A2F,0x2A3F):
        return 2
    
    # Relations
    if codepoint in (0x003C,0x003D,0x003E,0x2050,0x2208,0x2209,0x220A,0x220B,0x220C,0x220D,0x221D,0x22F0,0x22F1,
                     0x2322,0x2323,0x27C2,0x27DA,0x27DB,0x27DC,0x27DD,0x27DE,
                     0x2906,0x2907,0x2A95,0x2A96,0x2AAF,0x2AB0,0x2B31,0x2B33):
        return 3
    if (codepoint >= 0x2190 and codepoint <= 0x21F6) or (codepoint >= 0x2223 and codepoint <= 0x2226) or\
    (codepoint >= 0x2236 and codepoint <= 0x223D) or (codepoint >= 0x2241 and codepoint <= 0x2292) or\
    (codepoint >= 0x22A2 and codepoint <= 0x22B8) or (codepoint >= 0x22C8 and codepoint <= 0x22EE) or\
    (codepoint >= 0x27F4 and codepoint <= 0x27FF) or (codepoint >= 0x2A85 and codepoint <= 0x2A8C):
        return 3
    # Open delimiters
    if codepoint in (0x0028,0x005B,0x007B,0x2308,0x230A,0x231C,0x231E,0x23B0,0x2772,0x27C5,0x27CC,
                     0x27E6,0x27E8,0x27EA,0x27EC,0x27EE): 
        return 4 

    # Close delimiters  
    if codepoint in (0x0029,0x005D,0x007D,0x2309,0x230B,0x231D,0x231F,0x23B1,0x2773,0x27C6,0x27E7,0x27E9,0x27EB,0x27EF):
        return 5
    
    # Punctuation
    if codepoint in (0x002C,0x003A,0x003B):
        return 6
    
    # Accents
    if codepoint in (0x0300,0x0301,0x0302,0x0303,0x0304,0x0305,0x0306,0x0307,0x0308,0x0309,0x030A,0x030C,0x0310,
                     0x0312,0x0315,0x031A) or (codepoint >= 0x20D0 and codepoint<= 0x20EF):
        return 7
    
    # Over,NB: x23E0 is ADDED BY ME!
    if codepoint in (0x23B4,0x23DC,0x23DE,0x23E0):
        return 8
    
    # Under,x23E1 is ADDED BY ME!
    if codepoint in (0x23B5,0x23DD,0x23DF,0x23E1):
        return 9

    # Default to ordinary
    return 0  # etcORD

def main():
    # File paths - adjust as needed
    latex_unicode_path = 'latex-unicode.json' 
    font_path = 'latinmodern-math.otf'
    output_csv = 'LatinModernMathGlyphs.csv'
    
    print("Loading LaTeX-Unicode mapping...")
    with open(latex_unicode_path, 'r', encoding='utf-8') as f:
        latex_unicode_map = json.load(f)
    
    print("Loading font...")
    font = TTFont(font_path)
    
    # Get glyph order (index -> name mapping)
    glyphOrder = font.getGlyphOrder()
    
    # Create reverse mapping: UTF-8 character -> LaTeX macro
    utf8_to_latex = {v: k for k, v in latex_unicode_map.items()}
    
    # Get Unicode -> glyph name mapping
    cmap = font.getBestCmap()
    
    # Extract MATH table information
    print("Extracting MATH table data...")
    italic_corrections = {}
    top_accent_x = {}
    
    if 'MATH' in font:
        math_table = font['MATH']
        mathGlyphInfo = math_table.table.MathGlyphInfo
        # Extract italic corrections
        if mathGlyphInfo.MathItalicsCorrectionInfo:
            coverage = mathGlyphInfo.MathItalicsCorrectionInfo.Coverage
            values = mathGlyphInfo.MathItalicsCorrectionInfo.ItalicsCorrection
            # values is an array where the position (0,1,2...) corresponds to 
            # the same position in coverage.glyphs array
            for idx, ital_correction in enumerate(values):
                glyph_name = coverage.glyphs[idx]  # idx is the position in coverage.glyphs
                glyph_id = font.getGlyphID(glyph_name)
                italic_corrections[glyph_id] = int(ital_correction.Value)
        # Extract top accent attachment
        if mathGlyphInfo.MathTopAccentAttachment:
            coverage = mathGlyphInfo.MathTopAccentAttachment.TopAccentCoverage
            values = mathGlyphInfo.MathTopAccentAttachment.TopAccentAttachment
            for idx, top_accent in enumerate(values):
                glyph_name = coverage.glyphs[idx]  # idx is the position in coverage.glyphs
                glyph_id = font.getGlyphID(glyph_name)
                top_accent_x[glyph_id] = int(top_accent.Value)
    
    print(f"Writing CSV file: {output_csv}")
    
    # Generate CSV
    with open(output_csv, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile)
        
        # Write header - CORRECTED COLUMN ORDER
        writer.writerow(['nCodePoint', 'sName', 'sLaTex', 'eGlyphClass', 'nTopAccentX', 'nItalicCorr'])
        
        # Process each glyph
        for glyphIndex, glyphName in enumerate(glyphOrder):
            # Find Unicode codepoint for this glyph
            codepoints = [cp for cp, name in cmap.items() if name == glyphName]
            nCodePoint = codepoints[0] if codepoints else 0
            
            # Format codepoint as hex string or 0
            if nCodePoint != 0:
                codepoint_str = f"x{nCodePoint:X}"
            else:
                codepoint_str = "0"
            
            # Find LaTeX macro from UTF-8 character
            if nCodePoint != 0:
                try:
                    char = chr(nCodePoint)
                    # Handle HTML entities in JSON
                    if char == '&':
                        char = '&'
                    elif char == '<':
                        char = '<'  
                    elif char == '>':
                        char = '>'
                    sLaTex = utf8_to_latex.get(char, "")
                except ValueError:
                    sLaTex = ""
            else:
                sLaTex = ""
            
            # Remove backslash from LaTeX macro
            if sLaTex.startswith("\\"):
                sLaTex = sLaTex[1:]
            
            # Determine TeX class
            eGlyphClass = get_eGlyphClass(nCodePoint)
            
            # Get MATH table values
            nTopAccentX = top_accent_x.get(glyphIndex, 0)
            nItalicCorr = italic_corrections.get(glyphIndex, 0)
            
            # Handle sName - empty for uniXXXX names
            if glyphName.startswith('uni') and len(glyphName) == 7:  # uniXXXX format
                sName = ""
            elif glyphName.startswith('u1') and len(glyphName) == 6:  # u1XXXX format
                sName = ""
            elif glyphName.startswith('u2') and len(glyphName) == 6:  # u1XXXX format
                sName = ""
            else:
                sName = glyphName
            
            # Write row with corrected column order
            writer.writerow([codepoint_str, sName, sLaTex, eGlyphClass, nTopAccentX, nItalicCorr])
            
            # Progress indicator
            if glyphIndex % 100 == 0:
                print(f"Processed {glyphIndex} glyphs...")
    
    print(f"Successfully generated {output_csv} with {len(glyphOrder)} glyphs")

if __name__ == "__main__":
    main()


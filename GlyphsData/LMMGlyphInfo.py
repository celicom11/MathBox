import json
from fontTools.ttLib import TTFont
import csv
import os

def get_eTexClass(glyph_name, codepoint):
    """
    Heuristic to determine TeX class from glyph name and Unicode codepoint.
    Returns: 0-ORD(default), 1-(Large)Op, 2-Bin, 3-Rel, 4-Delimiter (Open/Close), 
             5-Punct, 6-Over, 7-Under, 8-Top Accent, 9-Radical, 10-Glue/Skip
    """
    name_lower = glyph_name.lower()
    
    # Large operators
    if any(x in name_lower for x in ["sum", "prod", "int", "oint", "bigcap", "bigcup", "bigwedge", "bigvee"]):
        return 1  # Op
    
    # Binary operators
    if any(x in name_lower for x in ["plus", "minus", "times", "div", "ast", "star", "cdot", "bullet", "oplus", "otimes", "wedge", "vee", "cap", "cup"]):
        return 2  # etcBIN
    
    # Relations
    if any(x in name_lower for x in ["equal", "lt", "gt", "le", "ge", "neq", "equiv", "sim", "approx", "subset", "supset", "in", "ni", "parallel", "perp"]):
        return 3  # etcREL
    
    # Open delimiters
    if any(x in name_lower for x in ["lparen", "lbrack", "lbrace", "langle", "lceil", "lfloor"]) or glyph_name in ["(", "[", "{"]:
        return 4  # etcOPEN
    
    # Close delimiters  
    if any(x in name_lower for x in ["rparen", "rbrack", "rbrace", "rangle", "rceil", "rfloor"]) or glyph_name in [")", "]", "}"]:
        return 5  # etcCLOSE
    
    # Punctuation
    if any(x in name_lower for x in ["period", "comma", "colon", "semicolon", "dots", "ldots", "cdots"]) or glyph_name in [".", ",", ";", ":"]:
        return 6  # etcPUNCT
    
    # Accents
    if any(x in name_lower for x in ["hat", "bar", "vec", "tilde", "dot", "ddot", "acute", "grave", "breve", "check", "ring"]):
        return 10  # etcACCENT
    
    # Radicals
    if any(x in name_lower for x in ["radical", "surd", "sqrt"]):
        return 11  # etcRADI
    
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
        writer.writerow(['nCodePoint', 'sName', 'sLaTex', 'eTexClass', 'nTopAccentX', 'nItalicCorr'])
        
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
                    sLaTex = utf8_to_latex.get(char, "0")
                except ValueError:
                    sLaTex = "0"
            else:
                sLaTex = "0"
            
            # Remove backslash from LaTeX macro
            if sLaTex != "0" and sLaTex.startswith("\\"):
                sLaTex = sLaTex[1:]
            
            # Determine TeX class
            eTexClass = get_eTexClass(glyphName, nCodePoint)
            
            # Get MATH table values
            nTopAccentX = top_accent_x.get(glyphIndex, 0)
            nItalicCorr = italic_corrections.get(glyphIndex, 0)
            
            # Handle sName - empty for uniXXXX names
            if nCodePoint == 0 and glyphName.startswith('uni') and len(glyphName) == 7:  # uniXXXX format
                sName = ""
            elif nCodePoint == 0 and glyphName.startswith('u1') and len(glyphName) == 6:  # u1XXXX format
                sName = ""
            else:
                sName = glyphName
            
            # Write row with corrected column order
            writer.writerow([codepoint_str, sName, sLaTex, eTexClass, nTopAccentX, nItalicCorr])
            
            # Progress indicator
            if glyphIndex % 100 == 0:
                print(f"Processed {glyphIndex} glyphs...")
    
    print(f"Successfully generated {output_csv} with {len(glyphOrder)} glyphs")

if __name__ == "__main__":
    main()


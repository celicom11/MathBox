# MathBox

A C++/DirectWrite-based library that parses and renders a comprehensive subset of $\LaTeX$ mathematical expressions directly to screen.

## Motivation

I was looking for a lightweight, open-source library to read and render TeX/LaTeX scripts directly on screen for Windows applications. While several excellent TeX processors exist (KaTeX, LuaLaTeX, pdfTeX, XeLaTeX) and [parsers that generate HTML+MathML](https://tex.stackexchange.com/questions/4223/what-parsers-for-latex-mathematics-exist-outside-of-the-tex-engines), none render $\TeX/\LaTeX$ scripts directly to screen with native Windows integration.

This project provides a **MathBox** library with C-ABI interface for cross-platform integration, a Windows demonstration application, and comprehensive unit tests. Future goals include selection support and copy-to-clipboard functionality for rendered mathematical formulas.

## Project Structure

The solution consists of three main components:

- **[MathBoxLib](ReadMeLib.md)** - Platform-independent C++ library
  - Core LaTeX parser and typesetting engine
  - Macro system with parameter substitution
  - C-ABI interface for cross-language integration
  - Platform-agnostic rendering interfaces
  
- **[MathBoxDemo](ReadMeDemo.md)** - Windows demo application
  - DirectWrite rendering implementation
  - Configuration-based document loading
  - Font management and display
  
- **[MathBoxTest](ReadMeTests.md)** - Unit test suite
  - ~80+ test methods with ~70% code coverage
  - MSTest-based automated testing
  - Parser, tokenizer, builder, and macro tests

**See detailed documentation:**
- [ReadMeLib.md](ReadMeLib.md) - Library architecture, APIs, and implementation details
- [ReadMeDemo.md](ReadMeDemo.md) - Demo app usage, configuration, and integration guide
- [ReadMeTests.md](ReadMeTests.md) - Test suite description and coverage analysis

## Concepts and References

This project requires deep understanding of $\TeX$ and OpenType font typesetting concepts. Key references used throughout development:

- [[1]](https://visualmatheditor.equatheque.net/doc/texbook.pdf) *"The $\TeX$book"* by Donald Knuth (1986) - foundational reference
- [[2]](https://www.tug.org/TUGboat/tb27-1/tb86jackowski.pdf) *"Appendix G Illuminated"* by Bogusław Jackowski - practical TeX implementation
- [[3]](https://www.ntg.nl/maps/38/03.pdf) *"OpenType Math Illuminated"* by Ulrik Vieth - OpenType MATH specification
- [[4]](https://learn.microsoft.com/en-us/typography/opentype/spec/math) *MATH - The Mathematical Typesetting Table* - Microsoft Typography
- [[5]](https://w3c.github.io/mathml-core/) *MathML Core Specifications* - W3C standards
- [[6]](https://fred-wang.github.io/MathFonts/LatinModern/) *Latin Modern Math Font Glyphs* - comprehensive glyph documentation
- [[7]](https://github.com/ViktorQvarfordt/unicode-latex) JSON mapping of LaTeX commands to **LMM** glyphs

## Architecture Overview

### Core Components

**MathBoxLib (Platform-Independent):**
- Multi-stage LaTeX parser (tokenization → macro expansion → grouping → building)
- Macro system: `\newcommand`, `\let`, `\setlength` with parameter substitution
- MathItem hierarchy: Words, Fractions, Radicals, Accents, Delimiters, Tables, etc.
- TeX typesetting engine following TeXbook specifications
- C-ABI interface for cross-language integration

**Parser Pipeline:**
1. **Tokenization** - Convert input to tokens with position tracking
2. **Macro Loading** - Load external macro definition files
3. **Macro Expansion** - Expand user-defined macros recursively
4. **Grouping** - Identify balanced braces and nested structures
5. **Building** - Construct MathItem tree via specialized builders

**Rendering Abstraction:**
- `IDocParams` - Document parameters (fonts, sizes, colors)
- `IFontManager` - Font provider interface
- `IDocRenderer` - Platform-agnostic rendering (lines, rectangles, glyph runs)

### Feature Highlights

**✅ Fully Implemented:**
- Comprehensive LaTeX tokenizer with error reporting
- Macro system with nesting and parameter substitution
- Math mode (`$...$`, `$$...$$`) and text mode
- Fractions, radicals, superscripts/subscripts
- Accents, delimiters, operators
- Tables, matrices, and arrays
- Multiple fonts (Roman, Bold, Italic, Sans, Typewriter)
- ~70% test coverage with 80+ unit tests

**🚧 In Progress:**
- Overlay/decoration commands (`\boxed`, `\underline`, `\cancel`)

**📋 Planned:**
- Selection and clipboard support
- Multi-line environments (`equation`, `align`)
- Color support
- Additional symbols and fonts

## How to Build

**Prerequisites:**
- Visual Studio 2022 (or later)
- Windows 10/11 with DirectWrite support
- C++14 compiler

**Quick Start:**
1. Clone the repository
2. Open `DemoApp.vcxproj` in Visual Studio 2022
3. Build Solution (Ctrl+Shift+B) - builds all three projects
4. Run **MathBoxDemo** (F5) to see the demo application
5. Run **MathBoxTest** via Test Explorer to verify functionality

**Required Runtime Files:**
- `LatinModernFonts/` - Font files (.otf)
- `LatinModernMathGlyphs.csv` - Glyph mapping table
- `MathBoxDemo.cfg` - Configuration file
- `TestDoc.txt` - Sample LaTeX document
- `Macros.mth` - Macro definitions (required)

**Detailed Instructions:**
- **Library**: See [ReadMeLib.md](ReadMeLib.md) for platform-independent build and integration
- **Demo App**: See [ReadMeDemo.md](ReadMeDemo.md) for configuration and usage
- **Testing**: See [ReadMeTests.md](ReadMeTests.md) for running and writing tests

## Contributing

Contributions are welcome! Please ensure changes maintain compatibility with:
- TeXbook specifications [[1]](#references)
- OpenType MATH table requirements [[3]](#references), [[4]](#references)
- MathML Core recommendations [[5]](#references)

**Coding Standards:**
- Hungarian notation (no exceptions)
- C++ class template:
  ```cpp
  class C{ClassName} : public IInterface  // if any
  {
  //TYPES - optional internal types
  //DATA - private by default
  public:
  //CTOR/DTOR/INIT
  //ATTS - Attributes/getters
  //METHODS
  //IInterface implementation - if any
  private:
  //internal methods with trailing underscore (e.g., Helper_)
  };
  ```
- Structures only for POD types or abstract interfaces
- Comprehensive unit tests for new features

## License

MIT License (under consideration)

---

**For detailed documentation, see:**
- [ReadMeLib.md](ReadMeLib.md) - Library architecture and APIs
- [ReadMeDemo.md](ReadMeDemo.md) - Demo application usage
- [ReadMeTests.md](ReadMeTests.md) - Unit testing guide

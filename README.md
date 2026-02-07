# MathBox

A C++/DirectWrite-based library that parses and renders a comprehensive subset of $\LaTeX$ mathematical expressions directly to screen.

## Motivation

I was looking for a lightweight, open-source library to read and render TeX/LaTeX scripts directly on screen for Windows applications. While several excellent TeX processors exist (KaTeX, LuaLaTeX, pdfTeX, XeLaTeX) and [parsers that generate HTML+MathML](https://tex.stackexchange.com/questions/4223/what-parsers-for-latex-mathematics-exist-outside-of-the-tex-engines), none render $\TeX/\LaTeX$ scripts directly to screen with native Windows integration.

This project provides a **MathBox** library with C-ABI interface for cross-platform integration, a Windows demonstration application, and comprehensive unit tests. Future goals include selection support and copy-to-clipboard functionality for rendered mathematical formulas.

## Project Structure

The solution consists of three main components:

- **[MathBoxLib](MathBoxLib/ReadMeLib.md)** - Platform-independent C++ library
  - Core LaTeX parser and typesetting engine
  - Macro system with parameter substitution
  - C-ABI interface for cross-language integration
  - Platform-agnostic rendering interfaces
  
- **[MathBoxDemo](MathBoxDemo/ReadMeDemo.md)** - Windows demo application
  - DirectWrite rendering implementation
  - Configuration-based document loading
  - Font management and display
  
- **[MathBoxTest](MathBoxTests/ReadMeTests.md)** - Unit test suite
- 156+ test methods with ~75% code coverage
- MSTest-based automated testing
- Parser, tokenizer, builder, and macro tests

**See detailed documentation:**
- [MathBoxLib/ReadMeLib.md](MathBoxLib/ReadMeLib.md) - Library architecture, APIs, and implementation details
- [MathBoxDemo/ReadMeDemo.md](MathBoxDemo/ReadMeDemo.md) - Demo app usage, configuration, and integration guide
- [MathBoxTests/ReadMeTests.md](MathBoxTests/ReadMeTests.md) - Test suite description and coverage analysis

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
- Overlay/decoration commands (`\boxed`, `\underline`, `\cancel`)
- Tables, matrices, and arrays
- Multiple fonts (Roman, Bold, Italic, Sans, Typewriter)
- ~75% test coverage with 156+ unit tests

**🚧 In Progress:**
- Releasing v1.0 with core features and demo application

**📋 Planned:**
- Multi-line environments (`equation`, `align`)
- Color text support
- Bra-ket Notation (handling middle | is tricky)
- Extensible Arrows (with or w/o macros)
- siunitx package support (units and spacing)
- Additional fonts (LMM missing some glyphs)

## How to Build

**Prerequisites:**
- Visual Studio 2022 (or later)
- Windows 10/11 with DirectWrite support
- C++14 compiler

**Quick Start:**
1. Clone the repository
2. Open `MathBoxDemo.sln` in Visual Studio 2022
3. Build Solution (Ctrl+Shift+B) - builds all three projects in Debug or Release x64 configuration
   - Builds `MathBoxLib.dll` (the core library with C-ABI interface)
   - Builds `MathBoxDemo.exe` (demonstration application)
   - Builds `MathBoxTest.dll` (unit test suite)
4. Run **MathBoxDemo** (F5) to see the demo application
5. Run **MathBoxTest** via Test Explorer to verify functionality

**Required Runtime Files (for MathBoxDemo):**

Copy to the executable output directory (e.g., `x64\Debug\`):
- `MathBoxLib.dll` - Core library (auto-copied by build)
- `LatinModernFonts/` - Font directory containing:
  - `latinmodern-math.otf`, `lmroman10-regular.otf`, `lmroman10-bold.otf`, `lmroman10-italic.otf`
  - `lmsans10-regular.otf`, `lmmonolt10-regular.otf`
  - **`LatinModernMathGlyphs.csv`** - Glyph mapping table (generated by Python script)
- `MathBoxDemo.cfg` - Configuration file
- `TestDoc.txt` - Sample LaTeX document
- `Macros.mth` - Default macro definitions (required!)

**Using MathBoxLib in Your Own Application:**

To integrate MathBoxLib.dll into your application:
1. Link against `MathBoxLib.lib` (import library)
2. Implement `MBI_FontManager` (must provide Latin Modern fonts from `LatinModernFonts/` directory)
3. Implement `MBI_DocRenderer` (rendering callbacks)
4. Provide `MB_DocParams` (document parameters)
5. Use C-ABI interface via `MB_GetApi()` entry point

See [MathBoxLib/ReadMeLib.md](MathBoxLib/ReadMeLib.md) for complete integration guide.

**Detailed Instructions:**
- **Library**: See [MathBoxLib/ReadMeLib.md](MathBoxLib/ReadMeLib.md) for C-ABI interface, integration requirements, and usage examples
- **Demo App**: See [MathBoxDemo/ReadMeDemo.md](MathBoxDemo/ReadMeDemo.md) for configuration and reference implementation
- **Testing**: See [MathBoxTests/ReadMeTests.md](MathBoxTests/ReadMeTests.md) for running and writing tests

## Contributing

Contributions are welcome! Please ensure changes maintain compatibility with:
- TeXbook specifications [[1]](#concepts-and-references)
- OpenType MATH table requirements [[3]](#concepts-and-references), [[4]](#concepts-and-references)
- MathML Core recommendations [[5]](#concepts-and-references)

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

# Changelog

All notable changes to MathBox will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1-beta] - 2026-02-14

### Improved
- **Error Reporting**: Enhanced macro expansion error messages
  - Now shows actual expanded text instead of macro definitions
  - 3-line structured format for macro errors:
    ```
    Main error message
      Expanded macro: <actual expanded text>
      from macro file(s): <filename(s)>
    ```
  - Position tracking (nStartPos/nEndPos) correctly maps to original user input
  - Nested macro expansions display full expansion chain including user arguments
  - Example: `\outer{1}` → `\sqrt{\ensuremat\frac{1^2}{2}}` shows complete context
- **Fixes**: 
  - added line-breaks support in text groups
  - improved sub/superscript spacing
  - removed extra space on new-lines

### Changed
- Updated documentation (ReadMeDemo.md) with comprehensive error handling examples
- Added 8 new unit tests for macro expansion error scenarios

### Technical
- Refactored `CTexParser::SetError()` to build expanded text from token stream
- Fixed bug where macro definition was shown instead of actual expansion
- Improved error context for deeply nested macro expansions

---

## [1.0-beta] - 2026-02-07

First public beta release of MathBoxLib.

### Added - MathBoxLib
- Complete LaTeX parser with 5-stage pipeline (tokenization, macro loading, expansion, grouping, building)
- Macro system supporting `\newcommand`, `\renewcommand`, `\let`, `\setlength`, `\addtolength`
- Recursive macro expansion with parameter substitution (`#1`-`#9`)
- Comprehensive error reporting with source position tracking through macro expansions
- C-ABI interface for cross-language integration (`MathBox_CAPI.h`)
- Full support for: fractions, radicals, accents, delimiters, operators, tables/matrices
- Dimension parsing for all TeX units: `pt`, `em`, `ex`, `cm`, `mm`, `in`
- Multiple font styles: Roman, Bold, Italic, Sans, Typewriter
- Math and text modes with proper style transitions
- Overlay/decoration commands: `\boxed`, `\underline`, `\overline`, `\cancel`

### Added - MathBoxDemo
- Reference implementation using DirectWrite rendering
- Configuration file support (`MathBoxDemo.cfg`)
- Latin Modern Math font integration with CSV glyph mapping

### Added - MathBoxTests  
- 156+ unit tests with ~75% code coverage
- Comprehensive testing of parser, tokenizer, builders, macro system
- Error case validation for all parsing stages
- Dimension parser tests with edge cases

### Known Limitations
- Windows-only (DirectWrite renderer)
- No scrolling in demo application
- Multi-line environments (`equation`, `align`, `gather`) not yet supported
- No color text commands (`\color`, `\textcolor`)
- Selection and copy-to-clipboard not implemented

### API
- **ABI Version**: 1 (stable)
- **C-ABI**: Frozen for v1.x releases (backward compatible)

---

## [Unreleased]

### Planned for [1.2+] - Q2 2026
- Bug fixes from community testing
- Auto line breaking
- Auto glue sizing/inter-word spacing. 
### Planned for [2.0] - 2027
- Color text support
- Bra-ket notation
- Extensible arrows
- Selection and copy-to-clipboard support
- Basic multi-line environment support
- Enhanced logging/debugging support
  - Add `traceLog` callback to `MB_DocParams` (at end of structure)
  - Diagnostic output for complex parsing scenarios
- **ABI Version**: Will increment to 2 (indicates new features available)
- **Backward Compatible**: Old v1.0 clients continue working with v2.0 DLL

### Future (v2.1+)
- Multi-line equation environments (`equation`, `align`, `gather`)
- siunitx package support
- Additional fonts beyond Latin Modern Math
- Cross-platform renderers (FreeType, Skia, etc.)

---

## Version Numbering

- **MAJOR**: Incremented for new feature sets (always backward compatible)
- **MINOR**: Incremented for bug fixes and minor enhancements (backward compatible)
- **Pre-release**: `-alpha`, `-beta`, `-rc.N` suffixes

### ABI Version Policy
- ABI version = MAJOR version number
- **Backward compatibility guaranteed**: Old clients work with newer DLLs
- New fields added at END of structures (never in middle or remove existing)
- Clients can check `MBI_API::abi_version` to detect available features


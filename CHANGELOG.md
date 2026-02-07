# Changelog

All notable changes to MathBox will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

### Planned for [1.0] - Q2 2026
- Bug fixes from community testing

### Planned for [2.0] - 2027
- Selection and copy-to-clipboard support
- Basic multi-line environment support
- Enhanced logging/debugging support
  - Add `traceLog` callback to `MB_DocParams` (at end of structure)
  - Diagnostic output for complex parsing scenarios
- Additional error recovery modes
- **ABI Version**: Will increment to 2 (indicates new features available)
- **Backward Compatible**: Old v1.0 clients continue working with v2.0 DLL

### Future (v2.1+)
- Multi-line equation environments (`equation`, `align`, `gather`)
- Color text support
- Bra-ket notation
- Extensible arrows
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
- **Backward compatibility guaranteed**: Old clients work with newer DLL
- New fields added at END of structures (never in middle or remove existing)
- Clients can check `MBI_API::abi_version` to detect available features

---

## Upgrade Guide

### From v1.0 to v2.0 (Backward Compatible Enhancement)

**ABI Version Changes**: `abi_version` increments from 1 to 2

**Old v1.0 Clients:**
- ? **No changes required** - continues working with v2.0 DLL
- Simply replace `MathBoxLib.dll` - no recompilation needed
- New features (like `traceLog`) ignored automatically

**New v2.0 Clients (to use new features):**
1. Update `MathBox_CAPI.h` header
2. Recompile against new header
3. Initialize new fields (e.g., `traceLog` callback)

**Example:**
```cpp
// v1.0 client code (still works with v2.0 DLL!)
MB_DocParams params = {};
params.size_bytes = sizeof(MB_DocParams);  // Library detects v1.0 client
params.font_size_pts = 24.0f;
params.font_mgr = myFontManager;
// ... v1.0 fields only
// No traceLog field - library handles gracefully

// v2.0 client code (uses new features)
MB_DocParams params = {};
params.size_bytes = sizeof(MB_DocParams);  // Library detects v2.0 client  
params.font_size_pts = 24.0f;
params.font_mgr = myFontManager;
params.traceLog = MyTraceLogCallback;  // NEW in v2.0 - optional!
// ... other fields
```

**Feature Detection at Runtime:**
```cpp
const MBI_API* pAPI = MB_GetApi();

if (pAPI->abi_version >= 2) {
   // v2.0+ features available (selection, tracing, etc.)
   // Can use traceLog callback
} else {
   // v1.0 features only
}
```

**Backward Compatibility Policy:**
- ? New fields always added at END of structures
- ? Existing fields never removed or reordered
- ? Function signatures never changed
- ? Old return codes/enums remain valid
- ? Never require recompilation of old clients
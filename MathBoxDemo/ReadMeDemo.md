# MathBoxDemo - Windows Demo Application

A Windows desktop application demonstrating the MathBox library integration with DirectWrite rendering.

## Overview

**MathBoxDemo** is a sample application that shows how to integrate the MathBox library into a Windows application. It provides:
- DirectWrite rendering implementation
- Font management via DirectWrite
- Configuration-based document loading
- Real-time LaTeX rendering

## Architecture

### Components

- **CMathBoxHost** - C++ wrapper for MathBox C-API
  - Manages library lifecycle
  - Handles macro loading
  - Provides error reporting
  
- **CD2DRenderer** - DirectWrite rendering implementation
  - Implements `MBI_DocRenderer` interface
  - Handles line drawing, rectangles, and glyph runs
  
- **CD2DFontManager** - Font management
  - Implements `MBI_FontManager` interface
  - Loads Latin Modern Math font family
  - Provides glyph metrics and font indices

### Application Flow

1. Initialize DirectWrite factory
2. Load Latin Modern Math fonts from `LatinModernFonts/` directory
3. Create MathBox engine with document parameters
4. Load macro definitions from `Macros.mth`
5. Parse LaTeX document from `TestDoc.txt`
6. Render to window using DirectWrite

## Configuration

### MathBoxDemo.cfg

Configuration file format (key-value pairs):

```ini
# Document settings
DocPath = TestDoc.txt

# Font size in points
FontSizePts = 24.0

# Maximum width in Font Design Units (0 = no limit)
MaxWidthFDU = 0

# Colors in hexadecimal RRGGBB format
ColorText = 0x004F00      # Dark green text
ColorBkg = 0xFFFFFF       # White background
ColorSelect = 0xFF4FFF    # Pink selection highlight
```

**Configuration Parameters:**

| Parameter     | Type  | Description                          | Default  |
|---------------|-------|--------------------------------------|----------|
| `DocPath`     | string| Path to LaTeX document file          | Required |
| `FontSizePts` | float | Font size in points                  | 24.0     |
| `MaxWidthFDU` | int32 | Max output width in FDU (N/I)        | 0        |
| `ColorText`   | hex   | Text color (0xRRGGBB)                | 0x000000 |
| `ColorBkg`    | hex   | Background color (0xRRGGBB)          | 0xFFFFFF |
| `ColorSelect` | hex   | Selection highlight color (0xRRGGBB) | 0x4040FF |

## Document Files

### TestDoc.txt

UTF-8 encoded LaTeX document. Supports:
- Inline math: `$x^2 + y^2 = r^2$`
- Display math: `$$\int_{-\infty}^{\infty} e^{-x^2} dx = \sqrt{\pi}$$`
- Text mode and math mode mixing
- Multi-line documents
- Comments (lines starting with `%`)

**Example Document:**

```latex
Here is some text with inline math: $E = mc^2$.

Display equation:
$$\frac{-b \pm \sqrt{b^2 - 4ac}}{2a}$$

Matrix example:
$$\begin{pmatrix}
a & b \\
c & d
\end{pmatrix}$$
```

### Macros.mth

LaTeX macro definitions file (required). Contains:

**Basic Command Aliases:**
```latex
\newcommand{\R}{\mathbb{R}}          % Real numbers
\newcommand{\N}{\mathbb{N}}          % Natural numbers
\newcommand{\Z}{\mathbb{Z}}          % Integers
\newcommand{\Q}{\mathbb{Q}}          % Rationals
\newcommand{\C}{\mathbb{C}}          % Complex numbers
```

**Delimiter Aliases:**
```latex
\let\lvert|
\let\rvert|
\newcommand{\lVert}{\Vert}
\newcommand{\rVert}{\Vert}
```

**Custom Macros (Examples):**
```latex
% Absolute value
\newcommand{\abs}[1]{\left|#1\right|}

% Norm
\newcommand{\norm}[1]{\left\lVert #1 \right\rVert}

% Set builder notation
\newcommand{\set}[2]{\left\{#1\mid #2\right\}}

% Binomial coefficient
\newcommand{\binom}[2]{\left(\begin{array}{c}#1\\#2\end{array}\right)}

% Floor and ceiling
\newcommand{\floor}[1]{\left\lfloor #1 \right\rfloor}
\newcommand{\ceil}[1]{\left\lceil #1 \right\rceil}
```

**Supported Macro Commands:**
- `\newcommand{<name>}[<args>]{<body>}` - Define new command
- `\newcommand{<name>}[<args>][<default>]{<body>}` - With optional argument
- `\let\<new>[=]\<existing>` - Create alias
- `\setlength{<var>}{<dimension>}` - Set length variable
- `\addtolength{<var>}{<dimension>}` - Add to length variable

**Parameter Substitution:**
- `#1`, `#2`, ..., `#9` - Positional parameters
- `##1`, `##2`, ... - Future-expansion parameters (for nested definitions)

## How to Build

### Prerequisites
- Visual Studio 2022 (or later)
- Windows 10/11 with DirectWrite support
- Windows SDK

### Build Steps

1. **Open Solution**
   ```
   Open DemoApp.vcxproj in Visual Studio 2022
   ```

2. **Build Configuration**
   - Select configuration: Debug or Release
   - Select platform: x64 (recommended) or x86

3. **Build Solution**
   - Build → Build Solution (Ctrl+Shift+B)
   - This builds both MathBoxLib and MathBoxDemo

4. **Prepare Runtime Files**

   Copy to output directory (e.g., `x64\Debug\`):
   ```
   LatinModernFonts\           (directory with .otf files)
   LatinModernMathGlyphs.csv
   MathBoxDemo.cfg
   TestDoc.txt
   Macros.mth
   ```

5. **Run Application**
   - Debug → Start Debugging (F5)
   - Or run the executable directly

## How to Use

### Basic Integration

```cpp
#include "MathBoxHost.h"
#include "D2DRenderer.h"
#include "D2DFontManager.h"

// 1. Initialize DirectWrite
IDWriteFactory* pDWriteFactory = nullptr;
DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, 
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&pDWriteFactory));

// 2. Setup font manager
CD2DFontManager fontMgr;
fontMgr.Init(L"LatinModernFonts\\", pDWriteFactory);

// 3. Setup renderer
CD2DRenderer renderer;
renderer.Initialize(hwnd);

// 4. Create MathBox host
CMathBoxHost mathBox(fontMgr, renderer);
mathBox.LoadMathBoxLib();

// 5. Configure
mathBox.SetFontSizePts(24.0f);
mathBox.SetMaxWidth(0);  // Unlimited
mathBox.SetClrText(0x000000);
mathBox.SetClrBkg(0xFFFFFF);

// 6. Load macros
string macros;
if (Utf8Utils::_readFile2Utf8(L"Macros.mth", macros)) {
   mathBox.AddMacros(macros, "Macros.mth");
}

// 7. Parse document
string latex;
if (Utf8Utils::_readFile2Utf8(L"TestDoc.txt", latex)) {
   if (!mathBox.Parse(latex)) {
      // Handle error
      auto error = mathBox.LastError();
      MessageBox(nullptr, error.sError.c_str(), L"Parse Error", MB_OK);
   }
}

// 8. Render
void OnPaint() {
   renderer.BeginDraw();
   renderer.EraseBkgnd(mathBox.ClrBkg());
   mathBox.Draw(10.0f, 10.0f);  // Draw at (10, 10)
   renderer.EndDraw();
}
```

### Error Handling

```cpp
if (!mathBox.Parse(latex)) {
   const SMathBoxError& error = mathBox.LastError();
   
   // Error information
   string message = error.sError;        // Error message
   int startPos = error.nStartPos;       // Error start position
   int endPos = error.nEndPos;           // Error end position
   EnumParsingStage stage = error.eStage; // Parsing stage
   
   // Display error
   wstring wMsg = Utf8ToWide(message);
   MessageBox(nullptr, wMsg.c_str(), L"Parse Error", MB_OK | MB_ICONERROR);
   
   // Optional: Highlight error position in editor
   // ...
}
```

### Parsing Stages

| Stage | Description |
|-------|-------------|
| `epsTOKENIZATION` | Converting input to tokens |
| `epsGROUPINGMACROS` | Grouping macro definition tokens |
| `epsBUILDINGMACROS` | Building macro definitions |
| `epsEXPANDINGMACROS` | Expanding macro invocations |
| `epsGROUPING` | Grouping document tokens |
| `epsBUILDING` | Building MathItem tree |

## Supported LaTeX Commands

See `Macros.mth` and **ReadMeLib.md** for complete list of supported commands.

**Frequently Used:**
- Math mode: `$...$`, `$$...$$`, `\(...\)`, `\[...\]`
- Fractions: `\frac{num}{denom}`, `\dfrac`, `\tfrac`
- Roots: `\sqrt{x}`, `\sqrt[n]{x}`
- Scripts: `x^2`, `x_i`, `x_i^2`
- Delimiters: `\left(...\right)`, `\left[...\right]`, etc.
- Operators: `\sum`, `\int`, `\prod`, `\lim`, `\sin`, `\cos`, etc.
- Accents: `\hat{x}`, `\tilde{x}`, `\vec{v}`, `\dot{x}`, etc.
- Fonts: `\mathbf`, `\mathrm`, `\mathit`, `\mathsf`, `\mathtt`, `\text`
- Matrices: `\begin{matrix}...\end{matrix}`, `pmatrix`, `bmatrix`, etc.

## Troubleshooting

### Common Issues

**1. "Could not load MathBoxLib"**
- Ensure `MathBoxLib.dll` is in the same directory as the executable
- Check that the correct architecture (x64/x86) is used

**2. "Could not read MathBoxDemo.cfg"**
- Verify `MathBoxDemo.cfg` exists in the executable directory
- Check file encoding (should be UTF-8 or ASCII)

**3. "Could not open/read document file"**
- Verify path in `MathBoxDemo.cfg` points to valid file
- Check file exists and is readable
- Ensure UTF-8 encoding for non-ASCII characters

**4. Parse errors**
- Check LaTeX syntax
- Ensure all `\begin{...}` have matching `\end{...}`
- Verify all braces `{...}` are balanced
- Check that commands used are defined in `Macros.mth`

**5. Missing fonts**
- Verify `LatinModernFonts/` directory contains all .otf files
- Check `LatinModernMathGlyphs.csv` is present
- Ensure fonts are not corrupted

## License

See main README.md for license information.

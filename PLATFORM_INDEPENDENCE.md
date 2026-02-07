# MathBoxLib Platform Independence

## Overview

**MathBoxLib** is a **100% platform-independent** C++ library for LaTeX parsing and typesetting.

## Verification

### Core Library (Platform-Independent)

**Location**: `MathBoxLib/MathBox/` and `MathBoxLib/LibCode/`

**Dependencies**: ONLY C++14 standard library (STL)
- `<vector>`, `<string>`, `<map>`, `<algorithm>`, `<cmath>`, etc.
- NO Windows API (`windows.h`, `WIN32`, `DirectWrite`, etc.)
- NO platform-specific code

**Portability Status**: ? Ready for Linux, macOS, BSD, embedded systems, etc.

### Demo Application (Windows-Specific)

**Location**: `MathBoxDemo/Code/`

**Dependencies**: Windows-specific
- `windows.h` - Win32 API
- `d2d1.h` - Direct2D rendering
- `dwrite.h` - DirectWrite font management

**Purpose**: Reference implementation showing how to integrate MathBoxLib on Windows

**Porting**: Implement `IFontManager` and `IDocRenderer` for your platform

### Unit Tests (Windows-Specific)

**Location**: `MathBoxTests/Code/`

**Dependencies**: Windows-specific
- DirectWrite for font management in tests
- MSTest framework

**Porting**: Tests can be rewritten for any platform using platform-specific font APIs

## How to Port to Other Platforms

### Required Implementations

You need to implement two interfaces (from `MathBox_CAPI.h`):

#### 1. Font Manager (`MBI_FontManager`)
```c
typedef struct {
   uint32_t size_bytes;
   uint32_t fontCount;
   
   bool (*getFontsDir)(uint32_t* buf_len, wchar_t* out_dir);
   bool (*getFontIndices)(int32_t font_idx, uint32_t count, 
                         const uint32_t* unicodes, uint16_t* out_indices);
   bool (*getGlyphRunMetrics)(int32_t font_idx, uint32_t count, 
                             const uint16_t* indices,
                             MB_GlyphMetrics* out_glyph_metrics, 
                             MB_Bounds* out_bounds);
} MBI_FontManager;
```

**Platform-Specific APIs:**
- **Linux**: FreeType 2 (`libfreetype`)
- **macOS**: Core Text
- **Cross-platform**: HarfBuzz + FreeType

#### 2. Renderer (`MBI_DocRenderer`)
```c
typedef struct {
   uint32_t size_bytes;
   
   void (*drawLine)(float x1, float y1, float x2, float y2, 
                   uint32_t style, float width, uint32_t argb);
   void (*drawRect)(float l, float t, float r, float b, 
                   uint32_t style, float width, uint32_t argb);
   void (*fillRect)(float l, float t, float r, float b, uint32_t argb);
   void (*drawGlyphRun)(int32_t font_idx, uint32_t count, 
                       const uint16_t* indices,
                       float base_x, float base_y, 
                       float scale, uint32_t argb);
} MBI_DocRenderer;
```

**Platform-Specific APIs:**
- **Linux**: Cairo, Qt Graphics, or Skia
- **macOS**: Core Graphics (Quartz)
- **Cross-platform**: Skia, Qt

### Example Port Targets

#### Linux (Using FreeType + Cairo)
```cpp
// LinuxFontManager.cpp
class CLinuxFontManager {
   FT_Library m_ftLibrary;
   vector<FT_Face> m_fonts;
   
   bool Init(const char* fontDir) {
      FT_Init_FreeType(&m_ftLibrary);
      // Load Latin Modern fonts from fontDir
      // ...
   }
   
   bool GetFontIndices(int32_t font_idx, uint32_t count, 
                      const uint32_t* unicodes, uint16_t* indices) {
      FT_Face face = m_fonts[font_idx];
      for (uint32_t i = 0; i < count; ++i) {
         indices[i] = FT_Get_Char_Index(face, unicodes[i]);
      }
      return true;
   }
   // ... other methods
};

// LinuxRenderer.cpp  
class CLinuxRenderer {
   cairo_t* m_cairo;
   
   void DrawLine(float x1, float y1, float x2, float y2, ...) {
      cairo_move_to(m_cairo, x1, y1);
      cairo_line_to(m_cairo, x2, y2);
      cairo_stroke(m_cairo);
   }
   
   void DrawGlyphRun(int32_t font_idx, uint32_t count, 
                     const uint16_t* indices, ...) {
      // Use cairo_show_glyphs() with FreeType font
   }
   // ... other methods
};
```

#### macOS (Using Core Text + Core Graphics)
```objc
// MacOSFontManager.mm
@implementation MacOSFontManager {
   NSArray<CTFontRef>* _fonts;
}

- (BOOL)getFontIndices:(int32_t)fontIdx 
                count:(uint32_t)count
             unicodes:(const uint32_t*)unicodes
              indices:(uint16_t*)indices {
   CTFontRef font = _fonts[fontIdx];
   for (uint32_t i = 0; i < count; ++i) {
      CGGlyph glyph;
      UniChar character = (UniChar)unicodes[i];
      CTFontGetGlyphsForCharacters(font, &character, &glyph, 1);
      indices[i] = glyph;
   }
   return YES;
}
// ... other methods
@end

// MacOSRenderer.mm
- (void)drawGlyphRun:(int32_t)fontIdx
               count:(uint32_t)count
             indices:(const uint16_t*)indices
               baseX:(float)baseX
               baseY:(float)baseY
               scale:(float)scale
                argb:(uint32_t)argb {
   CTFontRef font = _fonts[fontIdx];
   CGContextShowGlyphsAtPoint(_context, baseX, baseY, 
                              (CGGlyph*)indices, count);
}
```

### Cross-Platform (Using Skia)
```cpp
// SkiaFontManager.cpp
class CSkiaFontManager {
   vector<sk_sp<SkTypeface>> m_fonts;
   
   bool GetFontIndices(...) {
      SkTypeface* typeface = m_fonts[font_idx].get();
      typeface->unicharsToGlyphs(unicodes, count, indices);
      return true;
   }
};

// SkiaRenderer.cpp
class CSkiaRenderer {
   SkCanvas* m_canvas;
   
   void DrawGlyphRun(...) {
      SkFont font(m_fonts[font_idx], scale);
      SkTextBlob* blob = SkTextBlob::MakeFromGlyphs(
         indices, count * sizeof(uint16_t), font, ...);
      m_canvas->drawTextBlob(blob, baseX, baseY, paint);
   }
};
```

## Build Configuration

### Current (Windows Only)
```
MathBoxDemo.sln
??? MathBoxLib.vcxproj   (Pure C++ - portable!)
??? MathBoxDemo.vcxproj  (Windows - DirectWrite)
??? MathBoxTest.vcxproj  (Windows - DirectWrite + MSTest)
```

### Future (Cross-Platform with CMake)
```cmake
# CMakeLists.txt (planned)
cmake_minimum_required(VERSION 3.10)
project(MathBox)

# Platform-independent library
add_library(MathBoxLib SHARED
   MathBoxLib/MathBox/*.cpp
   MathBoxLib/LibCode/*.cpp
)
target_compile_features(MathBoxLib PUBLIC cxx_std_14)

# Platform-specific demo
if(WIN32)
   add_executable(MathBoxDemo WIN32
      MathBoxDemo/Code/*.cpp
   )
   target_link_libraries(MathBoxDemo d2d1 dwrite)
elseif(UNIX AND NOT APPLE)
   add_executable(MathBoxDemo
      MathBoxDemo/Code/LinuxMain.cpp
      MathBoxDemo/Code/LinuxFontManager.cpp
      MathBoxDemo/Code/LinuxRenderer.cpp
   )
   target_link_libraries(MathBoxDemo freetype cairo)
elseif(APPLE)
   add_executable(MathBoxDemo MACOSX_BUNDLE
      MathBoxDemo/Code/MacMain.mm
      MathBoxDemo/Code/MacFontManager.mm
      MathBoxDemo/Code/MacRenderer.mm
   )
   target_link_libraries(MathBoxDemo "-framework CoreText -framework CoreGraphics")
endif()
```

## Contributions Welcome!

We welcome ports to other platforms! If you create a port, please:

1. **Keep MathBoxLib unchanged** - it's already portable
2. **Implement platform-specific host** - font manager and renderer
3. **Share your work** - create a PR or separate repository
4. **Document requirements** - list dependencies and build steps

### Example Contribution Structure
```
MathBox/
??? MathBoxLib/          (unchanged - portable core)
??? MathBoxDemo/         (Windows implementation)
??? MathBoxDemoLinux/    (YOUR Linux implementation)
??? MathBoxDemoMacOS/    (YOUR macOS implementation)
??? MathBoxDemoQt/       (YOUR Qt implementation)
```

## Contact

For porting questions or to share your port:
- GitHub Issues: https://github.com/celicom11/MathBox/issues
- Email: celicom@[your-domain].com

## License

MIT License - See LICENSE.md for details

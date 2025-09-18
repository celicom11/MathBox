# Screenshots Description
## Test1.
Initial test to compare of the Overleaf rendering (top, PdfLaTex compiler, 12pts, Display Mode)) of the script 
```
\usepackage{lmodern} % For Latin Modern text
\usepackage{amsmath} % For math environments and some math features
\usepackage[fontsize=12pt]{fontsize}
\usepackage[margin=0.5cm]{geometry}
\begin{document}

\[
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{9pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{12pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{14pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{18pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{21pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{24pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{30pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{40pt}{20pt}\selectfont 1}}{2}}}
\quad
\sqrt{\sqrt[ABC]{\frac{\text{\fontsize{50pt}{20pt}\selectfont 1}}{2}}}
\quad
\]
\end{document}
```
vs rendering same items in MathBox (bottom).
Notes:
- spacing between radical sign and radicand is a bit smaller in MathBox - done on purpose, I just like it more
- radical's dgree/index position is a bit lower than in Overleaf  - again, done on purpose
- both Overleaf and MathBox hus minor/sub-pixel glitches as the radical sign top /side extentions are drawn manually as lines.
  Hopefully this will be addressed to a perfection in the MathBox ;)
# Screenshots Description
## Test1.
Initial test to compare of the Overleaf rendering (top, PdfLaTex compiler, 12pts, Display Mode)) of the redicals and fractions of different base-item's sizes.
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
## Test2.
Testing various math-fonts (and superscript!) items. 
```
\[\mathnormal{3x^2 \in R \subset Q}\]
\[\it{3x^2 \in R \subset Q}\]
\[\mathit{3x^2 \in R \subset Q}\]
\[\mathrm{3x^2 \in R \subset Q}\]
\[\mathbf{3x^2 \in R \subset Q}\]
\[\mathbfup{3x^2 \in R \subset Q}\]
\[\mathbfit{3x^2 \in R \subset Q}\]
\[\mathscr{3x^2 \in R \subset Q}\]
\[\mathbfscr{3x^2 \in R \subset Q}\]
\[\mathcal{3x^2 \in R \subset Q}\]
\[\mathbfcal{3x^2 \in R \subset Q}\]
\[\mathsf{3x^2 \in R \subset Q}\]
\[\mathsfup{3x^2 \in R \subset Q}\]
\[\mathsfit{3x^2 \in R \subset Q}\]
\[\mathbfsfup{3x^2 \in R \subset Q}\]
\[\mathtt{3x^2 \in R \subset Q}\]
\[\mathbb{3x^2 \in R \subset Q}\]
\[\mathbbit{3x^2 \in R \subset Q}\]
\[\mathfrak{3x^2 \in R \subset Q}\]
\[\mathbffrak{3x^2 \in R \subset Q}\]
```
One noticable difference is that Overleaf/LuaLaTex uses regular font for digits in "\mathbfit" scope, while MathBox - bold digits. Which way is correct??

## Test3.
Testing stacked items (VBox) with 24pt font (bottom) vs LuaLaTeX with amsmath+stackengine packages(top).
```
\stackMath
{_{a}^{b}},{^{a}_{b}},\underset{a}{b},\overset{a}{b},
\substack{b\\a},\substack{a\\b},\substack{\displaystyle b\\\displaystyle a},\substack{\displaystyle a\\\displaystyle b},\stackrel{\scriptscriptstyle >}{<},\stackrel{\scriptscriptstyle ?}{=},
\stackunder[0em]{a}{b},\stackunder[0.1em]{a}{b},\stackunder[0.2em]{a}{b},\stackunder[0.3em]{a}{b},
```
Noticable but not a crucial difference is in line spacing for \substack lines.

## Test4.
Testing extensible accents (Over/Under bracies) vs LuaLaTeX (bottom) with 24pt font.
```
\widehat{ABC},\widecheck{ABC},\widetilde{ABC},\overline{ABC},
\overbracket{ABC},\overparen{ABC},\overbrace{ABC},\\
\overleftarrow{ABCDE},\overrightarrow{ABCDE},\overleftrightarrow{ABCDE},\Overrightarrow{ABCDE},\\
\utilde{ABC},\underline{ABC},\underbracket{ABCDE},\underparen{ABCDE},\underbrace{ABCDE},
```
**Note:** MathBox has additional \obrbrak,\ubrbrak extendable top/bottdelimiterom "tortoise shell brackets" in 1st and the 3rd lines unsupported by known packages.

## Test5.
Testing delimiter's variants vs LuaLaTeX (right) with 24pt font.
```
(\big(\Big(\bigg(\Bigg(\Bigg)\bigg)\Big)\big)),
[\big[\Big[\bigg[\Bigg[\Bigg]\bigg]\Big]\big]]\\
\{\big\{\Big\{\bigg\{\Bigg\{\Bigg\}\bigg\}\Big\}\big\}\},
\backslash\big\backslash\Big\backslash\bigg\backslash\Bigg\backslash\Bigg/\bigg/\Big/\big//\\
\langle\big\langle\Big\langle\bigg\langle\Bigg\langle\Bigg\rangle\bigg\rangle\Big\rangle\big\rangle\rangle,
\lAngle\big\lAngle\Big\lAngle\bigg\lAngle\Bigg\lAngle\Bigg\rAngle\bigg\rAngle\Big\rAngle\big\rAngle\rAngle\\
\uparrow\big\uparrow\Big\uparrow\bigg\uparrow\Bigg\uparrow\Bigg\downarrow\bigg\downarrow\Big\downarrow\big\downarrow\downarrow,
\Uparrow\big\Uparrow\Big\Uparrow\bigg\Uparrow\Bigg\Uparrow\Bigg\Downarrow\bigg\Downarrow\Big\Downarrow\big\Downarrow\Downarrow\\
\updownarrow\big\updownarrow\Big\updownarrow\bigg\updownarrow\Bigg\updownarrow\Bigg\Updownarrow\bigg\Updownarrow\Big\Updownarrow\big\Updownarrow\Updownarrow,\|\big\|\Big\|\bigg\|\Bigg\|\Bigg|\bigg|\Big|\big||\\
\lceil\big\lceil\Big\lceil\bigg\lceil\Bigg\lceil\Bigg\rceil\bigg\rceil\Big\rceil\big\rceil\rceil,
\lfloor\big\lfloor\Big\lfloor\bigg\lfloor\Bigg\lfloor\Bigg\rfloor\bigg\rfloor\Big\rfloor\big\rfloor\rfloor
```
**Note:** MathBox has minor discrepancies on up/down arrows sizes - will be left as is for now.

## Test6.
Ultimate test for the array/matrix environments (w/o gather, align, etc.):
```
\Large \text{Environments} \normalsize 
$$\begin{array}{|l|l|l|l|}\hline
\begin{matrix}a & b \\c & d\end{matrix}&\text{\scriptsize\backslash begin\{matrix\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{matrix\}}&\begin{array}{cc}a & b \\c & d\end{array}&\text{\scriptsize\backslash begin\{array\}\{cc\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{array\}}\\\hline
\begin{pmatrix}a & b \\c & d\end{pmatrix}&\text{\scriptsize\backslash begin\{pmatrix\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{pmatrix\}}&\begin{bmatrix}a & b \\c & d\end{bmatrix}&\text{\scriptsize\backslash begin\{bmatrix\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{bmatrix\}}\\\hline
\begin{vmatrix}a & b \\c & d\end{vmatrix}&\text{\scriptsize\backslash begin\{vmatrix\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{vmatrix\}}&\begin{Vmatrix}a & b \\c & d\end{Vmatrix}&\text{\scriptsize\backslash begin\{Vmatrix\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{Vmatrix\}}\\\hline
\begin{Bmatrix}a & b \\c & d\end{Bmatrix}&\text{\scriptsize\backslash begin\{Bmatrix\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{Bmatrix\}}&\begin{array}{c:c:c} a & b & c \\\hline
 d & e & f\end{array}&\text{\scriptsize\backslash begin\{array\}\{c:c:c\}\\    a \& b \& c\backslash\backslash  \backslash hline\\    d \& e \& f\\\backslash end\{array\}}\\\hline
x=\begin{cases}a &\text{if } b \\c &\text{if } d\end{cases}&\text{\scriptsize x=\backslash begin\{cases\}\\a \&\backslash text\{if \} b \backslash \backslash\\c \&\backslash text\{if \} d\\\backslash end\{cases\}}&\begin{rcases}a &\text{if } b \\c &\text{if } d\end{rcases}\Rightarrow\ldots&\text{\scriptsize\backslash begin\{rcases\}\\a \&\backslash text\{if \} b \backslash \backslash\\c \&\backslash text\{if \} d\\\backslash end\{rcases\}\backslash Rightarrow...}\\\hline
\begin{smallmatrix}a & b \\c & d\end{smallmatrix}&\text{\scriptsize\backslash begin\{smallmatrix\}\\    a \& b \backslash\backslash\\    c \& d\\\backslash end\{smallmatrix\}}&\sum_{\begin{subarray}{l}i\in\Lambda\\0<j<n\end{subarray}}&\text{\scriptsize\backslash sum\_\{\\\backslash begin\{subarray\}\{l\}\\i\backslash in\backslash Lambda\backslash\backslash\\0<j<n\backslash end\{subarray\}\}}\\
\hline
\end{array}$$
```
Noticable differences: MathBox handles newlines(or \\) inside \text{} while LuaLaTeX and KaTex do not!
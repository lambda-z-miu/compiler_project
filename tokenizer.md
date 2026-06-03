我准备构建一个自定义的语言，可以从表达式渲染分子结构式，使用C/C++，生成SVG。首先先写一个tokenizer，包含以下几类token：
1) SEP := ,
2) KEYWORDS := ph | Me | Ac | = | # | =O | C | P | N | digit R | digit L | F | Cl | Br | I
3) SEMI := ;
4) identifier := [A-Z][A-Z]*
5) LPAREN = (
6) RPAREN = )
7) LSTR = [
8) RSTR = ]
9) digit = [0-9][0-9]*
10) HAF = -
忽略换行符、空格

对于TOKEN流，给出下列CFG，帮我解析成语法树
TOKEN TYPE：
1) SEP := ,
2) KEYWORDS := ph | Me | Ac | = | # | =O | NH2 | OH | SH | Im | Ind | Pyr | C | P | O | N | S | digit R | digit L | F | Cl | Br | I
3) SEMI := ;
4) identifier := [A-Z][A-Z]*
5) LPAREN = (
6) RPAREN = )
7) LSTR = [
8) RSTR = ]
9) LBRACE = {
10) RBRACE = }
11) digit = [0-9][0-9]*
12) HAF = -
13) CARET = ^

CFG规则：
1) CPD := CPO semi CPD
        | empty

2) CPO := INTERFACEL CORE SUBS INTERFACER
3) INTERFACEL := INTERFACE
   INTERFACER := INTERFACE
   INTERFACE := lparen POSES rparen
             | lbrace POSES rbrace
4) POSES := digit 
        | digit sep POSES
5) SUBS := POSES haf SUBS'
        | caret SUBS_ITEM
        | SUB
        | empty
6) SUBS' := SUBS_ITEM
        | caret SUBS_ITEM
7) SUBS_ITEM := SUB sep SUBS
        | lstr CPO rstr sep SUBS
8) SUB := identifier
        | CORE
9) CORE := KEYWORDS 
        | KEYWORDS CORE

caret before SUBS_ITEM means replacement: p-^X replaces atom p with X instead of attaching X to p. A leading ^X without explicit POSES uses pose 1.
Numbered atom patch syntax such as 5O, 4N, or 3S is invalid and is not part of CORE or SUBS; write 5-^O, 4-^N, or 3-^S instead.
Round interfaces connect corresponding atoms directly, for example (1,2);(1,2). Brace interfaces fuse by sharing atoms, for example {1,2};{1,2}. Adjacent interfaces must use the same bracket kind.
先判断这个文法是LL形还是LR形，告诉我解析策略，再开始写

CFG(消除左递归、左公因子)
1) CPD := CPO semi CPD
        | empty

2) CPO := INTERFACEL CORE sep SUBS INTERFACER
3) INTERFACEL := INTERFACE
   INTERFACER := INTERFACE
   INTERFACE := lparen POSES rparen
             | lbrace POSES rbrace
4) POSES := digit POSES'
5) POSES':=  empty
        | sep POSES'
5) SUBS := SUB
        | POSES haf SUBS'
        | caret SUBS_ITEM
        | empty
6) SUBS':= SUBS_ITEM
        | caret SUBS_ITEM
7) SUBS_ITEM := SUB sep SUBS
        | lstr CPO rstr sep SUBS
8) SUB := identifier
        | CORE
9) CORE := keywords CORET
10) CORET:= empty
        | keywords CORET

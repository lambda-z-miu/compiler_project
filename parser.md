对于TOKEN流，给出下列CFG，帮我解析成语法树
TOKEN TYPE：
1) SEP := ,
2) KEYWORDS := ph | Me | Ac | = | # | =O | NH2 | OH | SH | Im | Ind | Pyr | C | P | N | S | digit R | digit L | F | Cl | Br | I
3) SEMI := ;
4) identifier := [A-Z][A-Z]*
5) LPAREN = (
6) RPAREN = )
7) LSTR = [
8) RSTR = ]
9) digit = [0-9][0-9]*
10) HAF = -

CFG规则：
1) CPD := CPO semi CPD
        | empty

2) CPO := INTERFACEL CORE SUBS INTERFACER
3) INTERFACEL := lparen POSES rparen
   INTERFACER := lparen POSES rparen
4) POSES := digit 
        | digit sep POSES
5) SUBS := POSES SUB sep SUBS
        | POSES lstr CPO rstr  SUBS
        | empty
6) SUB := identifier
        | CORE
7) CORE := KEYWORDS 
        | KEYWORDS CORE
先判断这个文法是LL形还是LR形，告诉我解析策略，再开始写

CFG(消除左递归、左公因子)
1) CPD := CPO semi CPD
        | empty

2) CPO := INTERFACEL CORE sep SUBS INTERFACER
3) INTERFACEL := lparen POSES rparen
   INTERFACER := lparen POSES rparen
4) POSES := digit POSES'
5) POSES':=  empty
        | sep POSES'
5) SUBS := SUB
        | POSES haf SUBS'
        | empty
6) SUBS':= SUB sep SUBS
        |  lstr CPO rstr sep SUBS
7) SUB := identifier
        | CORE
8) CORE := keywords CORET
9) CORET:= empty
        | keywords CORET

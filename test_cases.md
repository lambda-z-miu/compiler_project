## test Cases:

```txt
Ph,1,2,3-OMe(4,5);7R,4-[A](6,7);7R,2,5,7-=,4,=O,4-OMe;[A;NC=OC]
Ph,1,2,3-OMe(4,5);7R,4-[NC=OC](6,7);7R,2,5,7-=,4,=O,4-OMe;

(0)Ph,1,2,3-Me,(4,5);(1,2)7R,4-[(1)NC=OC,(1)](6,7);(1,2)7R,2,5,7-=,4-=O,4-OMe,(0,0);

(1)Ph(1);(1)Ph(1);
(0)Ph,1-Me,(3);(1)Ph,(0);



for 
(0)Ph,1,2,3-OMe,(4,5);(1,2)7R,4-[(1)NC=OC,(1)](6,7);(1,2)7R,2,5,7-=,4-=O,4-OMe,(0,0);
Tokens:
LParen  (       0
Digit   0       1
RParen  )       2
Keyword Ph      3
Sep     ,       5
Digit   1       6
Sep     ,       7
Digit   2       8
Sep     ,       9
Digit   3       10
Haf     -       11
Keyword OMe     12
Sep     ,       15
LParen  (       16
Digit   4       17
Sep     ,       18
Digit   5       19
RParen  )       20
Semi    ;       21
LParen  (       22
Digit   1       23
Sep     ,       24
Digit   2       25
RParen  )       26
Keyword 7R      27
Sep     ,       29
Digit   4       30
Haf     -       31
LStr    [       32
LParen  (       33
Digit   1       34
RParen  )       35
Keyword N       36
Keyword C       37
Keyword =O      38
Keyword C       40
Sep     ,       41
LParen  (       42
Digit   1       43
RParen  )       44
RStr    ]       45
LParen  (       46
Digit   6       47
Sep     ,       48
Digit   7       49
RParen  )       50
Semi    ;       51
LParen  (       52
Digit   1       53
Sep     ,       54
Digit   2       55
RParen  )       56
Keyword 7R      57
Sep     ,       59
Digit   2       60
Sep     ,       61
Digit   5       62
Sep     ,       63
Digit   7       64
Haf     -       65
Keyword =       66
Sep     ,       67
Digit   4       68
Haf     -       69
Keyword =O      70
Sep     ,       72
Digit   4       73
Haf     -       74
Keyword OMe     75
Sep     ,       78
LParen  (       79
Digit   0       80
Sep     ,       81
Digit   0       82
RParen  )       83
Semi    ;       84
End             85
AST:
CPO #1:
  Interface L:
    Poses: 0
  Core: Ph
  Subs: entry
    Poses: 1, 2, 3
    Sub Core:
      Core: OMe
  Interface R:
    Poses: 4, 5
CPO #2:
  Interface L:
    Poses: 1, 2
  Core: 7R
  Subs: entry
    Poses: 4
    Group:
      Interface L:
        Poses: 1
      Core: N C =O C
      Subs: empty
      Interface R:
        Poses: 1
  Interface R:
    Poses: 6, 7
CPO #3:
  Interface L:
    Poses: 1, 2
  Core: 7R
  Subs: entry
    Poses: 2, 5, 7
    Sub Core:
      Core: =
    Next:
      Subs: entry
        Poses: 4
        Sub Core:
          Core: =O
        Next:
          Subs: entry
            Poses: 4
            Sub Core:
              Core: OMe
  Interface R:
    Poses: 0, 0

```
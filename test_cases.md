## test Cases:
Replacement / hetero atom examples:

```txt
(0)6R,1-^O,(0);
(0)6R,1-^OH,(0);
(0)6R,1-^[(1)COH,(0)],(0);
```

```txt
(0)Ph,1-OH,(0);

Ph,1,2,3-OMe(4,5);7R,4-[A](6,7);7R,2,5,7-=,4,=O,5-OMe;[A;NC=OC]
Ph,1,2,3-OMe(4,5);7R,4-[NC=OC](6,7);7R,2,5,7-=,4,=O,5-OMe;

(0)Ph,1,2,3-OMe,(4,5);(1,2)7R,4-[(1)NC=OC,(1)](6,7);(1,2)7R,2,5,7-=,4-=O,5-OMe,(0,0);

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
Keyword Me      12
Sep     ,       14
LParen  (       15
Digit   4       16
Sep     ,       17
Digit   5       18
RParen  )       19
Semi    ;       20
LParen  (       21
Digit   1       22
Sep     ,       23
Digit   2       24
RParen  )       25
Keyword 7R      26
Sep     ,       28
Digit   4       29
Haf     -       30
LStr    [       31
LParen  (       32
Digit   1       33
RParen  )       34
Keyword N       35
Keyword C       36
Keyword =O      37
Keyword C       39
Sep     ,       40
LParen  (       41
Digit   1       42
RParen  )       43
RStr    ]       44
LParen  (       45
Digit   6       46
Sep     ,       47
Digit   7       48
RParen  )       49
Semi    ;       50
LParen  (       51
Digit   1       52
Sep     ,       53
Digit   2       54
RParen  )       55
Keyword 7R      56
Sep     ,       58
Digit   2       59
Sep     ,       60
Digit   5       61
Sep     ,       62
Digit   7       63
Haf     -       64
Keyword =       65
Sep     ,       66
Digit   4       67
Haf     -       68
Keyword =O      69
Sep     ,       71
Digit   5       72
Haf     -       73
Keyword OMe     74
Sep     ,       77
LParen  (       78
Digit   0       79
Sep     ,       80
Digit   0       81
RParen  )       82
Semi    ;       83
End             84
AST:
CPO #1:
  Interface L:
    Poses: 0
  Core: Ph
  Subs: entry
    Poses: 1, 2, 3
    Sub Core:
      Core: Me
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
            Poses: 5
            Sub Core:
              Core: OMe
  Interface R:
    Poses: 0, 0
    
IR:
C(1,2,2,6,7,0,0)
C(2,3,1,1,8,0,0)
C(3,4,4,2,9,0,0)
C(4,5,3,3,16,0,0)
C(5,6,6,4,12,0,0)
C(6,1,5,0,0,0,0)
C(7,1,0,0,0,0,0)
C(8,2,0,0,0,0,0)
C(9,3,0,0,0,0,0)
C(12,5,13,0,0,0,0)
C(13,12,14,17,0,0,0)
C(14,13,15,0,0,0,0)
C(15,14,16,27,27,0,0)
C(16,4,15,23,23,0,0)
N(17,13,18,0,0,0,0)
C(18,17,19,19,20,0,0)
O(19,18,18,0,0,0,0)
C(20,18,0,0,0,0,0)
C(23,16,16,24,0,0,0)
C(24,23,25,28,28,0,0)
C(25,24,26,26,29,0,0)
C(26,25,25,27,0,0,0)
C(27,15,15,26,0,0,0)
O(28,24,24,0,0,0,0)
O(29,25,30,0,0,0,0)
C(30,29,0,0,0,0,0)

## test Cases:
Explicit inter-CPO connection examples:

```txt
(0)2L,(1,2);(1,2)2L,(0);
(0)6R,{4,5};{1,2}6R,(0);
```
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

(0)Ph,1,2,3-OMe,{4,5};{1,2}7R,4-[(1)NC=OC,(1)]{6,7};{1,2}7R,2,5,7-=,4-=O,5-OMe,(0,0);

(1)Ph(1);(1)Ph(1);
(0)Ph,1-Me,(3);(1)Ph,(0);



for 
(0)Ph,1,2,3-OMe,{4,5};{1,2}7R,4-[(1)NC=OC,(1)]{6,7};{1,2}7R,2,5,7-=,4-=O,4-OMe,(0,0);
Tokens:
LParen	(	0
Digit	0	1
RParen	)	2
Keyword	Ph	3
Sep	,	5
Digit	1	6
Sep	,	7
Digit	2	8
Sep	,	9
Digit	3	10
Haf	-	11
Keyword	OMe	12
Sep	,	15
LBrace	{	16
Digit	4	17
Sep	,	18
Digit	5	19
RBrace	}	20
Semi	;	21
LBrace	{	22
Digit	1	23
Sep	,	24
Digit	2	25
RBrace	}	26
Keyword	7R	27
Sep	,	29
Digit	4	30
Haf	-	31
LStr	[	32
LParen	(	33
Digit	1	34
RParen	)	35
Keyword	N	36
Keyword	C	37
Keyword	=O	38
Keyword	C	40
Sep	,	41
LParen	(	42
Digit	1	43
RParen	)	44
RStr	]	45
LBrace	{	46
Digit	6	47
Sep	,	48
Digit	7	49
RBrace	}	50
Semi	;	51
LBrace	{	52
Digit	1	53
Sep	,	54
Digit	2	55
RBrace	}	56
Keyword	7R	57
Sep	,	59
Digit	2	60
Sep	,	61
Digit	5	62
Sep	,	63
Digit	7	64
Haf	-	65
Keyword	=	66
Sep	,	67
Digit	4	68
Haf	-	69
Keyword	=O	70
Sep	,	72
Digit	4	73
Haf	-	74
Keyword	OMe	75
Sep	,	78
LParen	(	79
Digit	0	80
Sep	,	81
Digit	0	82
RParen	)	83
Semi	;	84
End		87
AST:
CPO #1:
  Interface L (connect):
    Poses: 0
  Core: Ph
  Subs: entry attach
    Poses: 1, 2, 3
    Sub Core:
      Core: OMe
  Interface R (fuse):
    Poses: 4, 5
CPO #2:
  Interface L (fuse):
    Poses: 1, 2
  Core: 7R
  Subs: entry attach
    Poses: 4
    Group:
      Interface L (connect):
        Poses: 1
      Core: N C =O C
      Subs: empty
      Interface R (connect):
        Poses: 1
  Interface R (fuse):
    Poses: 6, 7
CPO #3:
  Interface L (fuse):
    Poses: 1, 2
  Core: 7R
  Subs: entry attach
    Poses: 2, 5, 7
    Sub Core:
      Core: =
    Next:
      Subs: entry attach
        Poses: 4
        Sub Core:
          Core: =O
        Next:
          Subs: entry attach
            Poses: 4
            Sub Core:
              Core: OMe
  Interface R (connect):
    Poses: 0, 0
IR:
C(1,2,2,6,7,0,0)
C(2,3,1,1,9,0,0)
C(3,4,4,2,11,0,0)
C(4,5,3,3,19,0,0)
C(5,6,6,4,15,0,0)
C(6,1,5,0,0,0,0)
O(7,8,1,0,0,0,0)
C(8,7,0,0,0,0,0)
O(9,10,2,0,0,0,0)
C(10,9,0,0,0,0,0)
O(11,12,3,0,0,0,0)
C(12,11,0,0,0,0,0)
C(15,5,16,0,0,0,0)
C(16,15,17,20,0,0,0)
C(17,16,18,0,0,0,0)
C(18,17,19,30,30,0,0)
C(19,4,18,26,26,0,0)
N(20,16,21,0,0,0,0)
C(21,20,22,22,23,0,0)
O(22,21,21,0,0,0,0)
C(23,21,0,0,0,0,0)
C(26,19,19,27,0,0,0)
C(27,26,28,31,31,32,0)
C(28,27,29,29,0,0,0)
C(29,28,28,30,0,0,0)
C(30,18,18,29,0,0,0)
O(31,27,27,0,0,0,0)
O(32,27,33,0,0,0,0)
C(33,32,0,0,0,0,0)

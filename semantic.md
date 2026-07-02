# AST 到 IR 的属性文法 / SDT

这份文档描述当前 AST 如何翻译到中间表示 IR。因为 `parser.cpp` 已经把输入解析成 AST，所以实现时可以按 AST visitor 写；这里仍按文法给出属性和 SDT，方便直接对应代码。

## 1. IR 约定

IR 是一个无序图，用节点表表示：

```txt
GRAPH := NODE*
NODE  := ATOM(self_id, conn1, conn2, conn3, conn4, conn5, conn6)
ATOM  := H | C | P | O | N | S | F | Cl | Br | I
```

约定：

1. `self_id` 是正整数，且在一个 `GRAPH` 内唯一。
2. `conn_i` 是非负整数；`0` 表示空连接槽。
3. 键阶用重复邻接点表示：
   - 单键：`a` 的连接表中出现一次 `b`，`b` 的连接表中出现一次 `a`。
   - 双键：各出现两次。
   - 三键：各出现三次。
4. 一个节点最多有 6 个连接槽；插入连接时若没有足够的 `0` 槽，报 valence overflow。
5. `0` 不参与 ID 平移。

实现上建议使用：

```txt
Node {
    atom: string
    id: int
    conn[6]: int
}

Fragment {
    IR: list<Node>
    entry: int              // 片段接入点；0 表示无接入点
    exit: int               // 线性 CORE 的尾点；0 表示无尾点
    entryBond: int          // 外部连接到 entry 时使用的键阶，默认 1
    interfaceL: list<int>   // CPO 左接口
    interfaceR: list<int>   // CPO 右接口
    defaultNext: map<int,int> // pose -= 这类内部升键的默认目标
}

SubEntry {
    poses: list<int>
    frag: Fragment
    replace: bool          // false: attach; true: ^ replacement
}

Keyword {
    kind: atom | bond
    bond: int
    frag: Fragment
}
```

## 2. 基础辅助函数

下面的辅助函数是 SDT 里会反复使用的语义动作。

```txt
maxId(G):
    if G is empty: return 0
    return max(node.id for node in G)

shift(F, k):
    F 中所有非 0 的 node.id、conn、entry、exit、interfaceL、
    interfaceR、defaultNext 的 key/value 都加 k
    return F

merge(G1, G2):
    要求 G1 和 G2 的 self_id 不冲突
    return G1 ∪ G2

insertConn(G, a, b, times):
    在 G[a] 的前 times 个 0 槽写入 b
    若 0 槽不足则报错

link(G, a, b, order):
    if a == 0 or b == 0: error
    insertConn(G, a, b, order)
    insertConn(G, b, a, order)

bondOrder(G, a, b):
    return count(b in G[a].conn)

ensureBond(G, a, b, order):
    // 把 a-b 的键阶提升到 order；已有更高键阶则不降低
    old = bondOrder(G, a, b)
    if old == 0:
        link(G, a, b, order)
    else if old < order:
        insertConn(G, a, b, order - old)
        insertConn(G, b, a, order - old)

freeValence(G, a):
    return number of 0 slots in G[a].conn
```

### 稠合 fuse

稠合的含义是“两个接口原子被识别为同一对原子”，所以总节点数减少 2。

```txt
fuse(A, rightPorts, B, leftPorts):
    require len(rightPorts) == 2
    require len(leftPorts) == 2
    require all ports are non-zero

    B = shift(B, maxId(A.IR))
    alias[B.interfaceL[0]] = A.interfaceR[0]
    alias[B.interfaceL[1]] = A.interfaceR[1]

    for each node n in B.IR:
        n.id = alias.get(n.id, n.id)
        for each conn c in n.conn:
            if c != 0:
                c = alias.get(c, c)

    for each rewritten node n in B.IR:
        if n.id already exists in A.IR:
            require n.atom == A.IR[n.id].atom
            merge connections by edge multiplicity:
                对每条边 (n.id, x)，令最终键阶为 max(A 中已有键阶, B 中该键阶)
        else:
            append n to A.IR

    A.interfaceR = rewrite(B.interfaceR, alias)
    A.defaultNext += rewrite(B.defaultNext, alias)
    return A
```

### CPD 片段拼接

```txt
connectFragments(A, B):
    if B.IR is empty:
        return A
    if A.IR is empty:
        return B

    B = shift(B, maxId(A.IR))
    L = B.interfaceL
    R = A.interfaceR

    if allZero(R):
        error "left fragment has no right interface"
    if allZero(L):
        error "right fragment has no left interface"

    if len(R) == 1 and len(L) == 1:
        link(A.IR ∪ B.IR, R[0], L[0], B.entryBond)
        C.IR = merge(A.IR, B.IR)
    else if len(R) == 2 and len(L) == 2:
        C = fuse(A, R, B, L)
    else:
        error "interface size mismatch"

    C.interfaceL = A.interfaceL
    C.interfaceR = B.interfaceR
    C.entry = firstNonZero(C.interfaceL)
    C.exit = firstNonZero(C.interfaceR)
    C.entryBond = 1
    C.defaultNext = A.defaultNext ∪ B.defaultNext
    return C
```

`allZero([0])`、`allZero([0,0])` 都表示没有外部接口。顶层第一个 CPO 的 `interfaceL` 允许为 0，最后一个 CPO 的 `interfaceR` 允许为 0；只有需要连接下一个片段时，接口才必须非 0。

### 取代基连接 attach

```txt
attach(F, pose, S):
    require pose != 0
    require pose exists in F.IR

    // S 是纯键，例如 2-= 表示把 pose 的默认内部键提升为双键
    if S.IR is empty:
        require S.entryBond > 1
        target = F.defaultNext[pose]
        require target exists
        ensureBond(F.IR, pose, target, S.entryBond)
        return F

    S = shift(S, maxId(F.IR))

    // 普通 SUB 用 entry；[CPO] 作为 SUB 时优先用 interfaceL
    ports = nonZero(S.interfaceL)
    if ports is empty:
        ports = [S.entry]
    require len(ports) == 1

    F.IR = merge(F.IR, S.IR)
    link(F.IR, pose, ports[0], S.entryBond)
    F.defaultNext += S.defaultNext
    return F
```

多位置取代基 `p1,p2,...-SUB` 的语义是对每个 `pose` 复制一份 `SUB` 再连接：

```txt
attachMany(F, poses, S):
    for p in poses:
        F = attach(F, p, clone(S))
    return F
```

## 3. 文法属性

下面使用这些属性：

```txt
CPD.frag
CPO.frag
CORE.frag
SUB.frag
SUBS.entries
SUBS'.inhPoses
SUBS'.entries
POSES.list
KEYWORDS.kw
CORET.list
```

## 4. SDT 规则

### 4.1 CPD

```txt
CPD -> CPO semi CPD1
    {
        CPD.frag = connectFragments(CPO.frag, CPD1.frag)
    }

CPD -> empty
    {
        CPD.frag = empty Fragment
    }
```

实现时如果 AST 已经是 `vector<Cpo> items`，可以直接从左到右折叠：

```txt
result = empty Fragment
for cpo in items:
    f = translateCpo(cpo)
    if result.IR is empty:
        result = f
    else:
        result = connectFragments(result, f)
return result
```

### 4.2 CPO

```txt
CPO -> INTERFACEL CORE sep SUBS INTERFACER
    {
        F = CORE.frag
        F.interfaceL = INTERFACEL.list
        F.interfaceR = INTERFACER.list

        for each entry in SUBS.entries:
            F = attachMany(F, entry.poses, entry.frag)

        F.entry = firstNonZero(F.interfaceL)
        F.exit = firstNonZero(F.interfaceR)
        CPO.frag = F
    }
```

要求：

1. `INTERFACEL.list` 和 `INTERFACER.list` 中的非 0 位置必须存在于 `CORE.frag.IR`，除非该接口整体为 0。
2. `SUBS` 中所有 `pose` 必须存在于当前 CPO 的 IR。
3. 如果 `SUBS` 先连接了较大的取代基，后续 pose 仍然按原 CORE 的编号解释；也就是说取代位置不受新插入节点编号影响。

### 4.3 INTERFACE / POSES

```txt
INTERFACEL -> lparen POSES rparen
    { INTERFACEL.list = POSES.list }

INTERFACER -> lparen POSES rparen
    { INTERFACER.list = POSES.list }

POSES -> digit POSES'
    { POSES.list = [digit.value] + POSES'.list }

POSES' -> sep digit POSES'1
    { POSES'.list = [digit.value] + POSES'1.list }

POSES' -> empty
    { POSES'.list = [] }
```

### 4.4 SUBS

```txt
SUBS -> SUB
    {
        // 裸 SUB 默认接到 1 号位置
        SUBS.entries = [{ poses: [1], frag: SUB.frag }]
    }

SUBS -> POSES haf SUBS'
    {
        SUBS'.inhPoses = POSES.list
        SUBS.entries = SUBS'.entries
    }

SUBS -> empty
    {
        SUBS.entries = []
    }
```

```txt
SUBS' -> SUB sep SUBS
    {
        SUBS'.entries =
            [{ poses: SUBS'.inhPoses, frag: SUB.frag }] + SUBS.entries
    }

SUBS' -> lstr CPO rstr sep SUBS
    {
        G = CPO.frag
        // [CPO] 作为取代基时，外部连接点是它的左接口。
        // 若左接口有多个非 0 位置，当前 attach 规则暂不支持，需报错或扩展为多点桥接。
        G.entry = firstNonZero(G.interfaceL)
        G.entryBond = 1
        SUBS'.entries =
            [{ poses: SUBS'.inhPoses, frag: G }] + SUBS.entries
    }
```

### 4.5 SUB

```txt
SUB -> identifier
    {
        SUB.frag = clone(symbolTable[identifier])
        if identifier not found: error
    }

SUB -> CORE
    {
        SUB.frag = CORE.frag
    }
```

如果当前阶段还没有宏/变量定义，可以先让 `identifier` 查一个预置表；查不到就报错。后面支持用户自定义片段时，再把 CPD 或 CPO 的翻译结果登记进符号表。

### 4.6 CORE / CORET

```txt
CORE -> KEYWORDS CORET
    {
        CORE.frag = buildCore([KEYWORDS.kw] + CORET.list)
    }

CORET -> KEYWORDS CORET1
    {
        CORET.list = [KEYWORDS.kw] + CORET1.list
    }

CORET -> empty
    {
        CORET.list = []
    }
```

`buildCore`：

```txt
buildCore(list):
    F = empty Fragment
    pendingBond = 1
    leadingBond = 1

    for kw in list:
        if kw.kind == bond:
            if F.IR is empty:
                leadingBond = kw.bond
            else:
                pendingBond = kw.bond
            continue

        T = clone(kw.frag)
        T = shift(T, maxId(F.IR))

        if F.IR is empty:
            F = T
            F.entryBond = max(leadingBond, T.entryBond)
        else:
            order = max(pendingBond, T.entryBond)
            F.IR = merge(F.IR, T.IR)
            link(F.IR, F.exit, T.entry, order)
            F.exit = T.exit
            F.defaultNext += T.defaultNext

        pendingBond = 1

    if F.IR is empty:
        // 例如 CORE 只有 "=" 或 "#"
        F.entry = 0
        F.exit = 0
        F.entryBond = leadingBond

    return F
```

解释：

1. `=`、`#` 是纯键修饰符，不生成节点。
2. `=O` 不是纯键，它生成一个 O 节点，但 `entryBond = 2`，所以外部接入或前一个 CORE 接入它时会形成双键。
3. 如果 CORE 只有 `=`，它会得到一个空 IR 但 `entryBond = 2` 的片段；`attach` 会把目标 pose 的默认内部键升成双键。

## 5. KEYWORDS 的语义

默认属性：

```txt
kw.kind = atom
kw.bond = 1
kw.frag.entry = 1
kw.frag.exit = maxId(kw.frag.IR)
kw.frag.entryBond = 1
kw.frag.interfaceL = []
kw.frag.interfaceR = []
```

### Ph

```txt
KEYWORDS -> Ph
    {
        kw.frag.IR = [
            C(1,2,6,0,0,0,0),
            C(2,3,1,0,0,0,0),
            C(3,4,2,0,0,0,0),
            C(4,5,3,0,0,0,0),
            C(5,6,4,0,0,0,0),
            C(6,1,5,0,0,0,0)
        ]
        kw.frag.entry = 1
        kw.frag.exit = 1
        kw.frag.defaultNext = {
            1:2, 2:3, 3:4, 4:5, 5:6, 6:1
        }
    }
```

### Me

```txt
KEYWORDS -> Me
    {
        kw.frag.IR = [ C(1,0,0,0,0,0,0) ]
        kw.frag.entry = 1
        kw.frag.exit = 1
    }
```

### Ac

这里把 `Ac` 解释为 `C(=O)C`，入口和出口都在羰基碳上。

```txt
KEYWORDS -> Ac
    {
        kw.frag.IR = [
            C(1,2,3,3,0,0,0),
            C(2,1,0,0,0,0,0),
            O(3,1,1,0,0,0,0)
        ]
        kw.frag.entry = 1
        kw.frag.exit = 1
    }
```

### 键修饰符

```txt
KEYWORDS -> =
    {
        kw.kind = bond
        kw.bond = 2
        kw.frag = empty Fragment with entryBond = 2
    }

KEYWORDS -> #
    {
        kw.kind = bond
        kw.bond = 3
        kw.frag = empty Fragment with entryBond = 3
    }
```

### =O

```txt
KEYWORDS -> =O
    {
        kw.frag.IR = [ O(1,0,0,0,0,0,0) ]
        kw.frag.entry = 1
        kw.frag.exit = None
        kw.frag.entryBond = 2
    }
```

### NH2

    KEYWORDS -> NH2
        {
            kw.frag.IR = [
                N(1,2,3,0,0,0,0),
                H(2,1,0,0,0,0,0),
                H(3,1,0,0,0,0,0)
            ]
            kw.frag.entry = 1
            kw.frag.exit = 1
            kw.frag.entryBond = 1
        }

### OH

```txt
KEYWORDS -> OH
    {
        kw.frag.IR = [
            O(1,2,0,0,0,0,0),
            H(2,1,0,0,0,0,0)
        ]
        kw.frag.entry = 1
        kw.frag.exit = None
        kw.frag.entryBond = 1
    }
```

### 单原子

```txt
KEYWORDS -> C
    { kw.frag.IR = [ C(1,0,0,0,0,0,0) ] }

KEYWORDS -> P
    { kw.frag.IR = [ P(1,0,0,0,0,0,0) ] }

KEYWORDS -> N
    { kw.frag.IR = [ N(1,0,0,0,0,0,0) ] }

KEYWORDS -> F
    { kw.frag.IR = [ F(1,0,0,0,0,0,0) ] }

KEYWORDS -> Cl
    { kw.frag.IR = [ Cl(1,0,0,0,0,0,0) ] }

KEYWORDS -> Br
    { kw.frag.IR = [ Br(1,0,0,0,0,0,0) ] }

KEYWORDS -> I
    { kw.frag.IR = [ I(1,0,0,0,0,0,0) ] }
```

这些单原子的默认 `entry = exit = 1`，`entryBond = 1`。

### digit R

`nR` 表示 n 元环，入口和出口都默认为 1 号原子。

```txt
KEYWORDS -> digit R
    {
        n = digit.value
        require n >= 3

        for i in 1..n:
            next = (i == n ? 1 : i + 1)
            prev = (i == 1 ? n : i - 1)
            append C(i,next,prev,0,0,0,0)

        kw.frag.entry = 1
        kw.frag.exit = 1
        kw.frag.defaultNext = {
            i : (i == n ? 1 : i + 1) for i in 1..n
        }
    }
```

### digit L

`nL` 表示 n 个碳组成的线性链。

```txt
KEYWORDS -> digit L
    {
        n = digit.value
        require n >= 1

        if n == 1:
            append C(1,0,0,0,0,0,0)
        else:
            append C(1,2,0,0,0,0,0)
            for i in 2..n-1:
                append C(i,i-1,i+1,0,0,0,0)
            append C(n,n-1,0,0,0,0,0)

        kw.frag.entry = 1
        kw.frag.exit = n
        kw.frag.defaultNext = {
            i : i + 1 for i in 1..n-1
        }
    }
```

## 6. 关键样例的翻译含义

### 样例 1

```txt
(0)Ph,1-Me,(3);
```

含义：

1. `Ph` 生成 6 个 C 的环。
2. `1-Me` 在 1 号 C 上接一个甲基。
3. 左接口为 0，不连接前序片段。
4. 右接口为 3，若后面还有 CPO，则从 3 号 C 继续连接。

### 样例 2

```txt
(1,2)7R,2,5,7-=,4-=O,4-OMe,(0,0);
```

含义：

1. `7R` 生成 7 元环。
2. `2,5,7-=` 把 `2-defaultNext[2]`、`5-defaultNext[5]`、`7-defaultNext[7]` 的键提升为双键。
3. `4-=O` 在 4 号 C 上双键连接 O。
4. `4-OMe` 在 4 号 C 上单键连接 `OMe` 翻译出的片段。
5. 右接口 `(0,0)` 表示后面不再稠合。

## 7. 实现顺序建议

1. 先实现 `Node`、`Fragment`、`link`、`shift`、`merge`、`attach`。
2. 实现 `translateKeyword` 和 `buildCore`，用简单输入测试 `Ph`、`Me`、`=O`、`7R`。
3. 实现 `translateSub/translateSubs/translateCpo`，测试单个 CPO。
4. 实现 `connectFragments` 的 bridge。
5. 最后实现 `fuse`，测试 `(4,5);(1,2)` 这种双接口稠合。

## Replacement Substitution: ^X

Syntax extension:

    SUBS_PRIME -> caret SUB sep SUBS
    SUBS_PRIME -> caret lstr CPO rstr sep SUBS

Concrete expressions:

    p-^X,
    p1,p2-^X,
    p-^[CPO],

Semantics:

    replace(F, pose, S):
        require pose exists in F.IR
        require S.IR is not empty
        S = shift(S, maxId(F.IR))
        entry = first non-zero S.interfaceL if present, otherwise S.entry
        require entry exists
        alias[entry] = pose
        F.IR[pose].atom = S.IR[entry].atom
        keep all old bonds of pose
        add all internal bonds of S after rewriting entry to pose
        F.defaultNext += rewrite(S.defaultNext, alias)
        return F

p-X attaches X externally to atom p. p-^X replaces atom p with X. For example, 1-N adds an external N substituent, while 1-^N changes atom 1 itself into N and preserves the original bonds at atom 1. Oxygen heteroatoms can be written as 1-^O; replacing with a group such as 1-^OH makes the entry O occupy atom 1 and keeps the attached H.

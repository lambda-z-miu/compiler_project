# my-Chem DSL Prompt Guide

This document is meant to be pasted into another AI as a prompt/reference. Ask the AI to output the original my-Chem DSL expression, not IR, SMILES, or SVG.

## Output Contract

When asked to describe a molecule, output exactly one my-Chem expression:

```txt
<my-Chem>EXPRESSION</my-Chem>
```

The expression is a sequence of CPO fragments. Every CPO ends with `;`.

```txt
CPD  := CPO+
CPO  := "(" LEFT_INTERFACE ")" CORE "," SUBS "(" RIGHT_INTERFACE ")" ";"
```

Do not output SMILES. Do not output IR. Do not output SVG.

## Core Syntax

A CPO has four parts:

```txt
(left-interface)CORE,substitutions(right-interface);
```

Examples:

```txt
(0)Ph,1-OH,(0);
(0)NH2CC,4-Me,5-=O,5-OH,(0);
(0)Ox5,2-=O,3-=,3-OH,4-OH,(0);
```

If there are no substitutions, leave the substitution area empty:

```txt
(0)Ph,(0);
```

## Available Keywords

Atoms and simple fragments:

```txt
C P O N S F Cl Br I
Me Ac NH2 OH SH OMe OPh =O
```

Rings and common fragments:

```txt
Ph Im Ind Pyr Ox5
nR nL
```

Bond modifiers:

```txt
=   double bond
#   triple bond
```

`nR` means an all-carbon ring of size `n`, such as `5R`, `6R`, `8R`.

`nL` means an all-carbon line/chain of length `n`, such as `2L`, `4L`, `22L`.

`Ox5` is a five-membered oxygen-containing ring. Its atom 1 is O, atoms 2-5 are C.

`O` is available as a generic oxygen atom keyword. For an oxygen heteroatom inside an existing carbon ring, prefer replacement syntax such as `1-^O` rather than external attachment `1-O`.

## Local Numbering

All atom numbers are local to the current CPO core, not global molecule numbers.

For `nR`, atoms are numbered around the ring:

```txt
1-2-3-...-n-1
```

For `nL`, atoms are numbered along the chain:

```txt
1-2-3-...-n
```

For `Ph`, atoms are numbered around the phenyl ring 1-6.

For `Ox5`, atom 1 is oxygen, then 2-3-4-5 around the ring.

Substitution positions always refer to the original current core numbering. Inserted substituent atoms do not change later substitution positions in the same CPO.

## Substitutions

Attach a substituent to one or more positions:

```txt
position-SUB,
position1,position2-SUB,
```

Examples:

```txt
1-OH,
2-Me,
5-=O,
1,2,3-OMe,
```

`p-=O` attaches a double-bonded oxygen to atom `p`.

`p-=` does not attach a new atom. It upgrades the default internal bond from atom `p` to its default next atom into a double bond.

`p-^X` replaces atom `p` with substituent/group `X` instead of attaching `X` externally. The replaced position keeps its original bonds. The entry atom of `X` occupies the original atom id, and any extra atoms in `X` are merged as part of the same fragment.

Examples:

```txt
1-^O,       replace atom 1 with oxygen, useful for oxa rings
2-^N,       replace atom 2 with nitrogen
3-^OH,      replace atom 3 with oxygen and keep the H from OH
4-^[(1)COH,(0)],  replace atom 4 with the entry atom of a structured group
```

Examples:

```txt
(0)6R,1-=,3-=,5-=,(0);
```

This makes double bonds at 1-2, 3-4, and 5-6 in the local `6R` ring.

## Group Substitutions

Use `[CPO]` when a substituent is itself a structured fragment:

```txt
position-[(left)CORE,subs(right)],
```

The group attaches through its nonzero left interface. For group substituents, use exactly one nonzero left interface.

Examples:

```txt
4-[(1)COH,(0)],
5-[(1)CC,1-OH,2-OH,(0)],
4-[(1)8L,1-Me,5-Me,(0)],
```

## Interfaces Between CPOs

CPO fragments are connected left to right through their interfaces.

Use `(0)` or `(0,0)` to mean no external connection at an end.

### Simple Connection

If the left CPO has one right-interface atom and the next CPO has one left-interface atom, they are connected by a normal bond:

```txt
(0)6R,(3);(1)2L,(0);
```

This connects atom 3 of the first CPO to atom 1 of the second CPO.

If the right CPO core begins with `=`, the cross-CPO connection is a double bond:

```txt
(0)6R,(3);(1)=2L,(0);
```

### Ring Fusion

If both sides provide two nonzero interface atoms, the two CPOs are fused. The paired interface atoms become the same atoms:

```txt
(0)Ph,(4,5);(1,2)7R,(0,0);
```

This fuses `Ph` atoms 4 and 5 with `7R` atoms 1 and 2.

Two-interface fusion is not a normal bond. It means shared atoms.

## Important Semantic Pitfalls

`1-N` means attach an external N to atom 1. It does not replace atom 1 with N. Use `1-^N` to replace atom 1 with N. Use `1-^O` to make an oxa/oxygen heteroatom at atom 1 while preserving the original ring bonds.

`p-=` uses the current CPO local numbering and upgrades the internal default bond `p -> defaultNext[p]`.

Do not continue a core after terminal groups such as `Me`, `OH`, `SH`, `OMe`, `OPh`, `Ac`, `=O`, and halogens. Use them as substituents or as the end of a core.

Every CPO must contain the comma after `CORE`, even if there are no substitutions.

Adjacent CPO interfaces must match: one nonzero port connects to one nonzero port; two nonzero ports fuse with two nonzero ports.

## Amino Acid Pattern

Neutral amino acid backbone:

```txt
(0)NH2CC,5-=O,5-OH,(0);
```

Why carboxyl carbon is position 5:

`NH2` explicitly creates N plus two H atoms. Therefore in `NH2CC`:

```txt
1 = N
2 = H
3 = H
4 = alpha carbon
5 = carboxyl carbon
```

Examples:

```txt
Gly: (0)NH2CC,5-=O,5-OH,(0);
Ala: (0)NH2CC,4-Me,5-=O,5-OH,(0);
Ser: (0)NH2CC,4-[(1)COH,(0)],5-=O,5-OH,(0);
Phe: (0)NH2CC,4-[(1)CPh,(0)],5-=O,5-OH,(0);
Asp: (0)NH2CC,4-[(1)CC,2-=O,2-OH,(0)],5-=O,5-OH,(0);
```

Proline is special because the side chain closes back into the backbone nitrogen. Use `Pyr`:

```txt
(0)Pyr,2-[(1)C,1-=O,1-OH,(0)],(0);
```

## Vitamin C Example

Current my-Chem expression:

```txt
(0)Ox5,2-=O,3-=,3-OH,4-OH,5-[(1)CC,1-OH,2-OH,(0)],(0);
```

Interpretation:

`Ox5` gives a five-membered lactone-like oxygen ring. `2-=O` places the carbonyl. `3-=` makes the C3-C4 double bond. `3-OH` and `4-OH` place enediol hydroxyls. The side chain attaches at atom 5 as a two-carbon group with hydroxyls.

## Vitamin D Example

Current my-Chem expression:

```txt
(0)6R,1-OH,4-=C,(3);(1)=2L,(2);(1)=6R,(4,5);(1,2)5R,1-Me,5-[(1)8L,1-Me,5-Me,(0)],(0,0);
```

Interpretation:

The first `6R` is the A ring. `1-OH` adds the hydroxyl. `4-=C` creates an exocyclic methylene. The next `(1)=2L` creates a two-carbon linker connected by a double bond to the A ring. The following `6R` is connected by another double bond through its leading `=`. The final `5R` is fused to the preceding `6R` via `(4,5)` to `(1,2)`. The angular methyl is placed on fused atom `1` of the final `5R`.

## A Good Prompt For Another AI

Use this instruction:

```txt
You must output only a my-Chem DSL expression wrapped in <my-Chem>...</my-Chem>. Do not output SMILES, IR, SVG, explanation, or markdown.

Use the grammar:
CPO = "(" left ")" CORE "," SUBS "(" right ")" ";"
CPD = one or more CPOs.

Use local atom numbering inside each CPO. Use one-interface connections for normal bonds and two-interface connections for fused rings. Use p-= to upgrade the current CPO internal default bond p->defaultNext[p]. Use p-=O for carbonyl oxygen. Use p-^X to replace atom p with X instead of attaching X. Use [CPO] for structured substituents with exactly one nonzero left interface.

Available keywords: C, P, O, N, S, F, Cl, Br, I, Me, Ac, NH2, OH, SH, OMe, OPh, =O, Ph, Im, Ind, Pyr, Ox5, nR, nL, =, #.

Do not invent unsupported keywords. Do not use 1-N to replace a ring atom; it attaches an external N. Use 1-^N or 1-^O for ring-atom replacement/heteroatom substitution.
```


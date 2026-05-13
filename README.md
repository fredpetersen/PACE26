# PACE26

A solver for the MAF problem to be submitted for the PACE Challenge 2026

## Build:

```
make build
```

## Running:

the pace26stride tool is a requirement, see <pace26stride link here>

```
stride -i pace26_exact_pub.lst -s ./pace26.out
```

## Compile flags

Compile flags for disabling CPS reduction, both for the initial reduction, and runtime reductions.

| Compile flag                      | Init CPS           | Runtime CPS |
| --------------------------------- | ------------------ | ----------- |
| (none)                            | yes                | yes         |
| `-DDISABLE_RUNTIME_CPS_REDUCTION` | yes (full cascade) | no          |
| `-DDISABLE_CPS_REDUCTION`         | no                 | no          |

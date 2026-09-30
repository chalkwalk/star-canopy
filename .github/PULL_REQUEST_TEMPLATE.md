## What this changes

<!-- The effect, not the mechanism. One or two sentences. -->

## Why

<!-- The problem it solves. Link the issue if there is one. -->

## Checks

- [ ] `cmake --build build` is clean and `ctest --test-dir build --output-on-failure`
      passes -- output quoted below
- [ ] Tests added or extended for the behaviour I changed, and I saw them fail
      without the change
- [ ] **If it changes the look:** the blind comparison, how many skies, and what
      it scored (PRINCIPLES §1, §2). A change to the look is a new look version.
- [ ] **If it adds a control:** why it is not a macro (PRINCIPLES §5, fence #7)
- [ ] Documentation updated in the same commit, if this changed behaviour
- [ ] No reference imagery, and no reference game named, anywhere (fence #4)

If any box is unticked, say why here rather than removing it.

## Environment tested

<!-- Platform and GPU. If you tested on Windows or macOS, say so prominently --
     that is the gap this project most needs closed. -->

# The world's first pure binary editor
> real developers flip there own bits
>
> \- Quote from a Discord server

## One does not simply program in binary.

Even with current 21st century technology, it is almost impossible to feasibly edit and create files purely in base-2. The most direct interaction one can have with the bare metal hardware is a mere abstraction known as hexadecimal. You see, for years elitist programmers have been limiting the potential of new developers with high level languages and so called "hex editors". It is time to unveil these elites thereby putting an end to the troubles they have caused.

Introducing `binedit`, a user-friendly binary editor which runs in the terminal. There are only a few simple keys to remember:
```
arrow up   - go to previous byte
arrow down - go to next byte
arrow < >  - switch between bits
space      - toggle bit
i          - append new byte
d          - delete current byte
s          - save to file
q          - save and exit
```

## Installing
```
$ make
$ make install
```

## Uninstalling
Did you find it too complicated?

no worries:
```
$ make uninstall
```

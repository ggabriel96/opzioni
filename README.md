# opzioni

opzioni is a command line arguments parser library for C++.
Its goals are, in order of importance:

1. **Be as simple and enjoyable as possible.**

    This mainly targets the user of the library, but also includes the user of the command line tool built with it.

1. **If it compiles, it works.**

    That's utopic, but that's what is being strived for.
    Most of the time, all the information needed to build a command line interface is available at compile-time, so we should take advantage of that.

1. **Be bleeding-edge.**

    This library requires C++20. That limits a lot its potential users, but also allows for the use of the new and powerful features of C++. It also helps to accomplish the previous goals.

These goals ought to be discussed in further detail in a separate document.

# Disclaimer

### I would say this is a hobby project to be used in other hobby projects:

- This is a **personal project** with **no promise of maintainability** for the time being.

    I started it to learn more about C++ and its new features.

- Although it is *not* in *early* development, since I'm working on it for months and iterated over it many times, **I do not consider it stable**.

- There are **many unit tests missing**.

- I frequently changed the interface of the library and I'm **not afraid of changing it radically again** if I think it would improve the UX.

    Another example is the names of the `namespaces` and what is in them.

- There is *a lot* of polish and optimization work to do.

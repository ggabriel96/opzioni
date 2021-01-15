# Customization

## `Program`

The following sections explain the various attributes of `Program` and how to set them via the provided member functions.

In code, however, they are not member variables of `Program` (that's why I called them attributes instead).
They are actually member variables of the `ProgramMetadata` class, held in the `metadata` member variable of `Program`.

This detail deserves a longer explanation somewhere else, so to keep things short here, it suffices to say that it serves the purpose of having a common data structure between `Program` and `ProgramView`.
`ProgramView` is like a `std::string_view`, but for `Program`.
All program data is accessible through it, including its arguments and commands.


### `name`

The first and most basic information of `Program` is `name`.
It is the only attribute that the user is required to provide.
Thus, it is specified in the constructor:

```cpp
constexpr auto wget = Program("wget");
```

It is used, for example, when printing the automatic help text, and as identifier when adding a program as command of another program.

Output of the snippet above[^snippet-output]:

```
wget

Usage:
    wget [--help]

Options & Flags:
    -h, --help    Display this information
```

### `title`

The `title` is like the headline of the program.
It is optional, but is also given in the constructor, as the second argument:

```cpp
constexpr auto wget = Program("wget", "a non-interactive network retriever");
```

There is currently no special use for it, so it is only informational.

Output of the snippet above:

```
wget - a non-interactive network retriever

Usage:
    wget [--help]

Options & Flags:
    -h, --help    Display this information
```

### `version`

`version` is... well, the program version.
It is specified via the `version` member function and can be any string.

```cpp
constexpr auto wget = Program("wget", "a non-interactive network retriever")
                        .version("1.20.3");
```

In the automatic help, it is printed right next to the program name in the title line, so it is best to keep it within a maximum of ~16 characters.

Output of the snippet above:

```
wget 1.20.3 - a non-interactive network retriever

Usage:
    wget [--help]

Options & Flags:
    -h, --help    Display this information
```

### `introduction`

`introduction` is what goes between the program name and its list of arguments.
So, for example, it can be used as an actual introduction to the program and what is does, or some message about its usage.
It is specified via the `intro` member function.
Try to keep it short (2 or 3 lines maximum).

```cpp
constexpr auto wget = Program("wget", "a non-interactive network retriever")
                        .version("1.20.3")
                        .intro("Mandatory arguments to long options are mandatory for short options too.");
```

Output of the snippet above:

```
wget 1.20.3 - a non-interactive network retriever

Mandatory arguments to long options are mandatory for short options too.

Usage:
    wget [--help]

Options & Flags:
    -h, --help    Display this information
```

### `description`

`description` is a piece of text that goes after the list of arguments.
It can be used, for example, to add some details that weren't relevant before, like website, documentation, or contact information.
It is specified via the `details` member function.

```cpp
constexpr auto wget = Program("wget", "a non-interactive network retriever")
                        .version("1.20.3")
                        .intro("Mandatory arguments to long options are mandatory for short options too.")
                        .details("Email bug reports, questions, discussions to <bug-wget@gnu.org>"
                                 " and/or open issues at https://savannah.gnu.org/bugs/?func=additem&group=wget.");
```

Note that C++ allows strings to be break into separate pieces for better code formatting, but that does not affect the output of the automatic help.
Don't make it too long, though, as that will move the argument list up in the help text.

Output of the snippet above:

```
wget 1.20.3 - a non-interactive network retriever

Mandatory arguments to long options are mandatory for short options too.

Usage:
    wget [--help]

Options & Flags:
    -h, --help    Display this information

Email bug reports, questions, discussions to <bug-wget@gnu.org> and/or open issues at
https://savannah.gnu.org/bugs/?func=additem&group=wget.
```

### `msg_width`

The `msg_width` is what determines the maximum width of the messages that opzioni will output related to the program.
For example, it is used when printing the automatic help and when using the built-in error handling actions.
The default is `100` and it can be changed via the `max_width` member function.

```cpp
constexpr auto wget = Program("wget", "a non-interactive network retriever")
                        .version("1.20.3")
                        .intro("Mandatory arguments to long options are mandatory for short options too.")
                        .details("Email bug reports, questions, discussions to <bug-wget@gnu.org>"
                                 " and/or open issues at https://savannah.gnu.org/bugs/?func=additem&group=wget.")
                        .max_width(64);
```

Output of the snippet above:

```
wget 1.20.3 - a non-interactive network retriever

Mandatory arguments to long options are mandatory for short
options too.

Usage:
    wget [--help]

Options & Flags:
    -h, --help    Display this information

Email bug reports, questions, discussions to <bug-wget@gnu.org>
and/or open issues at
https://savannah.gnu.org/bugs/?func=additem&group=wget.
```

It is a best-effort, though, so don't make it too narrow.
If the maximum width is not enough for long words (like the URL above), they will be printed in a dedicated line anyway
(try this last snippet with a maximum width of 32).

### `error_handler`

`error_handler` determines how the program behaves when a user error occurs.
This doesn't affect the automatic help text.

The default is to print the error message and the usage of the program.
Change it via the `on_error` member function.
For example, to only print the error message:

```cpp
constexpr auto wget = Program("wget", "a non-interactive network retriever")
                        .version("1.20.3")
                        .intro("Mandatory arguments to long options are mandatory for short options too.")
                        .details("Email bug reports, questions, discussions to <bug-wget@gnu.org>"
                                 " and/or open issues at https://savannah.gnu.org/bugs/?func=additem&group=wget.")
                        .max_width(64)
                        .on_error(print_error);
```

This also deserves a longer documentation, but until that's available, here is a brief explanation:

The argument to `on_error` must be of the `ErrorHandler` type, which is actually a `using` declaration for the following function pointer:

```cpp
using ErrorHandler = int (*)(ProgramView const, UserError const &);
```

`ProgramView` was introduced [earlier](#program).

`UserError` is the base class of the exceptions that are thrown when a user error occurs (user as in user of the command-line program).
All opzioni exceptions inherit from `std::exception`, so the error message can be obtained via their `what` member function.

Currently, the built-in error handlers are:

- `print_error`: print the error message only.
- `print_error_and_usage`: print the error message and the program usage (the default).
- `rethrow`: rethrow the exception.

Just a detail about `rethrow`: note that, given the signature of `ErrorHandler`, the information of the actual class of the exception that was thrown is lost.
In order to preserve their type information (and also be able to granularly catch exceptions, if that's desired), use the `parse` free function instead of `Program`'s call operator when parsing the command-line.
Just add a `try` and as many `catch`es as desired to the following snippet:

```cpp
auto const map = parse(wget, {argv, argc});
```

`argc` and `argv` are "flipped" and within `{}` because that's constructing a `std::span`.

Calling `parse` instead of `Program::operator()` avoids the built-in `try... catch` (which invokes `error_handler`).
This means that "unsetting" or clearing `error_handler` isn't needed.
In fact, `on_error(nullptr)` or `on_error(NULL)` doesn't compile.
`error_handler` is a required attribute of `Program`.

<!-- footnotes -->
[^snippet-output]: snippet output generated by adding the automatic help to the example.

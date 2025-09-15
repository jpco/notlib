# notlib

A simple C library for building notification servers.  Not ready for the limelight just yet.

Notlib depends on gio/gobject/glib (in particular, the `dbus-glib` package on archlinux, or equivalents in other environments) for its dbus implementation and main loop.


## Installation

Installation should be wildly simple:

0. Install packages required to include `<glib.h>` and `<gio.h>`.
1. `make`
2. `sudo make install`

Alternately, you can build a static library with

1. `make static`

and then do whatever you want with the produced file `libnotlib.a`.


## Features

There are currently three optional features, which may be enabled or disabled by setting the build flags `-D${NL_FEATURE}=0` or `-D${NL_FEATURE}=1`.  These features are:

 - `NL_ACTIONS`: Controls whether the server handles actions.  Corresponds with the `actions` capability.  By default, `-DNL_ACTIONS=1`.
 - `NL_REMOTE_ACTIONS`: Controls whether the server supports an `InvokeAction` message which allows client binaries to invoke actions on open notifications.  Corresponds with the `x-notlib-remote-actions` capability.  Has no effect if `NL_ACTIONS` is not also true.  By default, `-DNL_REMOTE_ACTIONS=0`.
 - `NL_URGENCY`: Controls whether notlib specially handles the "urgency" hint.  If disabled, notlib will handle urgency like any other hint.  By default, `-DNL_URGENCY=1`.


## API

The notlib API is fairly simple.  The primary data type in notlib is `NLNote`:

```c
typedef struct {
    unsigned int id;
    char *appname;
    char *summary;
    char *body;

    int timeout;
#if NL_ACTIONS
    NLActions *actions;
#endif

#if NL_URGENCY
    enum NLUrgency urgency;
#endif
    NLHints *hints;
} NLNote;
```

Aside from the `hints` field which has special accessor functions described below, these fields are regular C types, which may be accessed in regular C ways.

To invoke notlib, there are two structs,

```c
typedef struct {
    void (*notify)  (const NLNote *);
    void (*close)   (const NLNote *);
    void (*replace) (const NLNote *);
} NLNoteCallbacks;

typedef struct {
    char *app_name;
    char *author;
    char *version;
} NLServerInfo;
```

which users may pass to the function

```c
void notlib_run(NLNoteCallbacks, char **capabilities, NLServerInfo *);
```

This function will run for the duration of the program.  Notlib owns the lifetime of the `const Note *`s passed to the callback functions.

### Hints

Because notification hints are polymorphic (that is, `DBUS_TYPE_VARIANT`), there are a number of helpers to access them.  Notlib currently supports generic hints of these types:

```c
enum NLHintType {
    HINT_TYPE_UNKNOWN = 0,
    HINT_TYPE_INT = 1,
    HINT_TYPE_BYTE = 2,
    HINT_TYPE_BOOLEAN = 3,
    HINT_TYPE_STRING = 4
};
```

There are type-specific functions to access hints, which return true values if the `NLNote` has a hint with that key and type.

```c
extern int nl_get_int_hint    (const NLNote *n, const char *key, int *out);
extern int nl_get_byte_hint   (const NLNote *n, const char *key, unsigned char *out);
extern int nl_get_boolean_hint(const NLNote *n, const char *key, int *out);
extern int nl_get_string_hint (const NLNote *n, const char *key, const char **out);
```

These functions populate their `out` params with the requested hint values.

There are also "generic" accessors,

```c
extern enum NLHintType nl_get_hint_type(const NLNote *n, const char *key);
extern char *nl_get_hint_as_string(const NLNote *n, const char *key);
extern int nl_get_hint(const NLNote *n, const char *key, NLHint *out);
```

the last of which populates a pointer to a tagged union:

```c
typedef struct {
    enum NLHintType type;
    union {
        int i;
        unsigned char byte;
        int bl;
        const char *str;
    } d;
} NLHint;
```

### Actions

If `NL_ACTIONS` is enabled, there are a few helper functions provided for dealing with actions.

```c
extern int nl_invoke_action(unsigned int id, const char *key);

extern const char **nl_action_keys(const NLNote *n);
extern const char *nl_action_name(const NLNote *n, const char *key);
```


## TODO

 - Several more optional features: icon, etc.
    - Build a graphical demo server to force this
 - Support 'x-canonical-private-synchronous'/'x-dunst-stack-tag' (NOT the non-prefixed versions!)
 - Better-defined (or at least thought-through) signal handling
 - Documented variable ownership in callbacks
 - Persistence?  How would that work?

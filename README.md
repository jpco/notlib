# notlib

A simple C library for building notification servers.  Not ready for the limelight just yet.

Notlib depends on gio/gobject/glib (in particular, the `dbus-glib` package on arch linux, or equivalents in other environments) for its dbus implementation and main loop; the intent is to keep these dependencies as small as possible to make it simple to swap them out if desired.


## Installation

Installation should be wildly simple:

0. Install packages required to include `<glib.h>` and `<gio.h>`.
1. `make`
2. `sudo make install`

Alternately, you can build a static library with

1. `make static`

and then do whatever you want with the produced file `libnotlib.a`.


## Features

There are currently two optional features, which may be enabled or disabled by setting the build flags `-D${FEATURE}=0` or `-D${FEATURE}=1`.  These features are:

 - ACTIONS: Controls whether the server has the "actions" capability.
 - URGENCY: Controls whether notlib specially handles the "urgency" hint.  If disabled, notlib will handle urgency like every other hint.


## API

The notlib API is extremely simple.  There are two structs,

```c
typedef struct {
    void (*notify)  (const Note *);
    void (*close)   (const Note *);
    void (*replace) (const Note *);
} NoteCallbacks;

typedef struct {
    char *app_name;
    char *author;
    char *version;
} ServerInfo;
```

which users may pass to the function

```c
void notlib_run(NoteCallbacks, ServerInfo *);
```

This function will run for the duration of the program.  Notlib owns the lifetime of the `const Note *`s passed to the callback functions; to free or modify them is an error.

The threading semantics of notlib are undefined.


## TODO

 - Several more optional features: icon, etc.
 - Fix "replaces ID of nonexistent note" behavior
 - Hints in general.
 - A standard `close()` function in the API.
 - Invoking actions.
 - Client-configurable responses to the `GetCapabilities` method.
 - A clearer (documented) idea of threading: does a long-running callback block receipt of further notifications?
 - Configurable default timeout.
 - Proper handling of errors that may arise.
 - As much as possible, allowing clients to keep from having to interact with GLib if they don't want to.
    - In particular, functions to copy/access values inside of notes

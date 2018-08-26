# notlib

A simple C library for building notification servers, which depends on the GLib dbus library.

Not ready for the limelight just yet.

## INSTALLATION

Installation should be wildly simple:

0. Install packages required to include `<glib.h>` and `<gio.h>`.
1. `make`
2. `sudo make install`

Alternately, you can build a static library with

1. `make static`

and then do whatever you want with the produced file `libnotlib.a`.

## API

Provide three callback functions of type `void (*)(const Note *)`, populate a `NoteCallbacks` struct with pointers to them, and call `notlib_run()` with that `NoteCallbacks` struct.  `notlib_run()` also takes a pointer to an optional second struct, which determines the return value of the `GetServerInformation` method.

There are currently two optional features, which may be enabled or disabled by setting (for example) `-DACTIONS=0` or `-DACTIONS=1`.  These features are

 - ACTIONS
 - URGENCY

## TODO

 - Several more optional features: icon, etc.
 - Hints in general.
 - A standard `close()` function in the API.
 - Invoking actions.
 - Client-configurable responses to the `GetCapabilities` method.
 - A clearer (documented) idea of threading: does a long-running callback block receipt of further notifications?
 - Configurable default timeout.
 - Proper handling of errors that may arise.
 - As much as possible, allowing clients to keep from having to interact with GLib if they don't want to.

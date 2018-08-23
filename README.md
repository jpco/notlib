# notlib

A simple C library for building notification servers, which depends on the GLib dbus library.

Not ready for the limelight just yet.

## INSTALLATION

Installation should be wildly simple:

0. Install packages required to include `<glib.h>` and `<gio.h>`.
1. `make`
2. `sudo make install`

## API

Provide three callback functions of type `void (*)(const Note *)`, populate a `NoteCallbacks` struct with pointers to them, and call `notlib_run()` with that `NoteCallbacks` struct.

There is currently one optional feature, `ACTIONS`, which may be included/excluded via the `-DACTIONS` build-time flag.

## TODO

 - Several more optional features: urgency, icon, etc.
 - Hints in general.
 - A standard `close()` function in the API.
 - Invoking actions.
 - Client-configurable responses to `GetServerInformation` and `GetCapabilities` methods.
 - A clearer (documented) idea of threading: does a long-running callback block receipt of further notifications?
 - Configurable default timeout.
 - Proper error handling.
 - As much as possible, allowing clients to keep from having to interact with GLib.

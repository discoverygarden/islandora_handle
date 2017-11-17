# Islandora Handle

## Introduction

Islandora implementation of the Handle Service API used to interact with a
Handle Server.

API: https://github.com/cordmata/handle-service

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)

## Installation

Install as usual, see
[this](https://drupal.org/documentation/install/modules-themes/modules-7) for
further information.

## Configuration

### Core Configuration

Accessible at `admin/islandora/islandora-handle`.

The necessary information to hit the Handle.net server must be configured,
including its URL, Handle prefix to use, and the username and password to
authenticate.

Additionally, the canonical public hostname of the server may be specified to
avoid potential issues dealing with Handles when accessing the site via
non-canon domains.

There's also a "debug mode", to permit the various API function to be called
without the need for a real Handle server.

### Content Model Configuration

Accessible at `admin/islandora/islandora-handle/content-models`.

The storage of generated Handles on objects is facilitated via XSLTs which
datastreams, targeted by datastream ID and content model. The Handle URL is
provided to XSLTs in the `handle_value` parameter. XSLTs will typically copy all
 the existing elements of the existing XML, inserting the desired element to
 store the Handle URL in the process.

To create a new association, select `--Add a Handle association--`. It it then
possible enter the datastream ID to target ("Datastream name"), the model for
which this association applies ("Content model name"), and the select the XSLT
which will take the `handle_value` and insert it into the given datastream. It
is possible to upload additional XSLTs via the "Upload an XSL" fieldset at the
bottom of the form.

NOTE: It is _not_ necessary to create an association for the `DC` datastream:
When another datastream is configured, it will be added to the DC automatically.

Similarly, it is possible to modify existing associations by selecting them in
the "Select a content model" dropdown. Modifying existing associations can allow
you to have the Handle URL stored in multiple datastreams. It is also possible
to remove associations, after selecting them.

## Troubleshooting/Issues

Having problems or solved a problem? Contact
[discoverygarden](http://support.discoverygarden.ca).

## Maintainers/Sponsors

Current maintainers:

* [discoverygarden](http://www.discoverygarden.ca)

## Development

If you would like to contribute to this module, please check out our helpful
Documentation for 
[Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers)
info, [Developers](http://islandora.ca/developers) section on Islandora.ca and
contact [discoverygarden](http://support.discoverygarden.ca).

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)

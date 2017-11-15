<?php

/**
 * @file
 * Hooks provided by Islandora Handle.
 */

/**
 * Retrieve module supplied XSL files for appending Handles to datastreams..
 *
 * @return array
 *   Returns an associative array where the key is the URI to the file on the
 *   file system and the value is the name of the file.
 */
function hook_islandora_handle_get_xsl_files() {
  return array(
    'path/to/some/file' => 'file',
  );
}

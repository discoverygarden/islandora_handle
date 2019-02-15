<?php

/**
 * @file
 * Performs the tasks of the islandora_handle module.
 */

/**
 * Retrieve all configurations for the specified content models.
 *
 * @param array $models
 *   An array of models to be queried against.
 *
 * @return array
 *   An array describing the result of the database query.
 */
function islandora_handle_retrieve_configurations_by_cmodels(array $models) {
  $handle_results = db_select('islandora_handle_apply_handle', 'h')
    ->fields('h', array('cmodel', 'datastream', 'xsl_location'))
    ->condition('cmodel', $models)
    ->execute()
    ->fetchAll();
  return $handle_results;
}

/**
 * Helper to determine whether the Handle has already been added in metadata.
 *
 * @param DOMDocument $alpha
 *   The content of the datastream currently in DOMDocument form.
 * @param DOMDocument $bravo
 *   THe XML output of the transform.
 *
 * @return bool
 *   TRUE if it exists, FALSE otherwise,
 */
function islandora_handle_datastream_content_equal(DOMDocument $alpha, DOMDocument $bravo) {
  // XXX: DOM in PHP cannot format and remove whitespace within an existing
  // loaded DOM. The DOMDocument::normalizeDocument method should do this but it
  // does not appear to function as expected.
  $clone_alpha = new DOMDocument();
  $clone_bravo = new DOMDocument();
  $docs = array(
    $clone_alpha,
    $clone_bravo,
  );
  foreach ($docs as $doc) {
    $doc->formatOutput = TRUE;
    $doc->preserveWhiteSpace = FALSE;
  }
  // XXX: Some attributes like encoding are stripped away in the inline control
  // group, ignore the XML declarations to ensure accuracy.
  $clone_alpha->loadXML($alpha->saveXML($alpha->documentElement));
  $clone_bravo->loadXML($bravo->saveXML($bravo->documentElement));
  return $clone_alpha->saveXML() == $clone_bravo->saveXML();
}

/**
 * Helper to retrieve all Handle handlers.
 */
function islandora_handle_retrieve_handle_handlers() {
  $handlers = module_invoke_all('islandora_handle_handle_handler');
  return $handlers;
}

/**
 * Helper to get a Handle handler to be used.
 */
function islandora_handle_retrieve_selected_handler() {
  $handler = &drupal_static(__FUNCTION__);

  if (is_null($handler)) {
    $handlers = islandora_handle_retrieve_handle_handlers();
    $handler = $handlers[variable_get('islandora_handle_handler', 'islandora_handle_service')]['class'];
  }
  return $handler;
}

<?php

/**
 * @file
 * Drush scripts.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_handle_drush_command() {
  return array(
    'islandora_handle_update_handles' => array(
      'aliases' => array('ih-uh'),
      'description' => dt('Updates handles for a given list of PIDs for which a handle already exists in the handle service. This includes updating the handle definition on the handle server, and re-running derivatives for any local metadata configured to contain the handle definition.'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'options' => array(
        'pidlist' => array(
          'description' => dt('Text file with newline-separated list of PIDs to update handles for.'),
          'required' => TRUE,
        ),
        'chunk_size' => array(
          'description' => dt('The number of items in the list to process in one cycle of the batch. Defaults to 100.'),
          'required' => FALSE,
        ),
      ),
    ),
  );
}

/**
 * Drush command to kick off the handle upsert batch.
 */
function drush_islandora_handle_update_handles() {
  module_load_include('inc', 'islandora_handle', 'includes/update_handles.batch');
  batch_set(islandora_handle_update_handles_batch_definition(
    drush_get_option('pidlist'),
    drush_get_option('chunk_size', 100)));
  drush_backend_batch_process();
}

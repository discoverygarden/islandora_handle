<?php

/**
 * @file
 * Handle update batch.
 */

/**
 * Batch definition for the handle update.
 *
 * @param string $pidlist
 *   Path to a file containing a newline-separated list of PIDs to process.
 * @param int $chunk_size
 *   The number of items to process at once.
 *
 * @return array
 *   Batch definition.
 */
function islandora_handle_update_handles_batch_definition($pidlist, $chunk_size) {
  return array(
    'title' => t('Updating handle destinations'),
    'file' => drupal_get_path('module', 'islandora_handle') . '/includes/update_handles.batch.inc',
    'operations' => array(
      array(
        'islandora_handle_update_handles_batch_operation',
        array(
          array(
            'pidlist' => $pidlist,
            'chunk_size' => $chunk_size,
          ),
        ),
      ),
    ),
  );
}

/**
 * Batch operation for updating handles.
 *
 * @param array $params
 *   An associative array containing:
 *   - 'pidlist': path to a file full of newline-separated PIDs to process.
 *   - 'chunk_size': The number of items to process at once.
 * @param array|DrushBatchContext $context
 *   Batch context.
 */
function islandora_handle_update_handles_batch_operation(array $params, &$context) {
  // Populate the sandbox.
  $sandbox = &$context['sandbox'];
  if (empty($sandbox)) {
    $sandbox['file_handle'] = fopen($params['pidlist'], 'rb');
    if (!$sandbox['file_handle']) {
      throw new InvalidArgumentException(t('Failed to load the PID list at @pidlist', array(
        '@pidlist' => $params['pidlist'],
      )));
    }
    module_load_include('inc', 'islandora_handle', 'includes/handle');
    $sandbox['current'] = 0;
  }

  // Cycle through some lines, do some thangs.
  fseek($sandbox['file_handle'], $sandbox['current']);
  for ($i = 0; $i < $params['chunk_size']; $i++) {
    if (feof($sandbox['file_handle'])) {
      drush_log(t('@lines lines processed', array(
        '@lines' => $sandbox['current'],
      )));
      $context['finished'] = 1;
      return;
    }
    $pid = trim(fgets($sandbox['file_handle']));
    if (empty($pid)) {
      continue;
    }
    $sandbox['current']++;
    $object = islandora_object_load($pid);
    if (!$object) {
      watchdog('islandora_handle_update_handles',
        'Failed to load PID @pid',
        array('@pid' => $pid),
        WATCHDOG_ERROR);
    }
    else {
      $suffix = islandora_handle_construct_suffix($object);
      $target = islandora_handle_construct_target_url($suffix);
      $handle = variable_get('islandora_handle_server_prefix', '1234567') . "/$suffix";
      $response = islandora_handle_update_handle_url($handle, $target);
      if ($response->code == 200) {
        // Re-run derivative generation for configured DSID sources.
        $configs = islandora_handle_retrieve_configurations_by_cmodels($object->models);
        foreach ($configs as $config) {
          islandora_do_derivatives($object, array(
            'force' => TRUE,
            'source_dsid' => $config->dsid,
          ));
        }
        $message = t('@pid: Updated handle @handle to point to @target', array(
          '@pid' => $pid,
          '@handle' => $handle,
          '@target' => $target,
        ));
      }
      else {
        $message = t('@pid: Failed to update handle @handle; error code @code', array(
          '@pid' => $pid,
          '@handle' => $handle,
          '@code' => $response->code,
        ));
      }
      drush_log($message);
    }
  }
  $context['finished'] = 0;
}

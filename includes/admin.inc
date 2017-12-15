<?php

/**
 * @file
 * Admin form for the Islandora Handle module.
 */

/**
 * Admin settings form for the Islandora Handle module.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The admin form.
 */
function islandora_handle_server_form(array $form, array &$form_state) {
  $form = array();
  $form['server_location'] = array(
    '#type' => 'textfield',
    '#title' => t('Handle Server Service Location'),
    '#description' => t('Location of the Handle.net server'),
    '#default_value' => variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-service'),
  );
  $form['server_handle_admin_user'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#description' => t('The username of the Handle administrator'),
    '#default_value' => variable_get('islandora_handle_server_admin_username', 'handleAdmin'),
  );
  $form['server_handle_admin_password'] = array(
    '#type' => 'password',
    '#title' => t('Password'),
    '#description' => t('The username of the Handle administrator'),
    '#default_value' => variable_get('islandora_handle_server_admin_password', 'superSecretPassword'),
  );
  $form['server_handle_prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Prefix'),
    '#description' => t('Handle prefix as specified from Handle.net'),
    '#default_value' => variable_get('islandora_handle_server_prefix', '1234567'),
  );
  // islandora_handle_host initial value is empty and will default resolver to
  // ingestion host.
  $form['server_handle_host'] = array(
    '#type' => 'textfield',
    '#title' => t('Hostname ex:http://example.com/'),
    '#description' => t('URL that should be used'),
    '#default_value' => variable_get('islandora_handle_host', ''),
  );
  $form['debug_mode'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug Mode'),
    '#description' => t('Enables debug mode, bypassing the need for an actual handle server.'),
    '#default_value' => variable_get('islandora_handle_debug_mode', FALSE),
  );
  $form['server_handle_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  return $form;
}

/**
 * Validation for the Islandora Handle server form.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_handle_server_form_validate(array $form, array &$form_state) {
  if (valid_url($form_state['values']['server_location'], TRUE) == FALSE) {
    form_set_error('server_location', t('Must be a valid URL!'));
  }
  if (!is_numeric($form_state['values']['server_handle_prefix'])) {
    form_set_error('server_handle_prefix', t('Handle prefix must be numeric!'));
  }
}

/**
 * The Islandora Handle server form submit handler.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_handle_server_form_submit(array $form, array &$form_state) {
  variable_set('islandora_handle_server_url', $form_state['values']['server_location']);
  variable_set('islandora_handle_server_admin_username', $form_state['values']['server_handle_admin_user']);
  variable_set('islandora_handle_server_admin_password', $form_state['values']['server_handle_admin_password']);
  variable_set('islandora_handle_server_prefix', $form_state['values']['server_handle_prefix']);

  variable_set('islandora_handle_host', $form_state['values']['server_handle_host']);

  variable_set('islandora_handle_debug_mode', (bool) $form_state['values']['debug_mode']);

  // Only create the debug table if needed.
  if (!db_table_exists('islandora_handle_debug_handles') && variable_get('islandora_handle_debug_mode', FALSE)) {
    $table = array(
      'description' => 'Stores handles used during debug mode.',
      'fields' => array(
        'suffix' => array(
          'description' => 'The suffix of the handle.',
          'type' => 'varchar',
          'length' => 255,
          'not null' => TRUE,
        ),
        'handle' => array(
          'description' => "The full handle, concatenated from the handle server's prefix and the individual handle entry's suffix.",
          'type' => 'varchar',
          'length' => 255,
        ),
        'location' => array(
          'description' => "The location where the debug Handle is pointing to.",
          'type' => 'varchar',
          'length' => 255,
        ),
      ),
      'primary key' => array('suffix'),
    );
    db_create_table('islandora_handle_debug_handles', $table);
    db_table_exists('islandora_handle_debug_handles') ? drupal_set_message(t('Successfully created a handle debug table in the database.')) : drupal_set_message(t('An error occurred while attempting to create the handle debug table in the database.'), 'error');
  }
  elseif (db_table_exists('islandora_handle_debug_handles') && !variable_get('islandora_handle_debug_mode', FALSE)) {
    db_drop_table('islandora_handle_debug_handles');
    if (db_table_exists('islandora_handle_debug_handles')) {
      drupal_set_message(t('An error occurred while attempting to remove the handle debug table from the database.'), 'error');
    }
    else {
      drupal_set_message(t('Successfully removed the handle debug table from the database.'));
    }
  }

  drupal_set_message(t('The settings have been saved!'));
}

/**
 * Form building function for the content model form.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The constructed content model form.
 */
function islandora_handle_cmodel_form(array $form, array &$form_state) {
  $form = array();
  $handle_hide = FALSE;
  $upload_file = FALSE;
  $form['cmodel_wrapper'] = array(
    '#prefix' => '<div id="cmodel-content">',
    '#suffix' => '</div>',
    '#weight' => 1,
  );
  if (isset($form_state['triggering_element'])) {
    $table_render = FALSE;
    if ($form_state['triggering_element']['#name'] == 'remove_selected') {
      $options = array();
      $total = count($form_state['values']['tableselect']);
      foreach ($form_state['values']['tableselect'] as $checkbox => $value) {
        if ($value !== 0) {
          $options[] = $checkbox;
        }
      }
      if (empty($options)) {
        drupal_set_message(t('Please select at least one Handle association to remove!'), 'error');
      }
      else {
        foreach ($options as $dsid) {
          db_delete('islandora_handle_apply_handle')
            ->condition('datastream', $dsid, '=')
            ->condition('cmodel', $form_state['values']['cmodel_selector'], '=')
            ->execute();
        }
        if ($total == count($options)) {
          $handle_hide = TRUE;
        }
        drupal_set_message(t('Removed the selected Handle associations!'));
      }
    }
    elseif ($form_state['triggering_element']['#name'] == 'cmodel_add' && !$handle_hide) {
      if (!isset($form_state['values']['cmodel_name'])) {
        $cmodel = $form_state['values']['cmodel_selector'];
      }
      else {
        $cmodel = $form_state['values']['cmodel_name'];
      }
      db_insert('islandora_handle_apply_handle')
        ->fields(array(
          'cmodel' => $cmodel,
          'datastream' => $form_state['values']['datastream_name'],
          'xsl_location' => $form_state['values']['handle_xsls'],
        ))
        ->execute();
      drupal_set_message(t('Added a Handle association for the @ds datastream on the @cm content model!', array(
        '@ds' => $form_state['values']['datastream_name'],
        '@cm' => $cmodel,
      )));
    }
    elseif ($form_state['triggering_element']['#name'] == 'xsl_upload_button') {
      $upload_file = TRUE;
      $validators = array(
        'file_validate_extensions' => array('xslt xsl'),
      );
      $file = file_save_upload('xsl_upload_field', $validators, 'public://islandora_handle_xsls');

      if ($file) {
        $file->status = FILE_STATUS_PERMANENT;
        file_save($file);
        drupal_set_message(t("The file @filename was uploaded successfully.",
          array(
            '@filename' => $file->filename,
          )
        ));
      }
      else {
        drupal_set_message(t('Failed to upload the specified file.'), 'error');
      }
    }
    $table_trig = array(
      'blank',
      'add',
    );
    // Render our table.
    if (!in_array($form_state['triggering_element']['#value'], $table_trig) && $form_state['values']['cmodel_selector'] != 'add' && !$handle_hide) {
      $table_render = TRUE;
      $cmodel_selected = $form_state['values']['cmodel_selector'];
      $handle_results = db_select('islandora_handle_apply_handle', 'h')
        ->fields('h', array('datastream', 'xsl_location'))
        ->condition('cmodel', $cmodel_selected, '=')
        ->execute()
        ->fetchAllAssoc('datastream');

      $header = array('Datastream', 'XSL Location');
      $rows = array();
      foreach ($handle_results as $handle_obj) {
        $rows[$handle_obj->datastream] = array($handle_obj->datastream, $handle_obj->xsl_location);
      }
      $form['cmodel_wrapper']['table']['tableselect'] = array(
        '#header' => $header,
        '#type' => 'tableselect',
        '#options' => $rows,
        '#weight' => 5,
        '#empty' => t('No associations currently present!'),
      );
      $form['cmodel_wrapper']['table']['remove_selected'] = array(
        '#type' => 'button',
        '#value' => t('Remove selected'),
        '#weight' => 6,
        '#name' => 'remove_selected',
        '#ajax' => array(
          'wrapper' => 'cmodel-content',
          'callback' => 'islandora_handle_table_ajax',
        ),
      );
    }
    if ($form_state['triggering_element']['#value'] == 'add' || $table_render == TRUE || $upload_file == TRUE || $form_state['triggering_element']['#name'] == 'cmodel_add') {
      $xsl_options = array('default' => t('-- Select an XSLT --'));
      $handle_append_files = module_invoke_all('islandora_handle_get_xsl_files');
      $transform_options = array_merge($xsl_options, $handle_append_files);
      $form['cmodel_wrapper']['add'] = array(
        '#type' => 'fieldset',
        '#collapsed' => FALSE,
        '#collapsible' => FALSE,
        '#title' => t('Add Handle association'),
        '#weight' => 10,
        'handle' => array(
          'datastream_name' => array(
            '#type' => 'textfield',
            '#title' => t('Datastream name'),
            '#description' => t('The name of the datastream for the Handle value to be applied to.'),
          ),
        ),
        'handle_xsls' => array(
          '#type' => 'select',
          '#title' => t('XSL Location'),
          '#options' => $transform_options,
          '#description' => t('The location of the XSL which will apply the Handle value to the datastream.'),
        ),
        'add_button' => array(
          '#type' => 'button',
          '#name' => 'cmodel_add',
          '#value' => t('Add'),
          '#ajax' => array(
            'wrapper' => 'cmodel-content',
            'callback' => 'islandora_handle_table_ajax',
          ),
        ),
      );
      $form['cmodel_wrapper']['xsl_upload'] = array(
        '#type' => 'fieldset',
        '#weight' => 20,
        '#collapsed' => TRUE,
        '#collapsible' => TRUE,
        '#title' => t('Upload an XSL'),
        '#prefix' => '<div id="islandora-handle-upload">',
        '#suffix' => '</div>',
        'xsl_upload_field' => array(
          '#type' => 'file',
          '#title' => t('Upload an XSL or XSLT file to be used for transformations'),
        ),
        'upload_button' => array(
          '#type' => 'button',
          '#name' => 'xsl_upload_button',
          '#value' => t('Upload'),
          '#ajax' => array(
            'callback' => 'islandora_handle_handle_ajax',
            'wrapper' => 'cmodel-content',
          ),
        ),
      );
      if (!$table_render) {
        $form['cmodel_wrapper']['add']['handle']['cmodel_name'] = array(
          '#type' => 'textfield',
          '#title' => t('Content model name'),
          '#description' => t('The name of the content model to hook on.'),
          '#autocomplete_path' => ISLANDORA_CONTENT_MODELS_AUTOCOMPLETE,
        );
      }
    }
  }
  $options = array(
    'blank' => t('-- Select a content model --'),
    'add' => t('-- Add a Handle association --'),
  );
  $cmodel_results = db_select('islandora_handle_apply_handle', 'h')
    ->fields('h', array('cmodel'))
    ->execute()
    ->fetchAllAssoc('cmodel');

  foreach ($cmodel_results as $cmodel) {
    $options[$cmodel->cmodel] = $cmodel->cmodel;
  }
  $form['cmodel_wrapper']['cmodel_selector'] = array(
    '#type' => 'select',
    '#title' => t('Select a content model'),
    '#options' => $options,
    '#default_value' => 'blank',
    '#ajax' => array(
      'wrapper' => 'cmodel-content',
      'callback' => 'islandora_handle_handle_ajax',
    ),
    '#weight' => -10,
  );
  return $form;
}

/**
 * AJAX callback for the content model form.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The portion of the form which we are rendering.
 */
function islandora_handle_handle_ajax(array $form, array &$form_state) {
  return $form['cmodel_wrapper'];
}

/**
 * Validation handler for the content model form.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_handle_cmodel_form_validate(array $form, array &$form_state) {
  if (isset($form_state['triggering_element'])) {
    if ($form_state['triggering_element']['#name'] == 'cmodel_add') {
      $valid_cmodel = TRUE;
      $valid_dsid = TRUE;
      if (isset($form_state['values']['cmodel_name'])) {
        $cmodel = $form_state['values']['cmodel_name'];
        if (empty($form_state['values']['cmodel_name'])) {
          form_set_error('cmodel_name', t('Must supply a content model name!'));
          $valid_cmodel = FALSE;
        }
      }
      else {
        $cmodel = $form_state['values']['cmodel_selector'];
      }
      if (empty($form_state['values']['datastream_name'])) {
        form_set_error('datastream_name', t('Must supply a datastream name!'));
        $valid_dsid = FALSE;
      }
      else {
        if ($form_state['values']['datastream_name'] == 'DC') {
          form_set_error('datastream_name', t('A Handle gets appended to the DC datastream automatically.'));
        }
      }
      if ($form_state['values']['handle_xsls'] == 'default') {
        form_set_error('handle_xsls', t('Must supply a valid XSL!'));
      }
      if ($valid_cmodel && $valid_dsid) {
        $handle_results = db_select('islandora_handle_apply_handle', 'h')
          ->fields('h', array('cmodel', 'datastream'))
          ->condition('cmodel', $cmodel, '=')
          ->condition('datastream', $form_state['values']['datastream_name'], '=')
          ->execute()
          ->fetchAssoc();
        if (!empty($handle_results)) {
          form_set_error('handle', t('A Handle association already exists for the @ds datastream on the @cmodel content model!', array(
            '@cmodel' => $cmodel,
            '@ds' => $form_state['values']['datastream_name'],
          )));
        }
      }
    }
  }
}

/**
 * Submit handler for the Islandora Handler content model form.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_handle_cmodel_form_submit(array $form, array &$form_state) {
  if (!isset($form_state['values']['cmodel_name'])) {
    $cmodel = $form_state['values']['cmodel_selector'];
  }
  else {
    $cmodel = $form_state['cmodel_name'];
  }
  db_insert('islandora_handle_apply_handle')
    ->fields(array(
      'cmodel' => $cmodel,
      'datastream' => $form_state['values']['datastream_name'],
      'xsl_location' => $form_state['values']['handle_xsls'],
    ))
    ->execute();
  drupal_set_message(t('Added a Handle association for the @ds datastream on the @cm content model!', array(
    '@ds' => $form_state['values']['datastream_name'],
    '@cm' => $cmodel,
  )));
}

/**
 * AJAX callback for the table portion of the content model form.
 *
 * @param array $form
 *   A Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The portion of the form we are rebuilding.
 */
function islandora_handle_table_ajax(array $form, array &$form_state) {
  return $form['cmodel_wrapper'];
}

/**
 * List all content models and enabled configs for each cmodel.
 *
 * This report is a usability stopgap. It would be better to update the config
 * to use the standard table pattern. This can be removed when that happens.
 *
 * @return html
 *   themed list of content models and handle configs.
 */
function islandora_handle_config_list() {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $table = array(
    '#theme' => 'table',
    '#sticky' => FALSE,
    '#caption' => t('Content Model Configuration for Islandora Handle'),
    '#empty' => t('No content models found.'),
  );
  $header = array(t('Content Model'), t('DSID'), t('Transform'));

  $content_models = islandora_get_content_models();

  $configs = db_select('islandora_handle_apply_handle', 'h')
    ->fields('h', array('cmodel', 'datastream', 'xsl_location'))
    ->orderBy('datastream', 'DESC')
    ->execute()
    ->fetchAll();

  $cmodel_configs = array();
  foreach ($configs as $config) {
    $cmodel_configs[$config->cmodel][] = $config;
  }

  $table['#header'] = $header;
  $rows = array();
  foreach ($content_models as $cmodel) {
    $row = array();
    $row[] = "{$cmodel['label']} ({$cmodel['pid']})";
    if (isset($cmodel_configs[$cmodel['pid']])) {
      $datastreams = array();
      $xsls = array();
      foreach ($cmodel_configs[$cmodel['pid']] as $config) {
        $datastreams[] = $config->datastream;
        $xsls[] = $config->xsl_location;
      }
      $row[] = implode('<br/>', $datastreams);
      $row[] = implode('<br/>', $xsls);
    }
    else {
      $row[] = array(
        'data' => t('No configurations were found.'),
        'colspan' => 2,
      );
    }
    $rows[] = $row;
  }
  $table['#rows'] = $rows;

  return $table;
}

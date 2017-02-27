<?php

/**
 * @file
 * Performs the tasks of the islandora_handle module.
 */

/**
 * Heavy lifting to go out and perform the request and add it to the datastream.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return object
 *   An object describing the response of the construction.
 */
function islandora_handle_construct_handle(AbstractObject $object) {
  $admin_user = variable_get('islandora_handle_server_admin_username', 'handleAdmin');
  $admin_password = variable_get('islandora_handle_server_admin_password', 'superSecretPassword');
  $handle_service_url = islandora_handle_construct_url($object, TRUE);
  $suffix = islandora_handle_construct_suffix($object);
  // $hostvar will be populated if host resolver is different than ingestion
  // service.
  $hostvar = variable_get('islandora_handle_host', '');
  if ($hostvar == '') {
    // We do this with language such that we don't get language specific
    // prefixes in the URL.
    $target = url("islandora/object/$suffix", array(
      'language' => (object) array('language' => FALSE),
      'absolute' => TRUE,
    ));
  }
  else {
    $target = $hostvar . "islandora/object/$suffix";
  }
  $query_target = drupal_http_build_query(array('target' => $target));
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);
  $response = drupal_http_request($handle_service_url, array(
    'headers' => array(
      'Authorization' => $authorization_header,
    ),
    'data' => $query_target,
    'method' => 'POST',
  ));
  return $response;
}

/**
 * Heavy lifting to go out and perform the update request for the Handle.
 *
 * @param string $handle
 *   A Handle that we are updating in prefix/suffix form.
 * @param string $target
 *   The target URL the Handle is being updated to point to.
 *
 * @return object
 *   An object describing the response of the updation.
 */
function islandora_handle_update_handle_url($handle, $target) {
  $admin_user = variable_get('islandora_handle_server_admin_username', 'handleAdmin');
  $admin_password = variable_get('islandora_handle_server_admin_password', 'superSecretPassword');
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);
  $handle_url = url(variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-service') . '/' . $handle, array(
    'query' => array('target' => $target),
  ));
  $response = drupal_http_request($handle_url, array(
    'headers' => array(
      'Authorization' => $authorization_header,
    ),
    'method' => 'PUT',
  ));
  return $response;
}

/**
 * Heavy lifting to go out and perform the request and delete the Handle.
 *
 * @param string $suffix
 *   The suffix of the Handle to be deleted, in our case the PID.
 *
 * @return object
 *   An object describing the deletion of the construction.
 */
function islandora_handle_delete_handle($suffix) {
  $object = islandora_object_load($suffix);
  $admin_user = variable_get('islandora_handle_server_admin_username', 'handleAdmin');
  $admin_password = variable_get('islandora_handle_server_admin_password', 'superSecretPassword');
  $handle_service_url = islandora_handle_construct_url($object, TRUE);
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);
  $response = drupal_http_request($handle_service_url, array(
    'headers' => array(
      'Authorization' => $authorization_header,
    ),
    'method' => 'DELETE',
  ));
  return $response;
}

/**
 * Adds our Handle value to the specified datastream.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 * @param string $datastream
 *   The datastream to which we are applying the Handle value to.
 * @param string $xsl_location
 *   The uri location of the XSL we are applying to append the Handle.
 *
 * @return array
 *   An array describing the outcome of the operation.
 */
function islandora_handle_append_handle(AbstractObject $object, $datastream, $xsl_location) {
  $xslt = new XSLTProcessor();
  $xsl_transform = new DOMDocument();
  $handle_url = islandora_handle_construct_url($object, FALSE);

  $ds_content = $object[$datastream]->content;

  $xsl_transform->load($xsl_location);
  $input = new DomDocument();

  $did_load_ok = $input->loadXML($ds_content);
  if ($did_load_ok) {
    $xslt->importStylesheet($xsl_transform);
    $xslt->setParameter('', 'handle_value', $handle_url);
    $ds_output = $xslt->transformToXml($input);
    if ($ds_output) {
      $object[$datastream]->content = $ds_output;
      return array(
        'success' => TRUE,
        'message' => array(
          'message' => t('Appended Handle to @ds datastream for @pid!'),
          'message_sub' => array('@pid' => $object->id, '@ds' => $datastream),
          'type' => 'dsm',
        ),
      );
    }
    else {
      return array(
        'success' => FALSE,
        'message' => array(
          'message' => t('Appending the Handle value for @obj on the @ds datastream failed!'),
          'message_sub' => array('@obj' => $object->id, '@ds' => $datastream),
          'type' => 'watchdog',
          'severity' => WATCHDOG_ERROR,
        ),
      );
    }
  }
  else {
    return array(
      'success' => FALSE,
      'message' => array(
        'message' => t('Unable to load content of the @ds datastream for @obj!'),
        'message_sub' => array('@obj' => $object->id, '@ds' => $datastream),
        'type' => 'watchdog',
        'severity' => WATCHDOG_ERROR,
      ),
    );
  }
}

/**
 * Constructs the Handle server URL.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @param bool $service
 *   Whether this is the service URL or the Handle.net URL.
 *
 * @return string
 *   The full Handle Service URL containing the Handle.
 */
function islandora_handle_construct_url($object, $service) {
  $suffix = islandora_handle_construct_suffix($object);
  $prefix = variable_get('islandora_handle_server_prefix', '1234567');

  if ($service) {
    $url = variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-service');
  }
  else {
    $url = 'http://hdl.handle.net';
  }
  return "$url/$prefix/$suffix";
}

/**
 * Constructs the suffix for the Handle URL.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return string
 *   The full Handle Service URL containing the Handle.
 */
function islandora_handle_construct_suffix($object) {
  // For now we are just specifying that the suffix is the PID of the object.
  // In the future we can modify this as we see fit.
  return $object->id;
}

/**
 * Sees if a Handle exists for a specified.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return bool
 *   Whether the object has a Handle present on the Handle server.
 *
 * @note: This will have to change if and when we allow the user to specify a
 * suffix that is not the PID. Likely done with a RELS statement.
 */
function islandora_handle_query_for_handle(AbstractObject $object) {
  $admin_user = variable_get('islandora_handle_server_admin_username', 'handleAdmin');
  $admin_password = variable_get('islandora_handle_server_admin_password', 'superSecretPassword');
  $handle = islandora_handle_construct_url($object, TRUE);
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);
  $response = drupal_http_request($handle, array(
    'headers' => array(
      'Authorization' => $authorization_header,
    ),
  ));
  if ($response->code == 204) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/**
 * Retrieve all configurations for the specified content models.
 *
 * @param array $models
 *   An array of models to be queried against.
 *
 * @return array
 *   An array describing the result of the database query.
 */
function islandora_handle_retrieve_configurations_by_cmodels($models) {
  $handle_results = db_select('islandora_handle_apply_handle', 'h')
    ->fields('h', array('cmodel', 'datastream', 'xsl_location'))
    ->condition('cmodel', $models)
    ->execute()
    ->fetchAll();
  return $handle_results;
}

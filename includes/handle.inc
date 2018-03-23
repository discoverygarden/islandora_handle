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
  $target = islandora_handle_construct_target_url($suffix);
  $query_target = drupal_http_build_query(array('target' => $target));
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);

  if (variable_get('islandora_handle_debug_mode', FALSE)) {
    $response = new stdClass();
    $response->code = 201;
    db_insert('islandora_handle_debug_handles')
      ->fields(array(
        'suffix' => $suffix,
        'handle' => $handle_service_url,
        'location' => $target,
      ))
      ->execute();
    return $response;
  }

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
 * Helper to construct a target URL.
 *
 * Constructed from islandora/object, appended with the given $suffix and
 * prepended with either the local server's URL or the custom configured
 * variable islandora_handle_host.
 *
 * @param string $suffix
 *   Suffix to append to the URL.
 *
 * @return string
 *   The constructed target URL.
 */
function islandora_handle_construct_target_url($suffix) {
  // $hostvar will be populated if host resolver is different than ingestion
  // service.
  $hostvar = variable_get('islandora_handle_host', '');
  if ($hostvar == '') {
    // We do this with language such that we don't get language specific
    // prefixes in the URL.
    return url("islandora/object/$suffix", array(
      'language' => (object) array('language' => FALSE),
      'absolute' => TRUE,
    ));
  }
  return $hostvar . "islandora/object/$suffix";
}

/**
 * Heavy lifting to go out and get the location that a Handle points to.
 *
 * @param string $handle
 *   A Handle that we are getting the location for in prefix/suffix form.
 *
 * @return object
 *   An object describing the response of the get.
 */
function islandora_handle_get_handle_location($handle) {
  if (variable_get('islandora_handle_debug_mode', FALSE)) {
    // Need to munge just the suffix out for debug case.
    $prefix = variable_get('islandora_handle_server_prefix', '1234567');
    $suffix = str_replace("$prefix/", '', $handle);
    $response = new stdClass();
    $response->code = 204;
    $location = db_select('islandora_handle_debug_handles', 'd')
      ->fields('d', array('location'))
      ->condition('suffix', $suffix, '=')
      ->execute()
      ->fetchField();
    if ($location) {
      $response->headers = array(
        'location' => $location,
      );
    }
    else {
      $response->code = 404;
    }
    return $response;
  }
  $admin_user = variable_get('islandora_handle_server_admin_username', 'handleAdmin');
  $admin_password = variable_get('islandora_handle_server_admin_password', 'superSecretPassword');
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);
  $handle_url = url(variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-service') . '/' . $handle, array());
  $response = drupal_http_request($handle_url, array(
    'headers' => array(
      'Authorization' => $authorization_header,
    ),
    'method' => 'GET',
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
  if (variable_get('islandora_handle_debug_mode', FALSE)) {
    $response = new stdClass();
    $response->code = 200;
    // Munge out of the suffix out of the Handle.
    $prefix = variable_get('islandora_handle_server_prefix', '1234567');
    $suffix = str_replace("$prefix/", '', $handle);
    db_update('islandora_handle_debug_handles')
      ->fields(array('location' => $target))
      ->condition('suffix', $suffix, '=')
      ->execute();
    return $response;
  }
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
  if (variable_get('islandora_handle_debug_mode', FALSE)) {
    $response = new stdClass();
    $response->code = 200;
    $response->error = "There was no error";
    db_delete('islandora_handle_debug_handles')
      ->condition('suffix', $suffix)
      ->execute();
    return $response;
  }
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
    $ds_output = $xslt->transformToDoc($input);
    if ($ds_output) {
      // If there is nothing to change don't make a new version.
      $xml_content = $ds_output->saveXML();
      if (!islandora_handle_datastream_content_equal($input, $ds_output)) {
        $object[$datastream]->content = $xml_content;
        return array(
          'success' => TRUE,
          'message' => array(
            'message' => t('Appended Handle to @ds datastream for @pid!'),
            'message_sub' => array('@pid' => $object->id, '@ds' => $datastream),
            'type' => 'dsm',
          ),
        );
      }
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
 * @param bool $service
 *   Whether this is the service URL or the Handle.net URL.
 *
 * @return string
 *   The full Handle Service URL containing the Handle.
 */
function islandora_handle_construct_url(AbstractObject $object, $service) {
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
function islandora_handle_construct_suffix(AbstractObject $object) {
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
  if (variable_get('islandora_handle_debug_mode', FALSE)) {
    $handle = db_select('islandora_handle_debug_handles', 'h')
      ->fields('h')
      ->condition('suffix', $object->id, '=')
      ->execute()
      ->fetchAssoc();
    return $handle;
  }
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

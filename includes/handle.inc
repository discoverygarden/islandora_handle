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
  $suffix = $object->id;
  $url = variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-server');
  $prefix = variable_get('islandora_handle_server_prefix', '1234567');
  $admin_user = variable_get('islandora_handle_server_admin_username', 'handleAdmin');
  $admin_password = variable_get('islandora_handle_server_admin_password', 'superSecretPassword');

  $handle_server_url = "$url/$prefix/$suffix";
  // We do this with language such that we don't get language specific prefixes
  // in the URL.
  $target = url("islandora/object/$suffix", array(
    'language' => (object) array('language' => FALSE),
    'absolute' => TRUE,
  ));
  $query_target = drupal_http_build_query(array('target' => $target));
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);
  $response = drupal_http_request($handle_server_url, array(
    'headers' => array(
      'Authorization' => $authorization_header,
    ),
    'data' => $query_target,
    'method' => 'POST',
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
  $url = variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-service');
  $prefix = variable_get('islandora_handle_server_prefix', '1234567');
  $admin_user = variable_get('islandora_handle_server_admin_username', 'handleAdmin');
  $admin_password = variable_get('islandora_handle_server_admin_password', 'superSecretPassword');
  $handle_server_url = "$url/$prefix/$suffix";
  $authorization_header = 'Basic ' . base64_encode($admin_user . ':' . $admin_password);
  $response = drupal_http_request($handle_server_url, array(
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
 */
function islandora_handle_append_handle(AbstractObject $object, $datastream, $xsl_location) {
  $xslt = new XSLTProcessor();
  $xsl_transform = new DOMDocument();
  $prefix = variable_get('islandora_handle_server_prefix', '1234567');
  $handle_value = "http://hdl.handle.net/$prefix/$object->id";

  $ds_content = $object[$datastream]->content;

  $xsl_transform->load($xsl_location);
  $input = new DomDocument();

  $did_load_ok = $input->loadXML($ds_content);
  if ($did_load_ok) {
    $xslt->importStylesheet($xsl_transform);
    $xslt->setParameter('', 'handle_value', $handle_value);
    $ds_output = $xslt->transformToXml($input);
    if ($ds_output) {
      $object[$datastream]->content = $ds_output;
    }
    else {
      watchdog('islandora_handle', "Appending the Handle value for $object->id failed!", WATCHDOG_ERROR);
    }
  }
  else {
    watchdog('islandora_handle', "Unable to load content of the $datastream datastream for $object->id!", WATCHDOG_ERROR);
  }
}

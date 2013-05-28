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
      watchdog('islandora_handle', "Appending the Handle value for @obj failed!", array('@obj' => $object->id), WATCHDOG_ERROR);
    }
  }
  else {
    watchdog('islandora_handle', "Unable to load content of the @ds datastream for @obj!", array('@ds' => $datastream, '@obj' => $object->id), WATCHDOG_ERROR);
  }
}

/**
 * Appends the Handle value to the DC Identifier field of an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object in Fedora.
 */
function islandora_handle_add_to_dc_datastream(AbstractObject $object) {
  if (isset($object['DC'])) {
    // Delete an existing Handle identifier if it exists and append the new one.
    $dom = new DomDocument();
    $dom->preserveWhiteSpace = FALSE;
    $dom->formatOutput = TRUE;
    $dom->loadXML($object['DC']->content);
    $dom_xpath = new DomXPath($dom);
    $dom_xpath->registerNamespace('oai_dc', 'http://www.openarchives.org/OAI/2.0/oai_dc/');
    $dom_xpath->registerNamespace('dc', 'http://purl.org/dc/elements/1.1/');

    $handle_replaced = FALSE;
    $prefix = variable_get('islandora_handle_server_prefix', '1234567');
    $handle_value = "http://hdl.handle.net/$prefix/$object->id";
    $handle_node = $dom->createElement('dc:identifier', $handle_value);

    // First check if one exists such that we can replace it.
    $identifier_results = $dom_xpath->query('/oai_dc:dc/dc:identifier[starts-with(text(), "http://hdl.handle.net")]');
    foreach ($identifier_results as $node) {
      $parent = $node->parentNode;
      $parent->insertBefore($handle_node, $node);
      $parent->removeChild($node);
      // Should only be one result so let's break just incase.
      $handle_replaced = TRUE;
      break;
    }
    if (!$handle_replaced) {
      $dom->documentElement->appendChild($handle_node);
    }
    $object['DC']->content = $dom->saveXML();
  }
  else {
    watchdog('islandora_handle', "Unable to append Handle to DC as the datastream does not exist for @obj!", array('@obj' => $object->id), WATCHDOG_ERROR);
  }
}

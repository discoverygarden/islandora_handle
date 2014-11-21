<?php
/**
 * @file
 * Constructs derivatives for the Islandora Handle module.
 */

/**
 * Appends a Handle from specified configuration.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 * @param bool $force
 *   Whether we are forcing the derivs or not.
 * @param array $hook
 *   An array describing the hook.
 *
 * @return array
 *   An array describing the success of the operation.
 */
function islandora_handle_append_handles_from_configuration(AbstractObject $object, $force = FALSE, $hook = array()) {
  module_load_include('inc', 'islandora_handle', 'includes/handle');
  $handle_results = islandora_handle_retrieve_configurations_by_cmodels($object->models);
  if (count($handle_results) > 0) {
    $handle_existence = islandora_handle_query_for_handle($object);
    if (!$handle_existence) {
      $response = islandora_handle_construct_handle($object);
      if ($response->code == 201) {
        $append_count = 0;
        $messages = array();
        $success = TRUE;
        foreach ($handle_results as $assoc) {
          if (isset($object[$assoc->datastream])) {
            $add_outcome = islandora_handle_append_handle($object, $assoc->datastream, $assoc->xsl_location);
            if (!$add_outcome['success']) {
              $success = FALSE;
            }
            $messages[] = $add_outcome['message'];
            $append_count++;
          }
        }
        // Edge case that could occur if for some reason the datastream(s)
        // are not present on the object when we go to append. No use having a
        // Handle present when it's not appended anywhere...
        if ($append_count == 0) {
          $success = FALSE;
          $delete_response = islandora_handle_delete_handle($object->id);
          if (!in_array($delete_response->code, array('204', '500'))) {
            $error = $delete_response->error;
            $messages[] = array(
              'message' => t('Unable to delete Handle. Error: @error'),
              'message_sub' => array('@error' => $error),
              'type' => 'watchdog',
              'severity' => WATCHDOG_ERROR,
            );
          }
          $messages[] = array(
            'message' => t('No Handle value was appended for @obj as the associated datastreams do not exist'),
            'message_sub' => array('@obj' => $object->id),
            'type' => 'watchdog',
            'severity' => WATCHDOG_ERROR,
          );
        }
        return array(
          'success' => $success,
          'messages' => $messages,
        );
      }
      else {
        $error = $response->error;
        return array(
          'success' => FALSE,
          'messages' => array(
            array(
              'message' => t('Error constructing Handle for @obj! Error of: @error.'),
              'message_sub' => array('@error' => $error, '@obj' => $object->id),
              'type' => 'watchdog',
              'severity' => WATCHDOG_ERROR,
            ),
          ),
        );
      }
    }
  }
}

/**
 * Appends the Handle value to the DC Identifier field of an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object in Fedora.
 * @param bool $force
 *   Whether the derivatives are being forced or not.
 * @param array $hook
 *   An array describing the hook.
 *
 * @return array
 *   An array describing the outcome of the derivative process.
 */
function islandora_handle_add_to_dc_datastream(AbstractObject $object, $force = FALSE, $hook = array()) {
  module_load_include('inc', 'islandora_handle', 'includes/handle');
  $handle_existence = islandora_handle_query_for_handle($object);
  $dc_existence = isset($object['DC']);
  if ($dc_existence && $handle_existence) {
    // This could trigger many times so let's be sure that the current Handle
    // value doesn't already exist to not create multiple versions.
    $dom = new DomDocument();
    $dom->preserveWhiteSpace = FALSE;
    $dom->formatOutput = TRUE;
    $dom->loadXML($object['DC']->content);
    $dom_xpath = new DomXPath($dom);
    $dom_xpath->registerNamespace('oai_dc', 'http://www.openarchives.org/OAI/2.0/oai_dc/');
    $dom_xpath->registerNamespace('dc', 'http://purl.org/dc/elements/1.1/');

    $update = FALSE;
    $handle_replaced = FALSE;
    $handle_url = islandora_handle_construct_url($object, FALSE);
    $handle_node = $dom->createElement('dc:identifier', $handle_url);

    // First check if one exists such that we can replace it.
    $identifier_results = $dom_xpath->query('/oai_dc:dc/dc:identifier[starts-with(text(), "http://hdl.handle.net")]');
    foreach ($identifier_results as $node) {
      // This could trigger many times so let's be sure that the current Handle
      // value doesn't already exist to not create multiple versions.
      if ($node->nodeValue != $handle_url) {
        // Delete an existing Handle identifier if it exists and append the new
        // one.
        $parent = $node->parentNode;
        $parent->insertBefore($handle_node, $node);
        $parent->removeChild($node);
        // Should only be one result so let's break just incase.
        $handle_replaced = TRUE;
        break;
      }
      else {
        $update = TRUE;
      }
    }
    if (!$handle_replaced) {
      $dom->documentElement->appendChild($handle_node);
    }

    if ($update) {
      $object['DC']->content = $dom->saveXML();
      return array(
        'success' => TRUE,
        'messages' => array(
          array(
            'message' => t('Updated the Handle in the Dublin Core for @pid.'),
            'message_sub' => array('@pid' => $object->id),
            'type' => 'dsm',
          ),
        ),
      );
    }
  }
  else {
    $messages = array();
    if (!$handle_existence) {
      $messages[] = array(
        'message' => t('Unable to append Handle to DC as the Handle was not constructed for @obj!'),
        'message_sub' => array('@obj' => $object->id),
        'type' => 'watchdog',
        'severity' => WATCHDOG_ERROR,
      );
    }
    if (!$dc_existence) {
      $messages[] = array(
        'message' => t('Unable to append Handle to DC as the datastream does not exist for @obj!'),
        'message_sub' => array('@obj' => $object->id),
        'type' => 'watchdog',
        'severity' => WATCHDOG_ERROR,
      );
      watchdog('islandora_handle', "Unable to append Handle to DC as the datastream does not exist for @obj!", array('@obj' => $object->id), WATCHDOG_ERROR);
    }
    return array(
      'success' => FALSE,
      'messages' => $messages,
    );
  }
}

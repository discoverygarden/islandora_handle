<?php

/**
 * Service handler for dealing with legacy Handle 7 servers.
 */
class IslandoraHandleServiceHandler extends IslandoraHandleHandleHandler {

  /**
   * Creates a Handle using the Handle service.
   *
   * @param AbstractObject $object
   *   The AbstractObject having a handle minted for it.
   *
   * @return bool
   *   TRUE on creation; FALSE otherwise.
   */
  public function createHandle(AbstractObject $object) {
    $suffix = $this->constructSuffix($object);
    $full_handle = $this->getFullHandle($object);

    $response = drupal_http_request($this->getServiceUrl($full_handle), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
      ),
      'data' => drupal_http_build_query(array(
        'target' => $this->constructTargetUrl($suffix),
      )),
      'method' => 'POST',
    ));
    if ($response->code == 201) {
      return TRUE;
    }
    else {
      // Watchdog a bit more verboseness.
      watchdog('islandora_handle', 'Unable to construct Handle for @pid. Error of @error.', array('@error' => $response->error, '@pid' => $object->id), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Gets an existing Handle's location from the Handle service.
   *
   * @param string|AbstractObject $handle
   *   A Handle that we are reading in prefix/suffix form or an AbstractObject
   *   that is being read. In the AbstractObject case a Handle URL will be
   *   constructed to compare.
   *
   * @return string|bool
   *   The location of the Handle if it exists; FALSE otherwise.
   */
  public function readHandle($handle) {
    $full_handle = $this->getFullHandle($handle);
    $response = drupal_http_request($this->getServiceUrl($full_handle), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
      ),
      'method' => 'GET',
    ));
    if ($response->code == 204) {
      return urldecode($response->headers['location']);
    }
    else {
      watchdog('islandora_handle', 'Unable to retrieve Handle location for @handle. Error: @error.', array(
        '@handle' => $full_handle,
        '@error' => $response->error,
      ), WATCHDOG_ERROR);
      return FALSE;
    }

  }

  /**
   * Updates a Handle's location using the Handle service.
   *
   * @param string|AbstractObject $handle
   *   A Handle that we are updating in prefix/suffix form or an AbstractObject
   *   that is being updated. In the AbstractObject case a Handle URL will be
   *   constructed to compare.
   * @param string $target
   *   The target URL the Handle is being updated to point to.
   *
   * @return bool
   *   TRUE if successful; FALSE otherwise.
   */
  public function updateHandle($handle, $target) {
    $full_handle = $this->getFullHandle($handle);
    $response = drupal_http_request($this->getServiceUrl($full_handle, $target), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
      ),
      'method' => 'PUT',
    ));
    if (in_array($response->code, array('200', '204'))) {
      return TRUE;
    }
    else {
      watchdog('islandora_handle', 'Unable to update Handle location for @handle. Error: @error.', array(
        '@error' => $response->error,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Deletes a Handle using the Handle service.
   *
   * @param string|AbstractObject $handle
   *   A Handle that we are deleting in prefix/suffix form or an AbstractObject
   *   that is being deleted. In the AbstractObject case a Handle URL will be
   *   constructed to compare.
   *
   * @return bool
   *   TRUE if successful; FALSE otherwise.
   */
  public function deleteHandle($handle) {
    $response = drupal_http_request($this->getServiceUrl($this->getFullHandle($handle)), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
      ),
      'method' => 'DELETE',
    ));
    if (!in_array($response->code, array('204', '500'))) {
      watchdog('islandora_handle', 'Request to delete Handle from the Handle service failed. Error: @error', array(
        '@error' => $response->error,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  /**
   * Helper that returns the service URL.
   *
   * @param string $handle
   *   The handle that is being queried for.
   * @param string $target
   *   The target URL that may be needed to be pointed at for certain requests.
   *
   * @return string
   *   The URL to be used for connecting to the Handle service.
   */
  public function getServiceUrl($handle, $target = NULL) {
    $params = array();
    if (!is_null($target)) {
      $params['query'] = array(
        'target' => $target,
      );
    }
    return url(format_string('!service_url/!handle', array(
      '!service_url' => variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-service'),
      '!handle' => $handle,
    )), $params);
  }

}

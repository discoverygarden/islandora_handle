<?php

/**
 * REST handler for dealing with Handle 8+ servers.
 */
class IslandoraHandleRestHandler extends IslandoraHandleHandleHandler {

  protected $serviceUrl;

  /**
   * {@inheritdoc}
   */
  public function __construct(AbstractObject $object = NULL, $prefix = NULL) {
    parent::__construct($object, $prefix);
    $this->authorizationHeader = format_string('Basic !auth', array(
      '!auth' => base64_encode(format_string(
        '300%3A!prefix/!admin:!password',
        array(
          '!prefix' => $this->prefix,
          '!admin' => variable_get('islandora_handle_server_admin_username', 'handleAdmin'),
          '!password' => variable_get('islandora_handle_server_admin_password', 'superSecretPassword'),
        ))),
    ));
    $this->serviceUrl = variable_get('islandora_handle_server_url', 'http://localhost:8080/handle-service');
  }

  /**
   * Creates a Handle using the Handle REST API.
   *
   * @param AbstractObject $object
   *   The AbstractObject having a handle minted for it.
   *
   * @return bool
   *   TRUE on creation; FALSE otherwise.
   */
  public function createHandle(AbstractObject $object) {
    $response = drupal_http_request($this->buildPutUrl($object), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
        'Content-Type' => 'application/json;charset=UTF-8',
      ),
      'data' => json_encode(array(
        array(
          'index' => 1,
          'type' => 'URL',
          'data' => $this->constructTargetUrl($this->constructSuffix($object)),
        ),
      )),
      'method' => 'PUT',
    ));
    $handle_response_code = $this->mapHandleResponseCodes($response);
    if ($handle_response_code && $handle_response_code['code'] === 1) {
      return TRUE;
    }
    else {
      // Watchdog a bit more verboseness.
      watchdog('islandora_handle', 'Unable to construct Handle for @pid. @type error @code with message @error.', array(
        '@pid' => $object->id,
        '@type' => $handle_response_code ? t('Handle') : t('HTTP'),
        '@code' => $handle_response_code ? $handle_response_code['code'] : $response->code,
        '@error' => $handle_response_code ? $handle_response_code['message'] : $response->error,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Gets an existing Handle's location from the Handle REST API.
   *
   * @param string|AbstractObject $handle
   *   A Handle that we are reading in prefix/suffix form or an AbstractObject
   *   that is being read. In the AbstractObject case a Handle URL will be
   *   constructed to compare.
   * @param bool $index
   *   Whether to also return the index of the URL, used in updating.
   *
   * @return string|bool|array
   *   The location of the Handle; FALSE if it does not exist, or when the index
   *   is requested an array containing:
   *   -index: A integer of the index where the URL value is.
   *   -value: A string containing the location of where the Handle is pointed
   *   at.
   */
  public function readHandle($handle, $index = FALSE) {
    $full_handle = $this->getFullHandle($handle);
    $response = drupal_http_request(format_string('!url/!handle', array(
      '!url' => $this->serviceUrl,
      '!handle' => $full_handle,
    )), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
      ),
      'method' => 'GET',
    ));
    $handle_response_code = $this->mapHandleResponseCodes($response);
    if ($handle_response_code && $handle_response_code['code'] === 1) {
      // Need to extract the URL from the Handle.
      if (isset($response->data)) {
        $data = json_decode($response->data, TRUE);
        if (isset($data['values'])) {
          foreach ($data['values'] as $values) {
            if ($values['type'] === 'URL' && isset($values['data']['value'])) {
              return $index ? array('index' => $values['index'], 'value' => $values['data']['value']) : $values['data']['value'];
            }
          }
        }
      }
    }
    else {
      watchdog('islandora_handle', 'Unable to retrieve Handle location for @handle. @type error @code with message: @error.', array(
        '@handle' => $full_handle,
        '@type' => $handle_response_code ? t('Handle') : t('HTTP'),
        '@code' => $handle_response_code ? $handle_response_code['code'] : $response->code,
        '@error' => $handle_response_code ? $handle_response_code['message'] : $response->error,
      ), WATCHDOG_ERROR);
      return FALSE;
    }

  }

  /**
   * Updates a Handle's location using the Handle REST API.
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
    $location = $this->readHandle($full_handle, TRUE);
    if (!$location) {
      return FALSE;
    }
    $params = array(
      'overwrite' => 'true',
      'index' => $location['index'],
    );
    $response = drupal_http_request($this->buildPutUrl($full_handle, $params), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
        'Content-Type' => 'application/json;charset=UTF-8',
      ),
      'method' => 'PUT',
      'data' => json_encode(array(
        array(
          'index' => $location['index'],
          'type' => 'URL',
          'data' => $target,
        ),
      )),
    ));
    $handle_response_code = $this->mapHandleResponseCodes($response);
    if ($handle_response_code && $handle_response_code['code'] === 1) {
      return TRUE;
    }
    else {
      watchdog('islandora_handle', 'Unable to update Handle location for @handle. @type error @code with message: @error.', array(
        '@handle' => $full_handle,
        '@type' => $handle_response_code ? t('Handle') : t('HTTP'),
        '@code' => $handle_response_code ? $handle_response_code['code'] : $response->code,
        '@error' => $handle_response_code ? $handle_response_code['message'] : $response->error,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Deletes a Handle using the Handle REST API.
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
    $full_handle = $this->getFullHandle($handle);
    $response = drupal_http_request(format_string('!url/!handle', array(
      '!url' => $this->serviceUrl,
      '!handle' => $full_handle,
    )), array(
      'headers' => array(
        'Authorization' => $this->authorizationHeader,
      ),
      'method' => 'DELETE',
    ));
    $handle_response_code = $this->mapHandleResponseCodes($response);
    if ($handle_response_code && $handle_response_code['code'] === 1) {
      return TRUE;
    }
    else {
      watchdog('islandora_handle', 'Unable to delete Handle for @handle. @type error @code with message: @error.', array(
        '@handle' => $full_handle,
        '@type' => $handle_response_code ? t('Handle') : t('HTTP'),
        '@code' => $handle_response_code ? $handle_response_code['code'] : $response->code,
        '@error' => $handle_response_code ? $handle_response_code['message'] : $response->error,
      ), WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Helper to build a URL for PUT requests (Create and Update).
   *
   * @param string|AbstractObject $handle
   *   A Handle that we are deleting in prefix/suffix form or an AbstractObject
   *   that is being deleted. In the AbstractObject case a Handle URL will be
   *   constructed to compare.
   * @param array $params
   *   An array of query parameters to be used for the Handle URL. Will contain:
   *   -overwrite: A string either 'true' or 'false' if the Handle or index
   *   should be overwritten if it exists.
   *   -index: An integer denoting the index to update, in our case the URL.
   *
   * @return string
   *   The URL to be used for the request.
   */
  protected function buildPutUrl($handle, array $params = array()) {
    $params += array(
      'overwrite' => 'false',
    );
    return url(format_string('!service_url/!full_handle', array(
      '!service_url' => $this->serviceUrl,
      '!full_handle' => $this->getFullHandle($handle),
    )), array(
      'query' => $params,
    ));
  }

  /**
   * Maps Handle's response codes to error messages if they exist.
   *
   * @param object $response
   *   The response of the HTTP request to the Handle REST API.
   *
   * @return bool|array
   *   FALSE if no data or the code does not exist in our mapping, otherwise an
   *   array containing:
   *   -code: An integer of the Handle response code.
   *   -message: A string describing what that message actually means.
   */
  protected function mapHandleResponseCodes($response) {
    $mapping = array(
      '1' => t('Success'),
      '2' => t('An unexpected error on the server has occurred'),
      '100' => t('Handle not found'),
      '101' => t('Handle already exists'),
      '102' => t('Invalid handle'),
      '200' => t('Values not found'),
      '201' => t('Value already exists'),
      '202' => t('Invalid value'),
      '301' => t('Server not response for handle'),
      '402' => t('Authentication needed'),
    );
    if (isset($response->data)) {
      $data = json_decode($response->data, TRUE);
      if (isset($data['responseCode']) && isset($mapping[$data['responseCode']])) {
        return array(
          'code' => $data['responseCode'],
          'message' => $mapping[$data['responseCode']],
        );
      }
    }
    return FALSE;
  }

}

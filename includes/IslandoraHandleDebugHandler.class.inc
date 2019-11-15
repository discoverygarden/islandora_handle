<?php

/**
 * Debug handler for when there's no Handle server to interact with.
 */
class IslandoraHandleDebugHandler extends IslandoraHandleHandleHandler {

  /**
   * Creates a Handle in the database for debugging.
   *
   * @param AbstractObject $object
   *   The AbstractObject having a handle minted for it.
   *
   * @return bool
   *   TRUE on creation; FALSE otherwise.
   */
  public function createHandle(AbstractObject $object) {
    db_insert('islandora_handle_debug_handles')
      ->fields(array(
        'handle' => $this->getFullHandle($object),
        'location' => $this->constructTargetUrl($object),
      ))
      ->execute();
    return TRUE;
  }

  /**
   * Gets an existing Handle's location from the database for debugging.
   *
   * @param string|AbstractObject $handle
   *   A Handle that we are updating in prefix/suffix form or an AbstractObject
   *   that is being updated. In the AbstractObject case a Handle URL will be
   *   constructed to compare.
   *
   * @return string|bool
   *   The location of the Handle if it exists; FALSE otherwise.
   */
  public function readHandle($handle) {
    $full_handle = $this->getFullHandle($handle);
    return db_select('islandora_handle_debug_handles', 'd')
      ->fields('d', array('location'))
      ->condition('handle', $full_handle, '=')
      ->execute()
      ->fetchField();
  }

  /**
   * Updates a Handle's location in the database for debugging.
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
    db_merge('islandora_handle_debug_handles')
      ->fields(array(
        'location' => $target,
        'handle' => $full_handle,
      ))
      ->condition('handle', $full_handle)
      ->execute();
    return TRUE;
  }

  /**
   * Deletes a Handle from the database for debugging.
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
    db_delete('islandora_handle_debug_handles')
      ->condition('handle', $full_handle)
      ->execute();
    return TRUE;
  }

}

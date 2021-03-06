<?php

/**
 * @file
 * Contains \Drupal\comment\FieldNewValue.
 */

namespace Drupal\comment;

use Drupal\Core\TypedData\ContextAwareTypedData;
use Drupal\Core\TypedData\ReadOnlyException;
use InvalidArgumentException;

/**
 * A computed property for the integer value of the 'new' field.
 *
 * @todo: Declare the list of allowed values once supported.
 */
class FieldNewValue extends ContextAwareTypedData {

  /**
   * Implements \Drupal\Core\TypedData\TypedDataInterface::getValue().
   */
  public function getValue($langcode = NULL) {
    if (!isset($this->value)) {
      if (!isset($this->parent)) {
        throw new InvalidArgumentException('Computed properties require context for computation.');
      }
      $field = $this->parent->getParent();
      $entity = $field->getParent();
      $this->value = node_mark($entity->nid->target_id, $entity->changed->value);
    }
    return $this->value;
  }

  /**
   * Implements \Drupal\Core\TypedData\TypedDataInterface::setValue().
   */
  public function setValue($value) {
    if (isset($value)) {
      throw new ReadOnlyException('Unable to set a computed property.');
    }
  }
}

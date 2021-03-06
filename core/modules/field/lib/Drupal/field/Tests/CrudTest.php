<?php

/**
 * @file
 * Definition of Drupal\field\Tests\CrudTest.
 */

namespace Drupal\field\Tests;

use Drupal\field\FieldException;
use Exception;

class CrudTest extends FieldUnitTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('number');

  public static function getInfo() {
    return array(
      'name' => 'Field CRUD tests',
      'description' => 'Test field create, read, update, and delete.',
      'group' => 'Field API',
    );
  }

  // TODO : test creation with
  // - a full fledged $field structure, check that all the values are there
  // - a minimal $field structure, check all default values are set
  // defer actual $field comparison to a helper function, used for the two cases above

  /**
   * Test the creation of a field.
   */
  function testCreateField() {
    $field_definition = array(
      'field_name' => 'field_2',
      'type' => 'test_field',
    );
    field_test_memorize();
    $field = field_create_field($field_definition);
    $mem = field_test_memorize();
    $this->assertIdentical($mem['field_test_field_create_field'][0][0]['field_name'], $field_definition['field_name'], 'hook_field_create_field() called with correct arguments.');
    $this->assertIdentical($mem['field_test_field_create_field'][0][0]['type'], $field_definition['type'], 'hook_field_create_field() called with correct arguments.');

    // Read the configuration. Check against raw configuration data rather than
    // the loaded ConfigEntity, to be sure we check that the defaults are
    // applied on write.
    $field_config = \Drupal::config('field.field.' . $field->id())->get();

    // Ensure that basic properties are preserved.
    $this->assertEqual($field_config['id'], $field_definition['field_name'], 'The field name is properly saved.');
    $this->assertEqual($field_config['type'], $field_definition['type'], 'The field type is properly saved.');

    // Ensure that cardinality defaults to 1.
    $this->assertEqual($field_config['cardinality'], 1, 'Cardinality defaults to 1.');

    // Ensure that default settings are present.
    $field_type = field_info_field_types($field_definition['type']);
    $this->assertEqual($field_config['settings'], $field_type['settings'], 'Default field settings have been written.');

    // Ensure that default storage was set.
    $this->assertEqual($field_config['storage']['type'], config('field.settings')->get('default_storage'), 'The field type is properly saved.');

    // Guarantee that the name is unique.
    try {
      field_create_field($field_definition);
      $this->fail(t('Cannot create two fields with the same name.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot create two fields with the same name.'));
    }

    // Check that field type is required.
    try {
      $field_definition = array(
        'field_name' => 'field_1',
      );
      field_create_field($field_definition);
      $this->fail(t('Cannot create a field with no type.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot create a field with no type.'));
    }

    // Check that field name is required.
    try {
      $field_definition = array(
        'type' => 'test_field'
      );
      field_create_field($field_definition);
      $this->fail(t('Cannot create an unnamed field.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot create an unnamed field.'));
    }

    // Check that field name must start with a letter or _.
    try {
      $field_definition = array(
        'field_name' => '2field_2',
        'type' => 'test_field',
      );
      field_create_field($field_definition);
      $this->fail(t('Cannot create a field with a name starting with a digit.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot create a field with a name starting with a digit.'));
    }

    // Check that field name must only contain lowercase alphanumeric or _.
    try {
      $field_definition = array(
        'field_name' => 'field#_3',
        'type' => 'test_field',
      );
      field_create_field($field_definition);
      $this->fail(t('Cannot create a field with a name containing an illegal character.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot create a field with a name containing an illegal character.'));
    }

    // Check that field name cannot be longer than 32 characters long.
    try {
      $field_definition = array(
        'field_name' => '_12345678901234567890123456789012',
        'type' => 'test_field',
      );
      field_create_field($field_definition);
      $this->fail(t('Cannot create a field with a name longer than 32 characters.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot create a field with a name longer than 32 characters.'));
    }

    // Check that field name can not be an entity key.
    // "ftvid" is known as an entity key from the "test_entity" type.
    try {
      $field_definition = array(
        'type' => 'test_field',
        'field_name' => 'ftvid',
      );
      field_create_field($field_definition);
      $this->fail(t('Cannot create a field bearing the name of an entity key.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot create a field bearing the name of an entity key.'));
    }
  }

  /**
   * Test failure to create a field.
   */
  function testCreateFieldFail() {
    $field_name = 'duplicate';
    $field_definition = array('field_name' => $field_name, 'type' => 'test_field', 'storage' => array('type' => 'field_test_storage_failure'));
    $field = entity_load('field_entity', $field_name);

    // The field does not exist.
    $this->assertFalse($field, 'The field does not exist.');

    // Try to create the field.
    try {
      $field = field_create_field($field_definition);
      $this->assertTrue(FALSE, 'Field creation (correctly) fails.');
    }
    catch (Exception $e) {
      $this->assertTrue(TRUE, 'Field creation (correctly) fails.');
    }

    // The field does not exist.
    $field = entity_load('field_entity', $field_name);
    $this->assertFalse($field, 'The field does not exist.');
  }

  /**
   * Tests reading a single field definition.
   */
  function testReadField() {
    $field_definition = array(
      'field_name' => 'field_1',
      'type' => 'test_field',
    );
    field_create_field($field_definition);

    // Read the field back.
    $field = field_read_field($field_definition['field_name']);
    $this->assertTrue($field_definition < $field, 'The field was properly read.');
  }

  /**
   * Tests reading field definitions.
   */
  function testReadFields() {
    $field_definition = array(
      'field_name' => 'field_1',
      'type' => 'test_field',
    );
    field_create_field($field_definition);

    // Check that 'single column' criteria works.
    $fields = field_read_fields(array('field_name' => $field_definition['field_name']));
    $this->assertTrue(count($fields) == 1 && isset($fields[$field_definition['field_name']]), 'The field was properly read.');

    // Check that 'multi column' criteria works.
    $fields = field_read_fields(array('field_name' => $field_definition['field_name'], 'type' => $field_definition['type']));
    $this->assertTrue(count($fields) == 1 && isset($fields[$field_definition['field_name']]), 'The field was properly read.');
    $fields = field_read_fields(array('field_name' => $field_definition['field_name'], 'type' => 'foo'));
    $this->assertTrue(empty($fields), 'No field was found.');

    // Create an instance of the field.
    $instance_definition = array(
      'field_name' => $field_definition['field_name'],
      'entity_type' => 'test_entity',
      'bundle' => 'test_bundle',
    );
    field_create_instance($instance_definition);
  }

  /**
   * Test creation of indexes on data column.
   */
  function testFieldIndexes() {
    // Check that indexes specified by the field type are used by default.
    $field_definition = array(
      'field_name' => 'field_1',
      'type' => 'test_field',
    );
    field_create_field($field_definition);
    $field = field_read_field($field_definition['field_name']);
    $schema = $field->getSchema();
    $expected_indexes = array('value' => array('value'));
    $this->assertEqual($schema['indexes'], $expected_indexes, 'Field type indexes saved by default');

    // Check that indexes specified by the field definition override the field
    // type indexes.
    $field_definition = array(
      'field_name' => 'field_2',
      'type' => 'test_field',
      'indexes' => array(
        'value' => array(),
      ),
    );
    field_create_field($field_definition);
    $field = field_read_field($field_definition['field_name']);
    $schema = $field->getSchema();
    $expected_indexes = array('value' => array());
    $this->assertEqual($schema['indexes'], $expected_indexes, 'Field definition indexes override field type indexes');

    // Check that indexes specified by the field definition add to the field
    // type indexes.
    $field_definition = array(
      'field_name' => 'field_3',
      'type' => 'test_field',
      'indexes' => array(
        'value_2' => array('value'),
      ),
    );
    field_create_field($field_definition);
    $field = field_read_field($field_definition['field_name']);
    $schema = $field->getSchema();
    $expected_indexes = array('value' => array('value'), 'value_2' => array('value'));
    $this->assertEqual($schema['indexes'], $expected_indexes, 'Field definition indexes are merged with field type indexes');
  }

  /**
   * Test the deletion of a field.
   */
  function testDeleteField() {
    // TODO: Also test deletion of the data stored in the field ?

    // Create two fields (so we can test that only one is deleted).
    $this->field = array('field_name' => 'field_1', 'type' => 'test_field');
    field_create_field($this->field);
    $this->another_field = array('field_name' => 'field_2', 'type' => 'test_field');
    field_create_field($this->another_field);

    // Create instances for each.
    $this->instance_definition = array(
      'field_name' => $this->field['field_name'],
      'entity_type' => 'test_entity',
      'bundle' => 'test_bundle',
      'widget' => array(
        'type' => 'test_field_widget',
      ),
    );
    field_create_instance($this->instance_definition);
    $this->another_instance_definition = $this->instance_definition;
    $this->another_instance_definition['field_name'] = $this->another_field['field_name'];
    field_create_instance($this->another_instance_definition);

    // Test that the first field is not deleted, and then delete it.
    $field = field_read_field($this->field['field_name'], array('include_deleted' => TRUE));
    $this->assertTrue(!empty($field) && empty($field['deleted']), 'A new field is not marked for deletion.');
    field_delete_field($this->field['field_name']);

    // Make sure that the field is marked as deleted when it is specifically
    // loaded.
    $field = field_read_field($this->field['field_name'], array('include_deleted' => TRUE));
    $this->assertTrue(!empty($field['deleted']), 'A deleted field is marked for deletion.');

    // Make sure that this field's instance is marked as deleted when it is
    // specifically loaded.
    $instance = field_read_instance('test_entity', $this->instance_definition['field_name'], $this->instance_definition['bundle'], array('include_deleted' => TRUE));
    $this->assertTrue(!empty($instance['deleted']), 'An instance for a deleted field is marked for deletion.');

    // Try to load the field normally and make sure it does not show up.
    $field = field_read_field($this->field['field_name']);
    $this->assertTrue(empty($field), 'A deleted field is not loaded by default.');

    // Try to load the instance normally and make sure it does not show up.
    $instance = field_read_instance('test_entity', $this->instance_definition['field_name'], $this->instance_definition['bundle']);
    $this->assertTrue(empty($instance), 'An instance for a deleted field is not loaded by default.');

    // Make sure the other field (and its field instance) are not deleted.
    $another_field = field_read_field($this->another_field['field_name']);
    $this->assertTrue(!empty($another_field) && empty($another_field['deleted']), 'A non-deleted field is not marked for deletion.');
    $another_instance = field_read_instance('test_entity', $this->another_instance_definition['field_name'], $this->another_instance_definition['bundle']);
    $this->assertTrue(!empty($another_instance) && empty($another_instance['deleted']), 'An instance of a non-deleted field is not marked for deletion.');

    // Try to create a new field the same name as a deleted field and
    // write data into it.
    field_create_field($this->field);
    field_create_instance($this->instance_definition);
    $field = field_read_field($this->field['field_name']);
    $this->assertTrue(!empty($field) && empty($field['deleted']), 'A new field with a previously used name is created.');
    $instance = field_read_instance('test_entity', $this->instance_definition['field_name'], $this->instance_definition['bundle']);
    $this->assertTrue(!empty($instance) && empty($instance['deleted']), 'A new instance for a previously used field name is created.');

    // Save an entity with data for the field
    $entity = field_test_create_entity(0, 0, $instance['bundle']);
    $langcode = LANGUAGE_NOT_SPECIFIED;
    $values[0]['value'] = mt_rand(1, 127);
    $entity->{$field['field_name']}[$langcode] = $values;
    $entity_type = 'test_entity';
    field_attach_insert($entity);

    // Verify the field is present on load
    $entity = field_test_create_entity(0, 0, $this->instance_definition['bundle']);
    field_attach_load($entity_type, array(0 => $entity));
    $this->assertIdentical(count($entity->{$field['field_name']}[$langcode]), count($values), "Data in previously deleted field saves and loads correctly");
    foreach ($values as $delta => $value) {
      $this->assertEqual($entity->{$field['field_name']}[$langcode][$delta]['value'], $values[$delta]['value'], "Data in previously deleted field saves and loads correctly");
    }
  }

  function testUpdateFieldType() {
    $field = array('field_name' => 'field_type', 'type' => 'number_decimal');
    $field = field_create_field($field);

    $test_field = array('field_name' => 'field_type', 'type' => 'number_integer');
    try {
      field_update_field($test_field);
      $this->fail(t('Cannot update a field to a different type.'));
    }
    catch (FieldException $e) {
      $this->pass(t('Cannot update a field to a different type.'));
    }
  }

  /**
   * Test updating a field.
   */
  function testUpdateField() {
    // Create a field with a defined cardinality, so that we can ensure it's
    // respected. Since cardinality enforcement is consistent across database
    // systems, it makes a good test case.
    $cardinality = 4;
    $field = field_create_field(array(
      'field_name' => 'field_update',
      'type' => 'test_field',
      'cardinality' => $cardinality,
    ));
    $instance = field_create_instance(array(
      'field_name' => 'field_update',
      'entity_type' => 'test_entity',
      'bundle' => 'test_bundle',
    ));

    do {
      // We need a unique ID for our entity. $cardinality will do.
      $id = $cardinality;
      $entity = field_test_create_entity($id, $id, $instance['bundle']);
      // Fill in the entity with more values than $cardinality.
      for ($i = 0; $i < 20; $i++) {
        $entity->field_update[LANGUAGE_NOT_SPECIFIED][$i]['value'] = $i;
      }
      // Save the entity.
      field_attach_insert($entity);
      // Load back and assert there are $cardinality number of values.
      $entity = field_test_create_entity($id, $id, $instance['bundle']);
      field_attach_load('test_entity', array($id => $entity));
      $this->assertEqual(count($entity->field_update[LANGUAGE_NOT_SPECIFIED]), $field['cardinality'], 'Cardinality is kept');
      // Now check the values themselves.
      for ($delta = 0; $delta < $cardinality; $delta++) {
        $this->assertEqual($entity->field_update[LANGUAGE_NOT_SPECIFIED][$delta]['value'], $delta, 'Value is kept');
      }
      // Increase $cardinality and set the field cardinality to the new value.
      $field['cardinality'] = ++$cardinality;
      field_update_field($field);
    } while ($cardinality < 6);
  }

  /**
   * Test field type modules forbidding an update.
   */
  function testUpdateFieldForbid() {
    $field = array('field_name' => 'forbidden', 'type' => 'test_field', 'settings' => array('changeable' => 0, 'unchangeable' => 0));
    $field = field_create_field($field);
    $field['settings']['changeable']++;
    try {
      field_update_field($field);
      $this->pass(t("A changeable setting can be updated."));
    }
    catch (FieldException $e) {
      $this->fail(t("An unchangeable setting cannot be updated."));
    }
    $field['settings']['unchangeable']++;
    try {
      field_update_field($field);
      $this->fail(t("An unchangeable setting can be updated."));
    }
    catch (FieldException $e) {
      $this->pass(t("An unchangeable setting cannot be updated."));
    }
  }

}

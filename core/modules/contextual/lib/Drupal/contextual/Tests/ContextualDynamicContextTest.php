<?php

/**
 * @file
 * Definition of Drupal\contextual\Tests\ContextualDynamicContextTest.
 */

namespace Drupal\contextual\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Tests accessible links after inaccessible links on dynamic context.
 */
class ContextualDynamicContextTest extends WebTestBase {

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('contextual', 'node', 'views');

  public static function getInfo() {
    return array(
      'name' => 'Contextual links on node lists',
      'description' => 'Tests if contextual links are showing on the front page depending on permissions.',
      'group' => 'Contextual',
    );
  }

  function setUp() {
    parent::setUp();
    $this->drupalCreateContentType(array('type' => 'page', 'name' => 'Basic page'));
    $this->drupalCreateContentType(array('type' => 'article', 'name' => 'Article'));
    $web_user = $this->drupalCreateUser(array('access content', 'access contextual links', 'edit any article content'));
    $this->drupalLogin($web_user);
  }

  /**
   * Tests contextual links on node lists with different permissions.
   */
  function testNodeLinks() {
    // Create three nodes in the following order:
    // - An article, which should be user-editable.
    // - A page, which should not be user-editable.
    // - A second article, which should also be user-editable.
    $node1 = $this->drupalCreateNode(array('type' => 'article', 'promote' => 1));
    $node2 = $this->drupalCreateNode(array('type' => 'page', 'promote' => 1));
    $node3 = $this->drupalCreateNode(array('type' => 'article', 'promote' => 1));

    // Now, on the front page, all article nodes should have contextual edit
    // links. The page node in between should not.
    $this->drupalGet('node');
    $this->assertRaw('node/' . $node1->nid . '/edit', 'Edit link for oldest article node showing.');
    $this->assertNoRaw('node/' . $node2->nid . '/edit', 'No edit link for page nodes.');
    $this->assertRaw('node/' . $node3->nid . '/edit', 'Edit link for most recent article node showing.');
  }
}

<?php

/**
 * @file
 * User page callbacks for the Aggregator module.
 */

use Drupal\aggregator\Plugin\Core\Entity\Feed;
use Drupal\Core\Entity\EntityInterface;

/**
 * Page callback: Displays the most recent items gathered from any feed.
 *
 * @return string
 *   The rendered list of items for the feed.
 *
 * @see aggregator_menu()
 */
function aggregator_page_last() {
  drupal_add_feed('aggregator/rss', config('system.site')->get('name') . ' ' . t('aggregator'));

  $items = aggregator_load_feed_items('sum');

  return _aggregator_page_list($items, arg(1));
}

/**
 * Page callback: Displays all the items captured from the particular feed.
 *
 * @param \Drupal\aggregator\Plugin\Core\Entity\Feed $feed
 *   The feed for which to display all items.
 *
 * @return string
 *   The rendered list of items for the feed.
 *
 * @see aggregator_menu()
 */
function aggregator_page_source(Feed $feed) {
  $feed_source = entity_view($feed, 'default');

  // It is safe to include the fid in the query because it's loaded from the
  // database by aggregator_feed_load().
  $items = aggregator_load_feed_items('source', $feed);

  return _aggregator_page_list($items, arg(3), $feed_source);
}

/**
 * Form constructor to show all items captured from a feed.
 *
 * @param $feed
 *   The feed for which to list all of the aggregated items.
 *
 * @return string
 *   The rendered list of items for the feed.
 *
 * @see aggregator_menu()
 * @see aggregator_page_source()
 * @ingroup forms
 */
function aggregator_page_source_form($form, $form_state, $feed) {
  return aggregator_page_source($feed);
}

/**
 * Form constructor to list items aggregated in a category.
 *
 * @param $category
 *   The category for which to list all of the aggregated items.
 *
 * @return string
 *   The rendered list of items for the feed.
 *
 * @see aggregator_menu()
 * @ingroup forms
 */
function aggregator_page_category($category) {
  drupal_add_feed('aggregator/rss/' . $category->cid, config('system.site')->get('name') . ' ' . t('aggregator - @title', array('@title' => $category->title)));

  // It is safe to include the cid in the query because it's loaded from the
  // database by aggregator_category_load().
  $items = aggregator_load_feed_items('category', $category);

  return _aggregator_page_list($items, arg(3));
}

/**
 * Form constructor to list items aggregated in a category.
 *
 * @param $category
 *   The category for which to list all of the aggregated items.
 *
 * @return string
 *   The rendered list of items for the feed.
 *
 * @see aggregator_menu()
 * @see aggregator_page_category()
 * @ingroup forms
 */
function aggregator_page_category_form($form, $form_state, $category) {
  return aggregator_page_category($category);
}

/**
 * Loads and optionally filters feed items.
 *
 * @param $type
 *   The type of filter for the items. Possible values are:
 *   - sum: No filtering.
 *   - source: Filter the feed items, limiting the result to items from a
 *     single source.
 *   - category: Filter the feed items by category.
 * @param $data
 *   Feed or category data used for filtering. The type and value of $data
 *   depends on $type:
 *   - source: $data is an object with $data->fid identifying the feed used to
 *     as filter.
 *   - category: $data is an array with $data['cid'] being the category id to
 *     filter on.
 *   The $data parameter is not used when $type is 'sum'.
 *
 * @return
 *   An array of the feed items.
 */
function aggregator_load_feed_items($type, $data = NULL, $limit = 20) {
  $items = array();
  switch ($type) {
    case 'sum':
      $query = db_select('aggregator_item', 'i');
      $query->join('aggregator_feed', 'f', 'i.fid = f.fid');
      $query->fields('i', array('iid'));
      break;
    case 'source':
      $query = db_select('aggregator_item', 'i');
      $query
        ->fields('i', array('iid'))
        ->condition('i.fid', $data->id());
      break;
    case 'category':
      $query = db_select('aggregator_category_item', 'c');
      $query->leftJoin('aggregator_item', 'i', 'c.iid = i.iid');
      $query->leftJoin('aggregator_feed', 'f', 'i.fid = f.fid');
      $query
        ->fields('i', array('iid'))
        ->condition('cid', $data->cid);
      break;
  }

  $result = $query
    ->extend('Drupal\Core\Database\Query\PagerSelectExtender')
    ->limit($limit)
    ->orderBy('i.timestamp', 'DESC')
    ->orderBy('i.iid', 'DESC')
    ->execute();

  $items = entity_load_multiple('aggregator_item', $result->fetchCol());
  return $items;
}

/**
 * Prints an aggregator page listing a number of feed items.
 *
 * Various menu callbacks use this function to print their feeds.
 *
 * @param $items
 *   The items to be listed.
 * @param $op
 *   Which form should be added to the items. Only 'categorize' is now
 *   recognized.
 * @param $feed_source
 *   The feed source URL.
 *
 * @return
 *   The rendered list of items for the feed.
 */
function _aggregator_page_list($items, $op, $feed_source = '') {
  if (user_access('administer news feeds') && ($op == 'categorize')) {
    // Get form data.
    $build = aggregator_categorize_items($items, $feed_source);
  }
  else {
    // Assemble output.
    $build = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('aggregator-wrapper')),
    );
    $build['feed_source'] = is_array($feed_source) ? $feed_source : array('#markup' => $feed_source);
    if ($items) {
      $build['items'] = entity_view_multiple($items, 'default');
      $build['pager']['#markup'] = theme('pager');
    }
  }

  return $build;
}

/**
 * Form constructor to build the page list form.
 *
 * @param array $items
 *   An array of the feed items.
 * @param string $feed_source
 *   (optional) The feed source URL. Defaults to an empty string.
 *
 * @return array
 *   An array of FAPI elements.
 *
 * @see aggregator_categorize_items_submit()
 * @see theme_aggregator_categorize_items()
 * @ingroup forms
 */
function aggregator_categorize_items($items, $feed_source = '') {
  $form['#submit'][] = 'aggregator_categorize_items_submit';
  $form['feed_source'] = array(
    '#value' => $feed_source,
  );
  $categories = array();
  $done = FALSE;

  $form['items'] = array(
    '#type' => 'table',
    '#header' => array('', t('Categorize')),
  );
  if ($items && $form_items = entity_view_multiple($items, 'default')) {
    foreach (element_children($form_items) as $iid) {
      $categories_result = db_query('SELECT c.cid, c.title, ci.iid FROM {aggregator_category} c LEFT JOIN {aggregator_category_item} ci ON c.cid = ci.cid AND ci.iid = :iid', array(':iid' => $iid));
      $selected = array();
      foreach ($categories_result as $category) {
        if (!$done) {
          $categories[$category->cid] = check_plain($category->title);
        }
        if ($category->iid) {
          $selected[] = $category->cid;
        }
      }
      $done = TRUE;
      $form['items'][$iid]['item'] = $form_items[$iid];
      $form['items'][$iid]['categories'] = array(
        '#type' => config('aggregator.settings')->get('source.category_selector'),
        '#default_value' => $selected,
        '#options' => $categories,
        '#size' => 10,
        '#multiple' => TRUE,
        '#parents' => array('categories', $iid),
      );
    }
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save categories'));
  $form['pager'] = array('#theme' => 'pager');

  return $form;
}

/**
 * Form submission handler for aggregator_categorize_items().
 */
function aggregator_categorize_items_submit($form, &$form_state) {
  if (!empty($form_state['values']['categories'])) {
    foreach ($form_state['values']['categories'] as $iid => $selection) {
      db_delete('aggregator_category_item')
        ->condition('iid', $iid)
        ->execute();
      $insert = db_insert('aggregator_category_item')->fields(array('iid', 'cid'));
      $has_values = FALSE;
      foreach ($selection as $cid) {
        if ($cid && $iid) {
          $has_values = TRUE;
          $insert->values(array(
            'iid' => $iid,
            'cid' => $cid,
          ));
        }
      }
      if ($has_values) {
        $insert->execute();
      }
    }
  }
  drupal_set_message(t('The categories have been saved.'));
}

/**
 * Default theme implementation to present a linked feed item for summaries.
 *
 * @param $variables
 *   An associative array containing:
 *   - item_link: Link to item.
 *   - item_age: Age of the item.
 *
 * @see template_preprocess()
 * @see template_preprocess_aggregator_summary_item()
 */
function theme_aggregator_summary_item($variables) {
  return $variables['item_url'] . ' ' . $variables['item_age'];
}

/**
 * Processes variables for aggregator-item.tpl.php.
 *
 * @see aggregator-item.tpl.php
 */
function template_preprocess_aggregator_item(&$variables) {
  $item = $variables['aggregator_item'];

  $variables['feed_url'] = check_url($item->link->value);
  $variables['feed_title'] = check_plain($item->title->value);
  $variables['content'] = aggregator_filter_xss($item->description->value);

  $variables['source_url'] = '';
  $variables['source_title'] = '';
  if (isset($item->ftitle) && isset($item->fid->value)) {
    $variables['source_url'] = url("aggregator/sources/$item->fid->value");
    $variables['source_title'] = check_plain($item->ftitle);
  }
  if (date('Ymd', $item->timestamp->value) == date('Ymd')) {
    $variables['source_date'] = t('%ago ago', array('%ago' => format_interval(REQUEST_TIME - $item->timestamp->value)));
  }
  else {
    $variables['source_date'] = format_date($item->timestamp->value, 'medium');
  }

  $variables['categories'] = array();
  foreach ($item->categories as $category) {
    $variables['categories'][$category->cid] = l($category->title, 'aggregator/categories/' . $category->cid);
  }
}

/**
 * Page callback: Displays all the feeds used by the Aggregator module.
 *
 * @return string
 *   An HTML-formatted string.
 *
 * @see aggregator_menu()
 */
function aggregator_page_sources() {
  $feeds = entity_load_multiple('aggregator_feed');

  $build = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('aggregator-wrapper')),
    '#sorted' => TRUE,
  );
  foreach ($feeds as $feed) {
    // Most recent items:
    $summary_items = array();
    $aggregator_summary_items = config('aggregator.settings')->get('source.list_max');
    if ($aggregator_summary_items) {
      if ($items = aggregator_load_feed_items('source', $feed, $aggregator_summary_items)) {
        $summary_items = entity_view_multiple($items, 'summary');
      }
    }
    $feed->url = url('aggregator/sources/' . $feed->id());
    $build[$feed->id()] = array(
      '#theme' => 'aggregator_summary_items',
      '#summary_items' => $summary_items,
      '#source' => $feed,
    );
  }
  $build['feed_icon'] = array(
    '#theme' => 'feed_icon',
    '#url' => 'aggregator/opml',
    '#title' => t('OPML feed'),
  );
  return $build;
}

/**
 * Page callback: Displays all the categories used by the Aggregator module.
 *
 * @return string
 *   An HTML formatted string.
 *
 * @see aggregator_menu()
 */
function aggregator_page_categories() {
  $result = db_query('SELECT c.cid, c.title, c.description FROM {aggregator_category} c LEFT JOIN {aggregator_category_item} ci ON c.cid = ci.cid LEFT JOIN {aggregator_item} i ON ci.iid = i.iid GROUP BY c.cid, c.title, c.description');

  $build = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('aggregator-wrapper')),
    '#sorted' => TRUE,
  );
  $aggregator_summary_items = config('aggregator.settings')->get('source.list_max');
  foreach ($result as $category) {
    $summary_items = array();
    if ($aggregator_summary_items) {
      if ($items = aggregator_load_feed_items('category', $category, $aggregator_summary_items)) {
        $summary_items = entity_view_multiple($items, 'summary');
      }
    }
    $category->url = url('aggregator/categories/' . $category->cid);
    $build[$category->cid] = array(
      '#theme' => 'aggregator_summary_items',
      '#summary_items' => $summary_items,
      '#source' => $category,
    );
  }

  return $build;
}

/**
 * Page callback: Generates an RSS 0.92 feed of aggregator items or categories.
 *
 * @return string
 *   An HTML formatted string.
 *
 * @see aggregator_menu()
 */
function aggregator_page_rss() {
  $result = NULL;
  $rss_config = config('system.rss');
  // arg(2) is the passed cid, only select for that category.
  if (arg(2)) {
    $category = db_query('SELECT cid, title FROM {aggregator_category} WHERE cid = :cid', array(':cid' => arg(2)))->fetchObject();
    $result = db_query_range('SELECT i.*, f.title AS ftitle, f.link AS flink FROM {aggregator_category_item} c LEFT JOIN {aggregator_item} i ON c.iid = i.iid LEFT JOIN {aggregator_feed} f ON i.fid = f.fid WHERE cid = :cid ORDER BY timestamp DESC, i.iid DESC', 0, $rss_config->get('items.limit'), array(':cid' => $category->cid));
  }
  // Or, get the default aggregator items.
  else {
    $category = NULL;
    $result = db_query_range('SELECT i.*, f.title AS ftitle, f.link AS flink FROM {aggregator_item} i INNER JOIN {aggregator_feed} f ON i.fid = f.fid ORDER BY i.timestamp DESC, i.iid DESC', 0, $rss_config->get('items.limit'));
  }

  $feeds = $result->fetchAll();
  return theme('aggregator_page_rss', array('feeds' => $feeds, 'category' => $category));
}

/**
 * Prints the RSS page for a feed.
 *
 * @param $variables
 *   An associative array containing:
 *   - feeds: An array of the feeds to theme.
 *   - category: A common category, if any, for all the feeds.
 *
 * @return void
 *
 * @ingroup themeable
 */
function theme_aggregator_page_rss($variables) {
  $feeds = $variables['feeds'];
  $category = $variables['category'];

  drupal_add_http_header('Content-Type', 'application/rss+xml; charset=utf-8');

  $items = '';
  $feed_length = config('system.rss')->get('items.view_mode');
  foreach ($feeds as $feed) {
    switch ($feed_length) {
      case 'teaser':
        $summary = text_summary($feed->description, NULL, config('aggregator.settings')->get('items.teaser_length'));
        if ($summary != $feed->description) {
          $summary .= '<p><a href="' . check_url($feed->link) . '">' . t('read more') . "</a></p>\n";
        }
        $feed->description = $summary;
        break;
      case 'title':
        $feed->description = '';
        break;
    }
    $items .= format_rss_item($feed->ftitle . ': ' . $feed->title, $feed->link, $feed->description, array('pubDate' => date('r', $feed->timestamp)));
  }

  $site_name = config('system.site')->get('name');
  $url = url((isset($category) ? 'aggregator/categories/' . $category->cid : 'aggregator'), array('absolute' => TRUE));
  $description = isset($category) ? t('@site_name - aggregated feeds in category @title', array('@site_name' => $site_name, '@title' => $category->title)) : t('@site_name - aggregated feeds', array('@site_name' => $site_name));

  $output  = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  $output .= "<rss version=\"2.0\">\n";
  $output .= format_rss_channel(t('@site_name aggregator', array('@site_name' => $site_name)), $url, $description, $items);
  $output .= "</rss>\n";

  print $output;
}

/**
 * Page callback: Generates an OPML representation of all feeds.
 *
 * @param $cid
 *   (optional) If set, feeds are exported only from a category with this ID.
 *   Otherwise, all feeds are exported. Defaults to NULL.
 *
 * @return string
 *   An OPML formatted string.
 *
 * @see aggregator_menu()
 */
function aggregator_page_opml($cid = NULL) {
  if ($cid) {
    $result = db_query('SELECT f.title, f.url FROM {aggregator_feed} f LEFT JOIN {aggregator_category_feed} c on f.fid = c.fid WHERE c.cid = :cid ORDER BY title', array(':cid' => $cid));
  }
  else {
    $result = db_query('SELECT * FROM {aggregator_feed} ORDER BY title');
  }

  $feeds = $result->fetchAll();
  return theme('aggregator_page_opml', array('feeds' => $feeds));
}

/**
 * Prints the OPML page for the feed.
 *
 * @param array $variables
 *   An associative array containing:
 *   - feeds: An array of the feeds to theme.
 *
 * @ingroup themeable
 */
function theme_aggregator_page_opml($variables) {
  $feeds = $variables['feeds'];

  drupal_add_http_header('Content-Type', 'text/xml; charset=utf-8');

  $output  = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  $output .= "<opml version=\"1.1\">\n";
  $output .= "<head>\n";
  $output .= '<title>' . check_plain(config('system.site')->get('name')) . "</title>\n";
  $output .= '<dateModified>' . gmdate(DATE_RFC2822, REQUEST_TIME) . "</dateModified>\n";
  $output .= "</head>\n";
  $output .= "<body>\n";
  foreach ($feeds as $feed) {
    $output .= '<outline text="' . check_plain($feed->title) . '" xmlUrl="' . check_url($feed->url) . "\" />\n";
  }
  $output .= "</body>\n";
  $output .= "</opml>\n";

  print $output;
}

/**
 * Processes variables for aggregator-summary-items.tpl.php.
 *
 * @see aggregator-summary-items.tpl.php
 */
function template_preprocess_aggregator_summary_items(&$variables) {
  $variables['title'] = check_plain($variables['source'] instanceof EntityInterface ? $variables['source']->label() : $variables['source']->title);
  $summary_items = array();
  foreach (element_children($variables['summary_items']) as $key) {
    $summary_items[] = $variables['summary_items'][$key];
  }
  $variables['summary_list'] = array(
    '#theme' => 'item_list',
    '#items' => $summary_items,
  );
  $variables['source_url'] = $variables['source'] instanceof EntityInterface ? $variables['source']->url->value : $variables['source']->url;
}

/**
 * Processes variables for aggregator-summary-item.tpl.php.
 *
 * @see aggregator-summary-item.tpl.php
 */
function template_preprocess_aggregator_summary_item(&$variables) {
  $item = $variables['aggregator_item'];

  $variables['item_url'] = l(check_plain($item->label()), check_url(url($item->link->value, array('absolute' => TRUE))), array(
    'attributes' => array(
      'class' => array('feed-item-url',),
    ),
  ));
  $variables['item_age'] = theme('datetime', array(
    'attributes' => array(
      'datetime' => format_date($item->timestamp->value, 'html_datetime', '', 'UTC'),
      'class' => array('feed-item-age',),
    ),
    'text' => t('%age old', array('%age' => format_interval(REQUEST_TIME - $item->timestamp->value))),
    'html' => TRUE,
  ));
}

/**
 * Processes variables for aggregator-feed-source.tpl.php.
 *
 * @see aggregator-feed-source.tpl.php
 */
function template_preprocess_aggregator_feed_source(&$variables) {
  $feed = $variables['aggregator_feed'];

  $variables['source_icon'] = theme('feed_icon', array('url' => $feed->url->value, 'title' => t('!title feed', array('!title' => $feed->label()))));

  if (!empty($feed->image->value) && $feed->label() && !empty($feed->link->value)) {
    $variables['source_image'] = l(theme('image', array('path' => $feed->image->value, 'alt' => $feed->title->value)), $feed->link->value, array('html' => TRUE, 'attributes' => array('class' => 'feed-image')));
  }
  else {
    $variables['source_image'] = '';
  }
  $variables['source_description'] = aggregator_filter_xss($feed->description->value);
  $variables['source_url'] = check_url(url($feed->link->value, array('absolute' => TRUE)));

  if ($feed->checked) {
    $variables['last_checked'] = t('@time ago', array('@time' => format_interval(REQUEST_TIME - $feed->checked->value)));
  }
  else {
    $variables['last_checked'] = t('never');
  }

  if (user_access('administer news feeds')) {
    $variables['last_checked'] = l($variables['last_checked'], 'admin/config/services/aggregator');
  }
}

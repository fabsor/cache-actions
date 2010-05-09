<?php
// $Id$

/**
 * @file
 * This file provides the rules integration for this module.
 * @author Fabian Sörqvist <fabian.sorqvist@gmail.com>
 */

/**
 * Implementation of hook_rules_action_info().
 */
function cache_actions_rules_action_info() {
  // Actions that works for everyone
  $actions = array(
    'cache_actions_action_clear_cache' => array(
      'label' => t('Clear cache bins'),
      'arguments' => array(
        'tables' => array('type' => 'string', 'label' => t('Cache tables'), 'description' => t('Specify which cache tables you want to clear here. Separate the tables with spaces. You can use * to clear all tables')),
      ),
      'module' => 'Cache Actions',
    ),
    'cache_actions_action_clear_css_js_cache' => array(
      'label' => t('Clear the css and js cache'),
      'module' => 'Cache Actions',
    ),
  );
  
  // If the views module is available, then we can clear the cache of individual views.
  if (module_exists('views')) {
    $actions['cache_actions_action_clear_views_cache'] = array(
      'label' => t('Clear the cache of a specific view.'),
      'arguments' => array(
        'view' => array('type' => 'string', 'label' => t('View'), 'description' => t('Specify the view you want to clear.')),
      ),
      'module' => 'Cache Actions',
    );
  }
  // If the page manager module is available, then we can clear the cache of panel pages.
  if (module_exists('page_manager')) {
    $actions['cache_actions_action_clear_panels_page_cache'] = array(
      'label' => t('Clear the cache of a specific panel page.'),
      'arguments' => array(
        'panel' => array('type' => 'number', 'label' => t('Panel page'), 'description' => t('Specify the panel page you want to clear.')),
      ),
      'module' => 'Cache Actions',
    );
  }
  // If the mini panels module is available, then we can clear the cache of mini panels.
  if (module_exists('panels_mini')) {
    $actions['cache_actions_action_clear_panels_mini_cache'] = array(
      'label' => t('Clear the cache of a specific mini panel.'),
      'arguments' => array(
        'panel' => array('type' => 'number', 'label' => t('Panel page'), 'description' => t('Specify the mini panel you want to clear.')),
      ),
      'module' => 'Cache Actions',
    );
  }

  return $actions;
}

/**
 * The form for the clear panel page action
 * @param array $settings
 * @param $form the form
 */
function cache_actions_action_clear_panels_page_cache_form($settings, &$form) {
  $available_pages = array();

  // Getting the specific handlers for each page task can be a little difficult,
  // this seems to be the easiest way,
  // simply fetching the handlers from the database.
  $result = db_query("SELECT * FROM {page_manager_handlers}");

  while ($handler = db_fetch_object($result)) {
    $display = panels_load_display($handler->did);
    $task = page_manager_get_task($handler->task);
    // If this handler has a subtask, then that's what we're really after.
    if (!empty($handler->subtask)) {
      $task = page_manager_get_task_subtask($task, $handler->subtask);
    }
    // We only want to list panels that have the cache enabled.
    if ($display->cache['method'] == 'simple') {
      $available_pages[$handler->did] = $task['name'];
    }
  }
  $form['settings']['panel'] = array(
    '#type' => 'select',
    '#title' => t('Panel Page'),
    '#options' => $available_pages,
    '#default_value' => $settings['panel'],
    '#description' => t('The panel page to be cleared.
     Only panel pages which have the simple cache enabled will be shown here.'),
  );
  // Let's make sure the action can't be added if there are no applicable panel pages.
  if (!count($available_pages)) {
    form_set_error('panel', t('This action is not applicable, since there are no panel pages with caching enabled.'));
    $form['submit']['#disabled'] = TRUE;
  }
}
/**
 * The form for the clear mini panels cache action
 * @param array $settings
 * @param $form the form
 */
function cache_actions_action_clear_panels_mini_cache_form($settings, &$form) {
  $available_panels = array();
  $mini_panels = panels_mini_load_all();

  foreach ($mini_panels as $mini_panel) {
    $cache = $mini_panel->display->cache;
    if ($mini_panel->display->cache['method'] == 'simple') {
      $available_panels[$mini_panel->did] = $mini_panel->name;
    }
  }
  $form['settings']['panel'] = array(
    '#type' => 'select',
    '#title' => t('Mini Panel'),
    '#options' => $available_panels,
    '#default_value' => $settings['panel'],
    '#description' => t('The mini panel to be cleared.
     Only mini panels which have the simple cache enabled will be shown here.'),
  );
  // Let's make sure the action can't be added if there are no applicable mini panels.
  if (!count($available_panels)) {
    form_set_error('panel', t('This action is not applicable, since there are no mini panels with caching enabled.'));
    $form['submit']['#disabled'] = TRUE;
  }
}

/**
 * Clear the cache of the specified panel page.
 * @param int $did the display id
 */
function cache_actions_action_clear_panels_page_cache($did) {
  $display = panels_load_display($did);
  _cache_actions_clear_panels_cache($display);
}

/**
 * Clear the cache of the specified mini panel.
 * @param int $did the display id
 */
function cache_actions_action_clear_panels_mini_cache($did) {
  $display = panels_load_display($did);
  _cache_actions_clear_panels_cache($display);
}

/**
 * Clear a specific panel display.
 * @param display the display object.
 */
function _cache_actions_clear_panels_cache($display) {
  panels_load_include('simple', 'plugins/cache/');
  panels_simple_cache_clear_cache($display);

}

/**
 * Action drupal message configuration form.
 */
function cache_actions_action_clear_cache_form($settings, &$form) {
  $settings += array('tables' => '');
  $cache_bins = array('cache' => 'cache', 'cache_block' => 'cache_block',
    'cache_filter' => 'cache_filter', 'cache_page' => 'cache_page');
  foreach (module_invoke_all('flush_caches') as $cache_bin) {
    $cache_bins[$cache_bin] = $cache_bin;
  }
  $form['settings']['tables'] = array(
    '#type' => 'select',
    '#title' => t('Cache bins'),
    '#multiple' => TRUE,
    '#options' => $cache_bins,
    '#default_value' => $settings['tables'],
    '#description' => t('The cache bins to be cleared.'),
  );
}

/**
 * Submit handler for the cache clearing action.
 * @param array $settings the settings
 * @param $form the form
 * @param $form_state the form state.
 */
function cache_actions_action_clear_cache_submit(&$settings, $form, $form_state) {
  // The rules variable type is a string, so we just implode it.
  $settings['tables'] = implode(',', $settings['tables']);
}

/**
 * Form for the clear view action.
 */
function cache_actions_action_clear_views_cache_form($settings, &$form) {
  $settings += array('view' => '');
  $views = views_get_all_views();

  $views_names = array();
  foreach ($views as $view) {
    $views_names[$view->name] = $view->name;
  }
  $form['settings']['view'] = array(
    '#type' => 'select',
    '#options' => $views_names,
    '#default_value' => $settings['view'],
    '#title' => 'View',
  );
  // Let's make sure the action can't be added if there are no views.
  if (!count($views)) {
    form_set_error('view', t('This action is not applicable, since you don\'t have any views'));
    $form['submit']['#disabled'] = TRUE;
  }
}

/**
 * This action clears the cache of a specific view.
 * @param string $view the name of the view.
 */
function cache_actions_action_clear_views_cache($view) {
  views_include_handlers();
  require_once('./' . drupal_get_path('module', 'views') . "/plugins/views_plugin_cache.inc");
  $view = views_get_view($view);

  // We use the cache plugin to clear the cache, since that's probably the best idea.
  $cache_plugin = new views_plugin_cache();
  
  // The cache plugin does not care which display we are using when it clears the cache,
  // it will clear all caches related to this view anyway, so we can safely use any display.
  $cache_plugin->init($view, $view->display[0]);
  $cache_plugin->cache_flush();
}


/**
 * This action that clears all cache tables specified.
 * @param $tables the tables to be cleared
 */
function cache_actions_action_clear_cache($tables) {
  $tables = explode(',', $tables);
  // Fetch all available cache tables, so we can be sure that we don't remove data from a table that's not a cache.
  $core = array('cache', 'cache_block', 'cache_filter', 'cache_page');
  $cache_tables = array_merge(module_invoke_all('flush_caches'), $core);  

  foreach ($tables as $table) {
    if (array_search($table, $cache_tables) !== FALSE) {
      cache_clear_all('*', $table, TRUE);
    }
  }
}

/**
 * This action clears the css and js cache.
 */
function cache_actions_action_clear_css_js_cache() {
  // Change query-strings on css/js files to enforce reload for all users.
  _drupal_flush_css_js();
  drupal_clear_css_cache();
}

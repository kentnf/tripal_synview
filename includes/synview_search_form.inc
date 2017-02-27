<?php

/**
 * @file
 *
 */

/**
 * generate form for search syn.
 */
function synview_search_form($form, $form_state) {

  // search block by genome, chr, start, end 
  $first_opt = array('SELECT_G' => 'Select a genome');
  $org_opts = get_organism_options();
  $form['#org_opts']  = $org_opts; // pass org array through from
  $org_opts = $first_opt + $org_opts;

  $form['LOCATION'] = array(
    '#type' => 'fieldset',
    '#title' => t('Search blocks by location.'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['LOCATION']['SELECT_GENOME'] = array(
    '#type' => 'select',
    '#title' => t('Genome:'),
    '#options' => $org_opts,
    '#default_value' => 'SELECT_G', // put default db for different web
    '#ajax' => array(
      'event' => 'change',
      'callback' => 'ajax_synview_location_callback',
      'method'   => 'replace',
      'wrapper'  => 'id-textarea',
      'effect'   => 'fade',
    ),
  );

  $second_opt = array('SELECT_C' => 'Select a chromosome/scaffold');

  $form['LOCATION']['SELECT_CHR'] = array(
    '#type' => 'select',
    '#title' => t('Chromosome/Scaffold:'),
    '#prefix' => '<div id="id-textarea">',
    '#suffix' => '</div>',
    '#options' => $second_opt,
    '#default_value' => 'SELECT_C', // put default db for different web
    '#validated' => TRUE,

    //'#ajax' => array(
      //'event' => 'change',
      // 'callback' => 'ajax_synview_location_callback',
      //'method'   => 'replace',
      //'wrapper'  => 'id-textarea',
      //'effect'   => 'fade',
      //),
  );

  $form['LOCATION']['START'] = array(
    '#type' => 'textfield',
    '#title' => t('Start:'),
    '#maxlength' => 20,
    '#prefix' =>'<div id=xyz>',
  );

  $form['LOCATION']['END'] = array(
    '#type' => 'textfield',
    '#title' => t('End:'),
    '#maxlength' => 20,
    '#suffix' =>'</div>',
  );

  foreach ($form['#org_opts'] as $org_id => $org_name) {
    $label = "ORG_" . $org_id;
    if ($org_id == 1) {
      $form['LOCATION'][$label] = array(
        '#type' => 'checkbox',
        '#title' => $org_name,
        '#prefix' => '<br><p><b>Please choose one genome at least:</b></p><span style="float: left; display: block; margin: 10px;">',
        '#suffix' => '</span>',
      );
    } else {
      $form['LOCATION'][$label] = array(
        '#type' => 'checkbox',
        '#title' => $org_name,
        '#prefix' => '<span style="float: left; display: block; margin: 10px;">',
        '#suffix' => '</span>',
      );
    }
  }

  //.........................
  $form['ID'] = array(
    '#type' => 'fieldset',
    '#title' => t('<b>OR</b> Search block by ID.'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['ID']['INPUT_ID'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter a block ID. Example: test00001'),
    '#maxlength' => 20,
  );

  // Submit
  //.........................
  $form['submit'] = array(
    '#type' => 'submit',
    '#default_value' => ' Search ',
  );

  $form['#prefix'] = '<div class="row"> <div class="col-md-8 col-md-offset-2">';
  $form['#suffix'] = '</div> </div>';

  return $form;
}

/**
 * Validate the user options submitted via the above form.
 */
function synview_search_form_validate($form, &$form_state) {

  if (empty($form_state['values']['INPUT_ID'])) {
    // if no block ID is provided
    // the genome, chr, and ref genome should be required
    $org_id = $form_state['values']['SELECT_GENOME'];
    $chr    = $form_state['values']['SELECT_CHR'];
    $start  = $form_state['values']['START'];
    $end    = $form_state['values']['END'];

    if (empty($form_state['values']['SELECT_GENOME']) || $form_state['values']['SELECT_GENOME'] == 'SELECT_G') {
      form_set_error('SELECT_GENOME', t('No genome selected. Choose a genome from the list.'));
    }

    if (empty($chr)) {
      form_set_error('SELECT_CHR', t('No chromosome selected. Choose a chromosome from the list.'));
    }

    if ($start and !preg_match('/^\d+$/', $start)) {
      form_set_error('START', 'Please enter a positive numeric value for the start');
    }
    if ($end and !preg_match('/^\d+$/', $end)) {
      form_set_error('END', 'Please enter a positive numeric value for the end');
    }

    $ref_org = array();
    foreach ($form['#org_opts'] as $org_id => $org_name) {
      $label = "ORG_" . $org_id;
      if ($form_state['values'][$label]) {
        $ref_org[$org_id] = $org_name;
      }  
    }

    if (count($ref_org) == 0) {
      form_set_error('LOCATION', t('No reference genome selected. Check one genome at least.'));
    }

    $form_state['refORG'] = $ref_org;
    $form_state['byFlag'] = 'location';
  } else {
    $form_state['byFlag'] = 'id';
  }

}

/**
 * Process the user options submitted.
 */
function synview_search_form_submit($form, &$form_state) {

  $error = FALSE;

  // search database
  if ($form_state['byFlag'] == 'location') {
    $org_id = $form_state['values']['SELECT_GENOME'];
    $chr = $form_state['values']['SELECT_CHR'];
    $start = $form_state['values']['START'];
    $end = $form_state['values']['END'];
    $ref_org = $form_state['refORG']; 

    $_SESSION['tripal_synview_search']['SELECT_GENOME'] = $form_state['values']['SELECT_GENOME'];
    $_SESSION['tripal_synview_search']['SELECT_CHR']    = $form_state['values']['SELECT_CHR'];
    $_SESSION['tripal_synview_search']['START']         = $form_state['values']['START'];
    $_SESSION['tripal_synview_search']['END']           = $form_state['values']['END'];
    $_SESSION['tripal_synview_search']['REFORG']        = $form_state['refORG'];

    drupal_goto("synview/search/result");
  } 
  elseif ($form_state['byFlag'] == 'id') {
    // if search block id, go to display_block page
    // otherwise, show error information
    $block_id = $form_state['values']['INPUT_ID'];
    $blk_info = db_query('SELECT value FROM {synblock} WHERE blockid=:block_id ', array(':block_id' => $block_id))->fetchField();
   
    if(!empty($blk_info)) {
      drupal_goto("synview/block/$block_id");
    } else {
      drupal_set_message("Can not find block ID: $block_id", 'warning');
    }
  }
  else {
    $error = TRUE;
    tripal_report_error(
      'synview',
      TRIPAL_ERROR,
      "Can not determine the search method byFlag."
    );
  }
}

/**
 * AJAX: change the selection of chr according genome.
 */
function ajax_synview_location_callback($form, $form_state) {

  $genome_sel = $form_state['values']['SELECT_GENOME'];

  $syn_region = chado_select_record('feature', array('feature_id'), array('organism_id'=>$genome_sel, 'type_id'=>array('name' => 'syntenic_region')));

  $sql = "SELECT F.feature_id, F.uniquename FROM chado.feature F
     LEFT JOIN chado.featureloc FL ON F.feature_id = FL.srcfeature_id
     WHERE
       FL.feature_id = :feature_id";

  $chr_opts = $form['LOCATION']['SELECT_CHR']['#options'];
  foreach ($syn_region as $syn) {
    $result = chado_query($sql, array(':feature_id'=>$syn->feature_id));
    foreach ($result as $r) {
      $chr_opts[$r->feature_id] = $r->uniquename;
    }
  }

  $form['LOCATION']['SELECT_CHR']['#options'] = $chr_opts;
  $form['LOCATION']['SELECT_CHR']['#validated'] = TRUE;

  return $form['LOCATION']['SELECT_CHR'];
}

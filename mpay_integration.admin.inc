<?php

/**
 * @file
 * MPay integration settings form. 
 * Set up MPay service order URL and id, path to certificate, path to CA file.
 * Enable ip filter.
 */

/**
 * Implements hook_form().
 */
function mpay_integration_form($form_state) {
  $form = array();

  $form['mpay_integration_service_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Service order url'),
    '#default_value' => variable_get('mpay_integration_service_url', ''),
    '#required' => TRUE,
  );

  $form['mpay_integration_service_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Service Id'),
    '#default_value' => variable_get('mpay_integration_service_id', ''),
    '#required' => TRUE,
  );

  $form['mpay_integration_private_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to our private key'),
    '#default_value' => variable_get('mpay_integration_private_key', ''),
    '#description' => t('Need to sign our soap requests'),
    '#required' => TRUE,
  );

  $form['mpay_integration_certificate'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to our certificate'),
    '#default_value' => variable_get('mpay_integration_certificate', ''),
    '#description' => t('Need to sign our soap requests'),
    '#required' => TRUE,
  );

  $form['mpay_integration_enable_ip_filter'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable ip filter'),
    '#default_value' => variable_get('mpay_integration_enable_ip_filter', FALSE),
  );

  $form['mpay_integration_allowed_ip'] = array(
    '#type' => 'textarea',
    '#title' => t('Allowed ip address'),
    '#default_value' => variable_get('mpay_integration_allowed_ip', ''),
    '#description' => t('One ip pe line'),
    '#states' => array(
      'required' => array(
        'input[name="mpay_integration_enable_ip_filter"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['mpay_integration_validate_certificate_issuer'] = array(
    '#type' => 'checkbox',
    '#title' => t('Validate incoming certificate'),
    '#default_value' => variable_get('mpay_integration_validate_certificate_issuer', FALSE),
  );

  $form['mpay_integration_path_to_CA_file'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to CA file'),
    '#default_value' => variable_get('mpay_integration_path_to_CA_file', ''),
    '#description' => t('Need to validate received certificate. Received certificate must be signed by CA'),
    '#states' => array(
      'required' => array(
        'input[name="mpay_integration_validate_certificate_issuer"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['mpay_integration_validate_certificate_CN'] = array(
    '#type' => 'textfield',
    '#title' => t('Received certificate CN'),
    '#default_value' => variable_get('mpay_integration_validate_certificate_CN', ''),
    '#description' => t('Need to validate received certificate in soap request.'),
  );

  $form['#validate'][] = 'mpay_integration_form_validate';

  return system_settings_form($form);
}

/**
 * Implements hook_validate().
 */
function mpay_integration_form_validate($form, $form_state) {
  if (!empty($form_state['values']['mpay_integration_private_key'])) {
    if (!file_exists($form_state['values']['mpay_integration_private_key'])) {
      form_set_error('mpay_integration_private_key', t('Private key not found'));
    }
  }

  if (!empty($form_state['values']['mpay_integration_certificate'])) {
    if (!file_exists($form_state['values']['mpay_integration_certificate'])) {
      form_set_error('mpay_integration_certificate', t('Certificate not found'));
    }
  }

  if ($form_state['values']['mpay_integration_validate_certificate_issuer'] && empty($form_state['values']['mpay_integration_path_to_CA_file'])) {
    form_set_error('mpay_integration_path_to_CA_file', t('Path to CA file is required'));
  }

  if (!empty($form_state['values']['mpay_integration_path_to_CA_file'])) {
    if (!file_exists($form_state['values']['mpay_integration_path_to_CA_file'])) {
      form_set_error('mpay_integration_path_to_CA_file', t('CA file found'));
    }
  }

  if ($form_state['values']['mpay_integration_enable_ip_filter'] && empty($form_state['values']['mpay_integration_allowed_ip'])) {
    form_set_error('mpay_integration_allowed_ip', t('Allowed ip address.'));
  }
}

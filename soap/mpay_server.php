<?php
define('MPAY_SERVER_WSDL_CACHE_PREFIX', 'mpay_integration-wsdl-');
define('DRUPAL_MODULE_ROOT_RE', '|/sites/all/modules.*|');

ignore_user_abort(true);
set_time_limit(0);

ini_set('session.gc_maxlifetime', 1440);
session_set_cookie_params(1440);

/**
 * Get path to WSDL file that contains soap:address location attribute corrected according to request made by client
 * Also make cached copies of modified WSDL file.
 *
 * @param string $service_url
 *  URL of SOAP service
 * @return string
 *  Absolute or relative path to WSDL file.
 */
function get_wsdl_path($service_url) {
  global $drupal_url;
  $source_wsdl_path = drupal_get_path('module', 'mpay_integration') . '/soap/mpay-server.wsdl';

  //Fallback to default WSDL file if files directory is not properly configured
  $wsdl_path = $source_wsdl_path;

  //check for wsdl file with such suffix (hashed suffix)
  $try_mpay_server_xsd_path = 'public://' . MPAY_SERVER_WSDL_CACHE_PREFIX . md5($service_url) . 'mpay-server.xsd';
  $try_mpay_server1_xsd_path = 'public://' . MPAY_SERVER_WSDL_CACHE_PREFIX . md5($service_url) . 'mpay-server1.xsd';
  $try_wsdl_path = 'public://' . MPAY_SERVER_WSDL_CACHE_PREFIX . md5($service_url) . '.wsdl';

  if ($try_wsdl_path) {
    $wsdl_path = $try_wsdl_path;

    // if missing: compose new url with path = soap/?wsdl
    if (!is_readable($wsdl_path)) {
      // xml load, change, save
      $wsdl = simplexml_load_file($source_wsdl_path);
      $namespaces = $wsdl->getNamespaces(TRUE);
      $wsdl->children($namespaces["wsdl"])->types->children($namespaces["xsd"])->schema->import[0]->attributes()->schemaLocation = file_create_url($try_mpay_server1_xsd_path);
      $wsdl->children($namespaces["wsdl"])->types->children($namespaces["xsd"])->schema->import[1]->attributes()->schemaLocation = file_create_url($try_mpay_server_xsd_path);
      $wsdl->children($namespaces["wsdl"])->service->port->children($namespaces["soap"])->address->attributes()->location = $service_url;
      $wsdl->asXML($wsdl_path);

      //  Load xsd Files and save in right place
      $source_wsdl_path = drupal_get_path('module', 'mpay_integration') . '/soap/mpay-server1.xsd';
      $wsdl = simplexml_load_file($source_wsdl_path);
      $namespaces = $wsdl->getNamespaces(TRUE);
      $wsdl->children($namespaces["xs"])->import->attributes()->schemaLocation = file_create_url($try_mpay_server1_xsd_path);
      $wsdl->asXML($try_mpay_server1_xsd_path);

      $source_wsdl_path = drupal_get_path('module', 'mpay_integration') . '/soap/mpay-server.xsd';
      $wsdl = simplexml_load_file($source_wsdl_path);
      $wsdl->asXML($try_mpay_server_xsd_path);

    }
  }

  return $wsdl_path;
}

$soap_server_dir = getcwd();
$drupal_path = preg_replace(DRUPAL_MODULE_ROOT_RE, '', $soap_server_dir) . '/';

chdir($drupal_path);

// Set $_SERVER['SCRIPT_NAME'] to /index.php to properly generate base_url/base_path
define('DRUPAL_ROOT', getcwd());
$_SERVER['SCRIPT_NAME'] = '/index.php';

require_once 'includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
global $base_url;

// watchdog('mpay_integration', 'IN Request, HTTP_RAW_POST_DATA: @HTTP_RAW_POST_DATA php_input: @php_input', array('@HTTP_RAW_POST_DATA'=>$GLOBALS['HTTP_RAW_POST_DATA'], '@php_input'=>file_get_contents('php://input')), WATCHDOG_DEBUG);

if(variable_get('mpay_integration_enable_ip_filter', FALSE)) {
  $ip = explode("\n", variable_get('mpay_integration_allowed_ip', ''));
  if(!in_array(ip_address(), $ip)) {
    header('HTTP/1.0 403 Forbidden');
    exit();
  }
}

$module_path = drupal_get_path('module', 'mpay_integration');

include_once (DRUPAL_ROOT . "/{$module_path}/soap/mpayServer.class.inc");
include_once (DRUPAL_ROOT . "/{$module_path}/soap/server.api.inc");
include_once (DRUPAL_ROOT . "/{$module_path}/soap/signer.api.inc");
include_once (DRUPAL_ROOT . "/{$module_path}/libraries/soap-server-wsse.php");
include_once (DRUPAL_ROOT . "/{$module_path}/libraries/soap-wsse.php");
include_once (DRUPAL_ROOT . "/{$module_path}/libraries/soap-wsa.php");

use_soap_error_handler(TRUE);
$drupal_url   = preg_replace( DRUPAL_MODULE_ROOT_RE, '', $base_url);
$service_url  = $drupal_url . '/mpay/payment-interface?wsdl';
$wsdl_path = get_wsdl_path($service_url);

$server = new SoapServerExt($wsdl_path);
$server->setClass("mpayServer");

ob_start();
$server->handle();
$soapResponse = ob_get_contents();
ob_end_clean();
header('HTTP/1.1 200 OK');

if (empty($GLOBALS['HTTP_RAW_POST_DATA']) && !file_get_contents('php://input')) { //request for receive wsdl
  echo $soapResponse;
  exit();
}

$signer = new soapSigner();
$result = $signer->signRequest($soapResponse);

// watchdog('mpay_integration', 'Response, signer: @result', array('@result'=>$result), WATCHDOG_DEBUG);
$length = strlen($result);

header("Content-Length: ".$length);
echo $result;
exit();

<?php

class SoapServerExt extends SoapServer {

  /**
   * Collect some debugging values and handle the soap request.
   *
   * @param string $request
   * @return void
   */
  public function handle($request = null) {
    $clear_request = null;
    // check variable HTTP_RAW_POST_DATA
    if (!isset($GLOBALS['HTTP_RAW_POST_DATA'])) {
      $GLOBALS['HTTP_RAW_POST_DATA'] = file_get_contents('php://input');
    }

    // check input param
    if (is_null($request)) {
      $request = $GLOBALS['HTTP_RAW_POST_DATA'];
    }

    if($request) {
      require_once 'signer.api.inc';
      $signer = new soapSigner();
      $clear_request = $signer->validateRequest($request);
    }

    parent::handle($clear_request);
  }

}

function mpay_log($request_type, $message, $soap_request = FALSE) {
  if($soap_request) {
    $path = variable_get('mpay_log_soap_path', drupal_realpath('private://'));
  }
  else {
    $path = variable_get('mpay_log_path', drupal_realpath('private://'));
  }

  if (!$path) {
    return;
  }
  $path .= '/' . date('Y');
  try {
    if (!file_prepare_directory($path, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS)) {
      throw new LogException('Can not prepare writable directory @dir for MPay server logs.', array('@dir' => $path));
    }

    $save_message = "----------------------------------------------------------------------------------------\n";
    $save_message .= ip_address() ." {$request_type} " . date('d-m-Y H:i:s') . ":\n";
    $save_message .= $message . "\n";
    $save_message .= "----------------------------------------------------------------------------------------\n";

    file_put_contents(realpath($path) . '/' . date('d-m-Y') . '.txt', $save_message, FILE_APPEND);
  }
  catch (LogException $e) {
    watchdog('mpay_integration', $e->getMessage(), $e->getArgs(), WATCHDOG_CRITICAL);
  }catch (Exception $e) {
    watchdog('mpay_integration', $e->getMessage(), array(), WATCHDOG_CRITICAL);
  }
}

class LogException extends Exception {
  private $args;
  public function __construct($message, $args = array(), $code = 0, Exception $previous = null) {
    $this->args = $args;
    parent::__construct($message, $code, $previous);
  }

  public function getArgs() {
    return is_array($this->args) ? $this->args : array();
  }
}

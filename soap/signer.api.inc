<?php

class soapSigner {
  private $path_to_private_key;
  private $path_to_certificate;

  public function __construct() {
    $this->path_to_private_key = variable_get('mpay_integration_private_key', '');
    $this->path_to_certificate = variable_get('mpay_integration_certificate', '');
  }

  public function validateRequest($request) {
    mpay_log('IN', $request, TRUE);
    try {
      $dom = new DOMDocument();
      $dom->loadXML($request);

      $objWSSE = new WSSESoapServer($dom);
      $certificate = $objWSSE->getCertificate();
      if(variable_get('mpay_integration_validate_certificate_issuer', FALSE)) {
        $this->validateCertificate($certificate);
      }

      $proc_res = $objWSSE->process();
      if (!isset($proc_res)) {
        throw new Exception('DOM does not have sec node.');
      }
    } catch (Exception $e) {
      watchdog('mpay_integration', $e->getMessage(), (array)$e, WATCHDOG_CRITICAL);
      throw new SoapFault('soap_fault', $e->getMessage());
    }

    $request_cut = $objWSSE->saveXML();
    return $request_cut;
  }

  public function signRequest($request) {
    try {
      $dom = new DOMDocument();
      $dom->loadXML($request);

      $objWSSE = new WSSESoap($dom);

      /* Sign all headers to include signing the WS-Addressing headers */
      $objWSSE->signAllHeaders = TRUE;

      $objWSSE->addTimestamp();

      /* create new XMLSec Key using RSA SHA-1 and type is private key */
      $objKey = new XMLSecurityKey(XMLSecurityKey::RSA_SHA1, array('type'=>'private'));

      /* load the private key from file - last arg is bool if key in file (TRUE) or is string (FALSE) */
      $objKey->loadKey($this->path_to_private_key, TRUE);

      /* Sign the message - also signs appropraite WS-Security items */
      $objWSSE->signSoapDoc($objKey, array('insertBefore' => false));

      $objWSSE->mPayAttachCertificateInfo(file_get_contents($this->path_to_certificate));

      $request = $objWSSE->saveXML();
    }
    catch(Exception $e) {
      watchdog('mpay_integration', $e->getMessage(), (array)$e, WATCHDOG_CRITICAL);
      exit();
    }

    mpay_log('OUT', $request, TRUE);
    return $request;
  }

  public function validateCertificate($certificate) {
    $cn = variable_get('mpay_integration_validate_certificate_CN', '');
    if(!empty($cn)) {
      $parse_certificate = openssl_x509_parse($certificate);
      if($cn !== $parse_certificate['subject']['CN']) {
        watchdog('mpay_integration', 'Can not validate incoming certificate. Invalid CN: !CN', array('!CN' => $parse_certificate['subject']['CN']));
        throw new SoapFault('soap_fault', 'Can not validate incoming certificate. Invalid CN: ' . $parse_certificate['subject']['CN']);
      }
    }

    $trusted_certificate = variable_get('mpay_integration_path_to_CA_file', '');
    if ($trusted_certificate) {
      $path = drupal_get_path('module', 'mpay_integration');
      require_once($path . '/libraries/phpseclib/File/X509.php');
      require_once($path . '/libraries/phpseclib/Math/BigInteger.php');
      require_once($path . '/libraries/phpseclib/Crypt/RSA.php');
      $x509 = new File_X509();

      $x509->loadCA(file_get_contents($trusted_certificate));
      $x509->loadX509($certificate);
      $res = $x509->validateSignature();

      if(!$res) {
        watchdog('mpay_integration', 'Can not validate incoming certificate - not trusted.');
        throw new SoapFault('soap_fault', 'Can not validate incoming certificate - not trusted.');
      }
    }
  }
}

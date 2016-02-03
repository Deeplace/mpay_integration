<?php

/**
 * @file
 * For testing purpose.
 */

require('includes/soap-wsse.php');
require('includes/soap-wsa.php');

define('PRIVATE_KEY', 'data/server.key');
define('CERT_FILE', 'data/server.cer');

class mySoap extends SoapClient {
  function __doRequest($request, $location, $saction, $version) {
    $dom = new DOMDocument();
    $dom->loadXML($request);

    $objWSSE = new WSSESoap($dom);
    /* Sign all headers to include signing the WS-Addressing headers */
    $objWSSE->signAllHeaders = TRUE;

    $objWSSE->addTimestamp();

    /* create new XMLSec Key using RSA SHA-1 and type is private key */
    $objKey = new XMLSecurityKey(XMLSecurityKey::RSA_SHA1, array('type'=>'private'));

    /* load the private key from file - last arg is bool if key in file (TRUE) or is string (FALSE) */
    $objKey->loadKey(PRIVATE_KEY, TRUE);

    // Sign the message - also signs appropraite WS-Security items 
    $objWSSE->signSoapDoc($objKey);

    /* Add certificate (BinarySecurityToken) to the message and attach pointer to Signature */
    $token = $objWSSE->addBinaryToken(file_get_contents(CERT_FILE));
    $objWSSE->attachTokentoSig($token);

    $request = $objWSSE->saveXML();

    $dom = new DOMDocument();
    $dom->loadXML($request);
    $objWSA = new WSASoap($dom);
    $objWSA->addAction($saction);
    $objWSA->addTo($location);
    $objWSA->addMessageID();
    $objWSA->addReplyTo();

    $request = $objWSA->getDoc()->saveXML();
    return parent::__doRequest($request, $location, $saction, $version);
  }
}
// To test Soap server you should write your site's URL below, like $wsdl = '<your_site_url>/mpay/payment-interface?wsdl';
$wsdl = 'https://eservices/mpay/payment-interface?wsdl';
try {
  $sc = new mySoap($wsdl, array('trace' => 1, WSDL_CACHE_NONE => TRUE));
  var_dump($sc);
} catch (SoapFault $e) {
  echo $e->getMessage();
}

try {
  $param = array(
    'query' => array(
      'OrderKey' => '1000453',
      'ServiceID' => 'TEST01'
    )
  );

  $response = $sc->GetOrderDetails($param);

  $param = array(
    'confirmation' => array(
      'OrderKey' => '1000453',
      'ServiceID' => 'TEST01',
      'PaymentID' => '43dfdr43',
      'PaidAt' => '11-05-2015 15:45:26',
      'TotalAmount' => '50.00',
      'Currency' => 'MDL',
      'Lines' => array(
        array(
          'LineID' => '5318328d4c482',
          'Reason' => 'Marci',
          'Amount' => '500.00'
        )
      )
    )
  );

  $response = $sc->ConfirmOrderPayment($param);
} catch (SoapFault $fault) {
  echo '<pre><br>';
  echo 'Message:<br>' . print_r($fault->getMessage(),1) . '<br>';
  echo 'Code:<br>' . print_r($fault->getCode(),1) . '<br>';
  echo 'File:<br>' . print_r($fault->getFile(),1) . '<br>';
  echo 'Line:<br>' . print_r($fault->getLine(),1) . '<br>';
  echo 'Prev:<br>' . print_r($fault->getPrevious(),1) . '<br>';
  echo 'Trace:<br>' . print_r($fault->getTrace(),1) . '<br>';
  echo 'StringTrace:<br>' . print_r($fault->getTraceAsString(),1) . '<br><br>';
  echo '<strong>Raw Soap Fault:</strong>';
  var_dump($fault);
}

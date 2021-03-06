### Drupal 7 MPay Integration module ##

#### Summary
----------
 * Introduction
 * Signing keys
 * Settings
 * Variables
 * Hooks
 * Sample client implementation

#### Introduction
---------------
MPay is the service through which you can pay off public services with any payment instrument of your choice, such as credit card, internet banking or cash.

#### Signing keys
--------------------

RSA keys and X509 certificates must be placed in PEM format.

#### Settings
-------------

After module installation it is necessary to add in a .htaccess file the following line:

```
RewriteRule ^mpay/payment-interface(.*)$ sites/all/modules/mpay_integration/soap/mpay_server.php [L, QSA]
```

after rows:

```
  <IfModule mod_rewrite.c>
	RewriteEngine on.
```

In the final version it should looks like this:

```
  <IfModule mod_rewrite.c>
  RewriteEngine on

  RewriteRule ^mpay/payment-interface(.*)$
  sites/all/modules/mpay_integration/soap/mpay_server.php [L,QSA]
```

Module's settings can be found in *Administration » Configuration » Web services » Mpay integration settings.*
In field *Path to our private key* enter path to PEM key file like sites/all/modules/mpay\_integration/soap/data/server.key.

In field *Path to our certificate* enter path to X509 certificate like sites/all/modules/mpay\_integration/soap/data/server.cer

There is client/ subdirectory, you may use it to test SOAP server.

#### Variables
-------------

 * string `mpay_integration_private_key` - path to RSA private key in PEM format.
 * string `mpay_integration_certificate` - path to X509 certificate file in PEM format.
 * boolean `mpay_integration_validate_certificate_issuer` - allow or disallow
   validation of MPay Service certificate used to sign messages. By default: FALSE.
 * string `mpay_integration_validate_certificate_CN` - Common Name part of MPay
   Service certificate, if MPay certificate validation is enabled.
 * string `mpay_integration_path_to_CA_file` - path to PEM file containing X509
   certificate of issuer of MPay service certificate.
 * boolean `mpay_integration_enable_ip_filter` - allow or disallow
   validation of MPay Service request IP address. By default: FALSE.
 * string `mpay_integration_allowed_ip` - list of CIDR notations of IP adresses,
   separated by semicolon.
 * string `mpay_integration_service_url` - URL of MPay service for user
   redirection.
 * string `mpay_integration_service_id` - application identifier, as registered in
   MPay application profile.
 * string `mpay_log_soap_path` - path to directory for debuging records of
   SOAP requests sent/received. FALSE - disables logging.
 * string `mpay_log_path` - path to directory for debuging records of
   webservice messages sent/received. FALSE - disables logging.

#### Hooks
---------

Implementation of these hooks are required:
 * *hook_mpay_get_payment_info(array $order)* - invoked by GetOrderDetails()
   method.
  $order is order information with properties:
      string ServiceID
      string OrderKey
  hook should return OrderDetails object for requested OrderKey.
 * *hook_mpay_confirm_payment(array $confirmation)* - invoked by ConfirmOrderPayment()
   method.
  $confirmation is confirmation array as returned by MPay Service.

#### Sample client code

```
/**
 * Implementation of hook_mpay_get_payment_info()
 */
function example_mpay_confirm_payment(array $order_data) {
  $payment = order_load($order_data['OrderKey']);
  if(!$payment) {
    throw new SoapFault('UnknownOrder', 'Not found order for confirmation with key OrderKey: ' . $order_data['OrderKey']);
  }

  db_update('orders')
    ->fields(array('success_transaction_id' => $order_data['PaymentID']))
    ->condition('pid', $payment->id)
    ->execute();
}

/**
 * Implementation of hook_mpay_get_payment_info()
 */
function example_mpay_get_payment_info(array $order_data) {
  $payment = order_load($order_data['OrderKey']);

  if(!$payment) {
    return array();
  }

  $payment_info = array();

  $saved_locale = setlocale(LC_CTYPE, 'ro_RO.UTF-8', 'ro_RO.utf8', 'ru_RU.UTF8', 'en_US.UTF-8');
  $payment_info['Reason'] = iconv("UTF-8", "ASCII//TRANSLIT", $payment->reason);

  switch($payment->status) {
    case 'in_process':
      $payment_info['Status'] = MPAY_INTEGRATION_STATUS_ACTIVE;
      break;
    case 'success':
      $payment_info['Status'] = MPAY_INTEGRATION_STATUS_COMPLETED;
      break;
    default:
      $payment_info['Status'] = MPAY_INTEGRATION_STATUS_CANCELLED;
  }

  $additional_fee = 0;

  if($payment->customer_type === 'organization') {
    $payment_info['CustomerType'] = 'Organization';
    $additional_fee += 1.2;
  }
  else {
    $payment_info['CustomerType'] = 'Person';
  }

  //mpay supports only date, and does not support dateTime
  $payment_info['DueDate'] = gmdate('Y-m-d\T00:00:00', strtotime('+1 day')) . 'Z';


  $payment_info['TotalAmountDue'] = $payment->total_to_pay + $additional_fee;
  $payment_info['Currency'] = 'MDL';
  $payment_info['AllowPartialPayment'] = FALSE;
  $payment_info['AllowAdvancedPayment'] = FALSE;

  $payment_info['Lines'][] = array(
    'LineID' => uniqid(),
    'Reason' => $payment_info['Reason'],
    'AmountDue' => $payment_info['TotalAmountDue']
  );

  setlocale(LC_CTYPE, $saved_locale);

  return $payment_info;
}

```

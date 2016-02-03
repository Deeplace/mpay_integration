<?php

class mpayServer {

  public function GetOrderDetails($order_data) {
    try {
      mpay_log('IN', var_export($order_data, 1));
      $prep_data = (array)$order_data->query;

      $data = module_invoke_all('mpay_get_payment_info', $prep_data);

      if(empty($data)) {
        throw new Exception('Internal error. Cant generate order details. Not found order key');
      }

      $data['ServiceID'] = $prep_data['ServiceID'];
      $data['OrderKey'] = $prep_data['OrderKey'];
      $return = array('GetOrderDetailsResult' => array('OrderDetails' => $data));

      mpay_log('OUT', var_export($return, 1));
      return $return;
    }
    catch (Exception $e) {
      watchdog('mpayServer', $e->getMessage(), (array)$e);
      mpay_log('OUT', var_export($e->__toString(), 1));
      throw new SoapFault("InternalError", $e->getMessage());
    }
  }

  public function ConfirmOrderPayment($payment_confirmation) {
    try {
      mpay_log('IN', var_export($payment_confirmation, 1));
      $prep_data = (array)$payment_confirmation->confirmation;
      module_invoke_all('mpay_confirm_payment', $prep_data);
    }
    catch (SoapFault $e) {
      watchdog('mpayServer', $e->getMessage(), (array)$e);
      mpay_log('OUT', var_export($e->__toString(), 1));
      throw new SoapFault($e->getCode(), $e->getMessage());
    }
    catch (Exception $e) {
      watchdog('mpayServer', $e->getMessage(), (array)$e);
      mpay_log('OUT', var_export($e->__toString(), 1));
      throw new SoapFault("InternalError", $e->getMessage());
    }
  }
}

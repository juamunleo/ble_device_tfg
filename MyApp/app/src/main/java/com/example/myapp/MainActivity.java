package com.example.myapp;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Color;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.Uri;
import android.os.Build;
import android.provider.ContactsContract;
import android.util.Log;
import android.view.*;
import android.os.Bundle;
import android.view.autofill.AutofillValue;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ThemedSpinnerAdapter;
import android.widget.Toast;

import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.stream.Collectors;

public class MainActivity<bluetoothAdapter> extends AppCompatActivity {
    private static Context context;
    PackageManager packageManager;
    private String TAG = "[Info]";
    private BluetoothManager bluetoothManager;
    private BluetoothAdapter bluetoothAdapter;
    BluetoothDevice bluetoothDevice;
    private ScanCallback bleScanCallback;
    private List<BluetoothDevice> bledevices;
    private int veces = 0;
    final int LOCATION_REFRESH_TIME = 3000; // 15 seconds to update
    final int LOCATION_REFRESH_DISTANCE = 0; // 500 meters to update
    public static final UUID UUID_SERVICE = UUID.fromString("0000af00-0000-1000-8000-00805f9b34fb");
    public static final UUID UUID_CHARACTERISTIC_BUTTON = UUID.fromString("0000c001-0000-1000-8000-00805f9b34fb");
    public static final UUID UUID_DESCRIPTOR_BUTTON = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    public TextView button_state;
    public Spinner ble_devices_list;
    public TextView bond_button;
    public TextView connection_state;
    BluetoothGatt mBluetoothGatt;
    BluetoothLeScanner lescanner;
    ArrayAdapter<String> adapter;
    private BluetoothGattCallback mGattCallback;
    protected LocationManager locationManager;
    protected LocationListener locationListener;
    private Location currentLocation;

    @RequiresApi(api = Build.VERSION_CODES.N)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        button_state = findViewById(R.id.button_state_id);
        connection_state = findViewById(R.id.connection_status_id);
        ble_devices_list = findViewById(R.id.ble_devices_list_id);
        bond_button = findViewById(R.id.bond_button);
        MainActivity.context = getApplicationContext();
        locationManager = (LocationManager) getSystemService(context.LOCATION_SERVICE);
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        startLocationListener();
        locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, LOCATION_REFRESH_TIME, LOCATION_REFRESH_DISTANCE, locationListener);
        packageManager = context.getPackageManager();
        bluetoothManager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = bluetoothManager.getAdapter();
        bledevices = new ArrayList<>();
        button_state.setText("EL BOTON SE HA PULSADO 0 VECES");
        setEvents();
        connectToBond();
        Log.i(TAG, "App iniciada. Buscando dispositivos...");
        refreshList(null);
    }

    public void startLocationListener(){
        locationListener = new LocationListener() {
            @Override
            public void onLocationChanged(@NonNull Location location) {
                currentLocation = location;
                Log.i(TAG, "Location updated");
            }
        };
    }

    public void startClient() {
        try {
            mGattCallback = new BluetoothGattCallback() {

                private final String TAG = "mGattCallback";

                @Override
                public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {

                    super.onConnectionStateChange(gatt, status, newState);
                    Log.i(TAG, "Ha cambiado el estado de conexión:");
                    Log.i(TAG, status + " " + newState);
                    if (newState == BluetoothProfile.STATE_CONNECTED) {
                        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                            return;
                        }
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                    return;
                                }
                                connection_state.setText("CONECTADO A: " + bluetoothDevice.getName());
                                veces++;
                                //button_state.setText("EL BOTON SE HA PULSADO " + veces + " VECES");
                                button_state.setText("Latitud: " + currentLocation.getLatitude() + ", Longitud: "+currentLocation.getLongitude());
                            }
                        });

                        mBluetoothGatt.discoverServices();
                    } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                connection_state.setText("DESCONECTADO");
                            }
                        });

                    }
                }

                @Override
                public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                    //gatt.getServices().stream().forEach(x->Log.i(TAG, "Service discovered:"+x.getUuid()));

                    if (status == gatt.GATT_SUCCESS) {
                        BluetoothGattService service = gatt.getService(UUID_SERVICE);
                        if (service != null) {
                            Log.i(TAG, "Service connected");
                            BluetoothGattCharacteristic characteristic = service.getCharacteristic(UUID_CHARACTERISTIC_BUTTON);
                            if (characteristic != null) {
                                Log.i(TAG, "Characteristic connected");
                                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                    return;
                                }
                                gatt.setCharacteristicNotification(characteristic, true);
                                BluetoothGattDescriptor desc = characteristic.getDescriptor(UUID_DESCRIPTOR_BUTTON);
                                desc.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                                gatt.writeDescriptor(desc);

                            }
                        }
                    }
                }

                @Override
                public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
                    if (characteristic.getUuid().equals(UUID_CHARACTERISTIC_BUTTON)) {
                        if (characteristic.getValue()[0] == 1) {
                            Log.i(TAG, "Botón se ha pulsado");
                            runOnUiThread(new Runnable() {

                                @Override
                                public void run() {
                                    button_state.setText("PULSADO");
                                }
                            });

                        } else if (characteristic.getValue()[0] == 0) {
                            Log.i(TAG, "Botón se ha levantado");
                            runOnUiThread(new Runnable() {

                                @Override
                                public void run() {
                                    button_state.setText("LEVANTADO");
                                }
                            });
                        }
                    }
                }
            };

            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                return;
            }
            mBluetoothGatt = bluetoothDevice.connectGatt(this, true, mGattCallback);

            if (mBluetoothGatt == null) {
                Log.w(TAG, "Unable to create GATT client");
                return;
            } else {
                Log.w(TAG, "Gatt client started");
            }
        } catch (Exception e) {
            Log.w(TAG, e.toString());
        }
    }

    @SuppressLint("ResourceType")
    @RequiresApi(api = Build.VERSION_CODES.N)
    public void refreshList(View view) {
        lescanner = bluetoothAdapter.getBluetoothLeScanner();
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            Log.i(TAG, "Permiso denegado: adapter.");
            return;
        }else {
            adapter = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_dropdown_item);
            bledevices.clear();
            bleScanCallback =
                    new ScanCallback() {
                        @Override
                        public void onScanResult(int callbackType, ScanResult result) {
                            super.onScanResult(callbackType, result);
                            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                Log.i(TAG, "Permiso denegado: onScanResult.");
                                return;
                            } else {
                                if (result.getDevice() != null && result.getDevice().getName() != null && !bledevices.contains(result.getDevice())) {
                                    Log.i(TAG, "Dispositivo encontrado: " + result.getDevice().getName());
                                    adapter.add(result.getDevice().getName());
                                    bledevices.add(result.getDevice());
                                    ble_devices_list.setAdapter(adapter);
                                }
                            }
                        }

                        @Override
                        public void onScanFailed(int errorCode) {
                            Log.i(TAG, "SCAN FAILED");
                        }
                    };
        }
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            Log.i(TAG, "Permiso denegado (bluetooth scan): startScan");
            return;
        }else {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                Log.i(TAG, "Permiso denegado (coarse location): startScan");
                return;
            }
            lescanner.startScan(bleScanCallback);
            Thread t = new Thread(new Runnable() {
                public void run() {
                    try {
                        Thread.sleep(5000);
                        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                            Log.i(TAG, "Permiso denegado (bluetooth scan): stopScan");
                            return;
                        }
                        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                            Log.i(TAG, "Permiso denegado (coarse location): stopScan");
                            return;
                        }
                        lescanner.stopScan(bleScanCallback);
                        Log.i(TAG, "Escaner finalizado.");
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }

                }
            });
            t.start();
        }
    }

    public void setEvents() {
        ble_devices_list.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                ble_devices_list.setSelection(position);
                //((TextView) parentView.getChildAt(0)).setTextColor(Color.WHITE);
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });
    }

    @RequiresApi(api = Build.VERSION_CODES.N)
    public void bond(View view) {
        if (ble_devices_list.getSelectedItem() != null) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                return;
            }
            bluetoothDevice = bluetoothAdapter.getRemoteDevice(bledevices.stream().filter(x -> x.getName().equals(ble_devices_list.getSelectedItem())).findFirst().get().getAddress());
            bluetoothDevice.createBond();
            startClient();
            Thread t = new Thread(new Runnable() {
                public void run() {
                    while (bluetoothDevice != null) {
                        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                            return;
                        }else {
                            Log.i(TAG, "Bond state: " + bluetoothDevice.getBondState());
                            try {
                                Thread.sleep(1000);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
            });
            t.start();
        }
    }

    public void connectToBond() {
        mGattCallback = new BluetoothGattCallback() {

            private final String TAG = "mGattCallback";

            @Override
            public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {

                super.onConnectionStateChange(gatt, status, newState);
                Log.i(TAG, "Ha cambiado el estado de conexión:");
                Log.i(TAG, status + " " + newState);
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                        return;
                    } else {
                        runOnUiThread(new Runnable() {

                            @Override
                            public void run() {
                                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                    return;
                                } else {
                                    connection_state.setText("CONECTADO A: " + bluetoothDevice.getName());
                                    veces++;
                                    //button_state.setText("EL BOTON SE HA PULSADO " + veces + " VECES");
                                    button_state.setText("Latitud: " + currentLocation.getLatitude() + ", Longitud: "+currentLocation.getLongitude());
                                    //accion
                                }
                            }
                        });
                        mBluetoothGatt.discoverServices();
                    }
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            connection_state.setText("DESCONECTADO");
                        }
                    });

                }
            }

            @Override
            public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                //gatt.getServices().stream().forEach(x->Log.i(TAG, "Service discovered:"+x.getUuid()));

                if (status == gatt.GATT_SUCCESS) {
                    BluetoothGattService service = gatt.getService(UUID_SERVICE);
                    if (service != null) {
                        Log.i(TAG, "Service connected");
                        BluetoothGattCharacteristic characteristic = service.getCharacteristic(UUID_CHARACTERISTIC_BUTTON);
                        if (characteristic != null) {
                            Log.i(TAG, "Characteristic connected");
                            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                return;
                            }
                            gatt.setCharacteristicNotification(characteristic, true);
                            BluetoothGattDescriptor desc = characteristic.getDescriptor(UUID_DESCRIPTOR_BUTTON);
                            desc.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                            gatt.writeDescriptor(desc);

                        }
                    }
                }
            }

            @Override
            public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
                if (characteristic.getUuid().equals(UUID_CHARACTERISTIC_BUTTON)) {
                    if (characteristic.getValue()[0] == 1) {
                        Log.i(TAG, "Botón se ha pulsado");
                        runOnUiThread(new Runnable() {

                            @Override
                            public void run() {
                                button_state.setText("PULSADO");
                            }
                        });

                    } else if (characteristic.getValue()[0] == 0) {
                        Log.i(TAG, "Botón se ha levantado");
                        runOnUiThread(new Runnable() {

                            @Override
                            public void run() {
                                button_state.setText("LEVANTADO");
                            }
                        });
                    }
                }
            }
        };
        if (bluetoothAdapter.getBondedDevices().size() > 0) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                return;
            } else {
                for (BluetoothDevice d : bluetoothAdapter.getBondedDevices()) {
                    if (d.getName().equals("Nordic_JM")) {
                        bluetoothDevice = d;
                        bluetoothDevice.connectGatt(this, true, mGattCallback);
                    }
                }
            }
        }
    }
}
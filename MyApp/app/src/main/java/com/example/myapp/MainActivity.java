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
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Build;
import android.util.Log;
import android.view.*;
import android.os.Bundle;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class MainActivity<bluetoothAdapter> extends AppCompatActivity {
    private static Context context;
    PackageManager packageManager;
    private String TAG = "[Info]";
    private BluetoothManager bluetoothManager;
    private BluetoothAdapter bluetoothAdapter;
    BluetoothDevice bluetoothDevice;
    private BluetoothGatt btgatt;
    private List<BluetoothDevice> bledevices;
    private int veces = 0;
    final int LOCATION_REFRESH_TIME = 3000; // 3 seconds to update
    final int LOCATION_REFRESH_DISTANCE = 0; // 0 meters to update
    public static final UUID UUID_SERVICE = UUID.fromString("0000af00-0000-1000-8000-00805f9b34fb");
    public static final UUID UUID_CHARACTERISTIC_BUTTON = UUID.fromString("0000c001-0000-1000-8000-00805f9b34fb");
    public static final UUID UUID_CHARACTERISTIC_BATTERY = UUID.fromString("0000c002-0000-1000-8000-00805f9b34fb");
    public TextView button_state;
    public Spinner ble_devices_list;
    public TextView listen_button;
    public TextView connection_state;
    public TextView refresh_button;
    public TextView ipAddress;
    public TextView portNumber;
    public TextView battery_percent;
    BluetoothGatt mBluetoothGatt;
    BluetoothLeScanner lescanner;
    ArrayAdapter<String> adapter;
    private BluetoothGattCallback mGattCallback;
    protected LocationManager locationManager;
    protected LocationListener locationListener;
    private Location currentLocation;
    boolean listening = false;
    boolean buttonRead = false;
    List<BluetoothGattCharacteristic> charQueue = new ArrayList<>();

    @RequiresApi(api = Build.VERSION_CODES.N)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        button_state = findViewById(R.id.button_state_id);
        connection_state = findViewById(R.id.connection_status_id);
        ble_devices_list = findViewById(R.id.ble_devices_list_id);
        listen_button = findViewById(R.id.listen_button);
        refresh_button = findViewById(R.id.refresh_button);
        ipAddress = findViewById(R.id.ipAddress);
        portNumber = findViewById(R.id.portNumber);
        battery_percent = findViewById(R.id.battery_percent);
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
        listen_button.setEnabled(false);
        setEvents();
        Log.i(TAG, "App iniciada. Buscando dispositivos...");
        refreshList(null);
        initCallback();
    }

    public void startLocationListener() {
        locationListener = new LocationListener() {
            @Override
            public void onLocationChanged(@NonNull Location location) {
                currentLocation = location;
                Log.i(TAG, "Location updated");
            }
        };
    }

    public void initCallback() {
        try {
            mGattCallback = new BluetoothGattCallback() {
                @Override
                public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
                    super.onConnectionStateChange(gatt, status, newState);
                    Log.i(TAG, "Ha cambiado el estado de conexión:");
                    Log.i(TAG, status + " " + newState);
                    if (newState == BluetoothProfile.STATE_CONNECTED) {
                        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                            return;
                        }
                        gatt.discoverServices();
                    }
                }

                @SuppressLint("NewApi")
                @Override
                public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                    buttonRead = false;
                    //gatt.getServices().stream().forEach(x->Log.i(TAG, "Service discovered:"+x.getUuid()));
                    if (status == gatt.GATT_SUCCESS) {
                        BluetoothGattService service = gatt.getService(UUID_SERVICE);
                        if (service != null) {
                            Log.i(TAG, "Service connected");
                            BluetoothGattCharacteristic buttonChar = service.getCharacteristic(UUID_CHARACTERISTIC_BUTTON);
                            if (buttonChar != null) {
                                Log.i(TAG, "Button characteristic connected");
                                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                    return;
                                }
                                gatt.readCharacteristic(buttonChar);
                                charQueue.add(buttonChar);
                            }
                            BluetoothGattCharacteristic batteryChar = service.getCharacteristic(UUID_CHARACTERISTIC_BATTERY);
                            if (batteryChar != null) {
                                Log.i(TAG, "Battery characteristic connected");
                                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                    return;
                                }
                                charQueue.add(batteryChar);
                            }
                            readCharacteristicFromQueue(gatt);
                        }
                    }
                }

                @SuppressLint("NewApi")
                public void readCharacteristicFromQueue(BluetoothGatt gatt) {
                    if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                        return;
                    }
                    gatt.readCharacteristic(charQueue.stream().findFirst().get());
                }

                @SuppressLint("NewApi")
                @Override
                public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
                    if (characteristic.getUuid().equals(UUID_CHARACTERISTIC_BUTTON)) {
                        buttonRead = true;
                        Log.i(TAG, "Característica botón leída.");
                        if (characteristic.getValue()[0] == 1) {
                            botonPulsado();
                        }
                    }
                    if (characteristic.getUuid().equals(UUID_CHARACTERISTIC_BATTERY)) {
                        Log.i(TAG, "Característica batería leída.");
                        if (characteristic.getValue()[0] > 0) {
                            battery_percent.setText("Batería: " + characteristic.getValue()[0] + "%");
                        }
                    }

                    charQueue.remove(charQueue.stream().findFirst().get());

                    if (charQueue.size() > 0) {
                        readCharacteristicFromQueue(gatt);
                    } else {
                        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                            return;
                        }
                        gatt.disconnect();
                        Thread t2 = new Thread(new Runnable() {
                            public void run() {
                                try {
                                    Thread.sleep(5000);
                                } catch (InterruptedException e) {
                                    e.printStackTrace();
                                }
                                runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        button_state.setText("Esperando a que se pulse el botón...");
                                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                    return;
                                }
                                btgatt.connect();
                                    }
                                });
                            }
                        });
                        t2.start();
                    }
                }
            };
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

    public void botonPulsado(){
        Log.i(TAG, "Botón se ha pulsado");
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                    return;
                }
                try {
                    button_state.setText("Enviando Latitud: " + currentLocation.getLatitude() + "\n y Longitud: " + currentLocation.getLongitude());
                    Thread t1 = new Thread(new Runnable() {
                        public void run() {
                            PostBuilder postBuilder = new PostBuilder(ipAddress.getText().toString(), Integer.parseInt(portNumber.getText().toString()));
                            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                                return;
                            }
                            postBuilder.setName(bluetoothAdapter.getName());
                            postBuilder.setLocationLink("https://maps.google.com/?q=" + currentLocation.getLatitude() + "," + currentLocation.getLongitude());
                            postBuilder.sendRequest();
                        }
                    });
                    t1.start();
                }catch(Exception e){
                    Log.i(TAG, "Error al calcular la ubicación");
                }


            }
        });
    }

    public void setEvents() {
        ble_devices_list.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
                ble_devices_list.setSelection(position);
                //((TextView) parentView.getChildAt(0)).setTextColor(Color.WHITE);
                listen_button.setEnabled(true);
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {
                listen_button.setEnabled(false);
            }
        });
    }

    public void refreshList(View view) {
        adapter = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_dropdown_item);
        bledevices.clear();
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        if(bluetoothAdapter.getBondedDevices().size() > 0) {
            for (BluetoothDevice d : bluetoothAdapter.getBondedDevices()) {
                adapter.add(d.getName());
                bledevices.add(d);
            }
            ble_devices_list.setAdapter(adapter);
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.N)
    public void startBondListener(View view) {
        if(listening == false) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                return;
            }
            bluetoothDevice = bledevices.stream().filter(device -> device.getName().equals(ble_devices_list.getSelectedItem())).findFirst().get();
            btgatt = bluetoothDevice.connectGatt(this, true, mGattCallback);
            connection_state.setText("Escuchando a: " + bluetoothDevice.getName());
            ble_devices_list.setEnabled(false);
            refresh_button.setEnabled(false);
            ipAddress.setEnabled(false);
            portNumber.setEnabled(false);
            listen_button.setText("Parar");
            listening = true;
        }else{
            btgatt.disconnect();
            btgatt.close();
            connection_state.setText("No se está escuchando");
            ble_devices_list.setEnabled(true);
            refresh_button.setEnabled(true);
            ipAddress.setEnabled(true);
            portNumber.setEnabled(true);
            listen_button.setText("Escuchar");
            listening = false;
        }
    }
}
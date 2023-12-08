package com.a3.park_ir

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.ListView
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import com.a3.park_ir.DTO.DataClass
import com.harrysoft.androidbluetoothserial.BluetoothManager
import com.harrysoft.androidbluetoothserial.BluetoothSerialDevice
import com.harrysoft.androidbluetoothserial.SimpleBluetoothDeviceInterface
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.schedulers.Schedulers

class BluetoothActivity : AppCompatActivity() {
    private lateinit var connectBluetoothButton : Button
    private var REQUEST_ENABLE_BT = 0;
    private var REQUEST_BLUETOOTH_PERMISSION = 1
    private val discoveredBluetooth = ArrayList<DataClass.Bluetooth>()
    private val discoveredDevices = mutableListOf<String>() // List to store discovered devices
    private val discoveredMac = mutableListOf<String>() // List to store discovered devices
    private lateinit var deviceInterface: SimpleBluetoothDeviceInterface
    private lateinit var pairedDevices : Collection<BluetoothDevice>;
    private lateinit var listView: ListView
    private lateinit var bluetoothManager : BluetoothManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_bluetooth)
        supportActionBar?.hide()
        connectBluetoothButton = findViewById(R.id.connect_bluetooth_button)
        connectBluetoothButton.setOnClickListener {bluetoothConnect()}
    }

    private fun bluetoothConnect() {
        bluetoothManager = BluetoothManager.getInstance()
        if (bluetoothManager == null) {
            Toast.makeText(this, "Bluetooth not available.", Toast.LENGTH_LONG)
                .show()
            finish()
        }

        val permissionConnect = ActivityCompat.checkSelfPermission(
            this,
            Manifest.permission.BLUETOOTH_CONNECT
        ) != PackageManager.PERMISSION_GRANTED
        val permissionScan = ActivityCompat.checkSelfPermission(
            this,
            Manifest.permission.BLUETOOTH_SCAN
        ) != PackageManager.PERMISSION_GRANTED

        if (!permissionConnect || !permissionScan) {
            ActivityCompat.requestPermissions(
                this,
                arrayOf(Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN),
                REQUEST_BLUETOOTH_PERMISSION
            )
        }
        pairedDevices = bluetoothManager.pairedDevicesList
        val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
        startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT)
        Toast.makeText(this, "Bluetooth already active", Toast.LENGTH_SHORT).show()
    }
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == Activity.RESULT_OK) {
                showBluetoothDevicesDialog()
            } else {
                Toast.makeText(this, "User declined bluetooth activation", Toast.LENGTH_SHORT).show()
            }
        }
    }
    private fun connectDevice(mac: String) {
        bluetoothManager.openSerialDevice(mac)
            .subscribeOn(Schedulers.io())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe({ connectedDevice: BluetoothSerialDevice -> onConnected(connectedDevice) }) { error: Throwable ->
                onError(
                    error
                )
            }
    }
    private fun onConnected(connectedDevice: BluetoothSerialDevice) {
        deviceInterface = connectedDevice.toSimpleDeviceInterface()

        // Listen to bluetooth events
        deviceInterface.setListeners(
            SimpleBluetoothDeviceInterface.OnMessageReceivedListener { message: String -> onMessageReceived(message) },
            SimpleBluetoothDeviceInterface.OnMessageSentListener { message: String -> onMessageSent(message) },
            SimpleBluetoothDeviceInterface.OnErrorListener { error: Throwable ->
                onError (
                    error
                )
            })

        // Let's send a message:
        deviceInterface.sendMessage("958F6538")
    }
    private fun onMessageSent(message: String) {
        // We sent a message! Handle it here.
        Toast.makeText(this@BluetoothActivity, "Sent a message! Message was: $message", Toast.LENGTH_LONG)
            .show() // Replace context with your context instance.
    }

    private fun onMessageReceived(message: String) {
        // We received a message! Handle it here.
        Toast.makeText(this@BluetoothActivity, "Received a message! Message was: $message", Toast.LENGTH_LONG)
            .show() // Replace context with your context instance.
    }

    private fun onError(error: Throwable) {
        Toast.makeText(this@BluetoothActivity, "Can't connect to the device", Toast.LENGTH_SHORT).show()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == REQUEST_BLUETOOTH_PERMISSION) {
            // Check if the permission has been granted
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Bluetooth Permission Granted", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Bluetooth Permission not granted", Toast.LENGTH_SHORT).show()
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun showBluetoothDevicesDialog() {
        val view = this.layoutInflater.inflate(R.layout.bluetooth_modal, null)
        val alertDialog = AlertDialog.Builder(this)
            .setTitle("Discovered Bluetooth Devices")
            .setView(view)
            .setPositiveButton("OK") { dialog, which ->
                dialog.dismiss()
            }
            .setCancelable(false)
            .create()
        listView = view.findViewById(R.id.list_bluetooth)

        discoveredDevices.clear()
        discoveredMac.clear()

        pairedDevices.forEach {
//            discoveredDevices.add(it.name)
//            discoveredMac.add(it.address)
            discoveredBluetooth.add(DataClass.Bluetooth(it.name, it.address))
        }

        val arrayAdapter = CustomAdapter(this, discoveredDevices)
        listView.adapter = arrayAdapter
        listView.setOnItemClickListener { parent, view, position, id ->
            // Handle item click here
            val selectedItem = discoveredMac[position]
            Toast.makeText(this@BluetoothActivity, selectedItem, Toast.LENGTH_SHORT).show()
            connectDevice(selectedItem)
        }
        alertDialog.show()
    }

    class CustomAdapter(context: Context, objects: List<DataClass.Bluetooth>) : ArrayAdapter<DataClass.Bluetooth>(context, 0, objects) {
        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            var itemView = convertView
            if (itemView == null) {
                itemView = LayoutInflater.from(context).inflate(R.layout.bluetooth_modal, parent, false)
            }

            // Get the current item from the data list
            val currentItem = getItem(position)

            // Customize the item view
            val textViewId = itemView?.findViewById<TextView>(R.id.bluetoothID)
            textViewId?.text = currentItem?.Name
            val textViewMac = itemView?.findViewById<TextView>(R.id.bluetoothMac)
            textViewMac?.text = currentItem?.Mac

            return itemView!!
        }
    }
}
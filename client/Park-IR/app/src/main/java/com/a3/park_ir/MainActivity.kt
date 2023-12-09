package com.a3.park_ir

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.os.Bundle
import android.preference.PreferenceManager
import android.util.Log
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.ListView
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import com.a3.park_ir.DTO.DataClass
import com.a3.park_ir.request.BaseApiServices
import com.a3.park_ir.request.UtilsApi
import com.harrysoft.androidbluetoothserial.BluetoothManager
import com.harrysoft.androidbluetoothserial.BluetoothSerialDevice
import com.harrysoft.androidbluetoothserial.SimpleBluetoothDeviceInterface
import com.harrysoft.androidbluetoothserial.SimpleBluetoothDeviceInterface.OnMessageReceivedListener
import com.harrysoft.androidbluetoothserial.SimpleBluetoothDeviceInterface.OnMessageSentListener
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.schedulers.Schedulers
import retrofit2.Call
import retrofit2.Callback
import retrofit2.Response

class MainActivity : AppCompatActivity() {
    private lateinit var connectBluetoothButton : Button
    private var REQUEST_ENABLE_BT = 0;
    private var REQUEST_BLUETOOTH_PERMISSION = 1
    private val discoveredDevices = mutableListOf<String>() // List to store discovered devices
    private val discoveredMac = mutableListOf<String>() // List to store discovered devices
    private lateinit var deviceInterface: SimpleBluetoothDeviceInterface
    private lateinit var pairedDevices : Collection<BluetoothDevice>;
    private lateinit var listView: ListView
    private lateinit var bluetoothManager : BluetoothManager
    private lateinit var username : TextView
    private lateinit var vid : TextView
    private lateinit var isSubscibed : TextView
    private lateinit var mApiService: BaseApiServices
    private lateinit var preferensi: SharedPreferences
    private var mContext: Context? = null
    private var accessToken: String = ""
    private var refreshToken: String = ""
    private var name: String = " "


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        supportActionBar?.hide()

        mContext = this
        this.preferensi = PreferenceManager.getDefaultSharedPreferences(this)
        this.accessToken = this.preferensi.getString("token_akses", "").toString()
        this.refreshToken = this.preferensi.getString("token_refresh", "").toString()
        this.name = this.preferensi.getString("username", "").toString()

        mApiService = UtilsApi().getApiService(this)

        connectBluetoothButton = this.findViewById(R.id.connect_bluetooth_button)
        username = this.findViewById(R.id.username)
        vid = this.findViewById(R.id.the_vid)
        isSubscibed = this.findViewById(R.id.text_subscribed)

        connectBluetoothButton.setOnClickListener {
            startActivity(Intent(this@MainActivity, BluetoothActivity::class.java))
        }
        getPlateData()
    }
    protected fun getPlateData() {
        mApiService.getPlate("Bearer "+accessToken).enqueue(object: Callback<DataClass.PlateResponse> {
            override fun onResponse(
                call: Call<DataClass.PlateResponse>,
                response: Response<DataClass.PlateResponse>
            ) {
                if (!response.isSuccessful) {
                    Toast.makeText(mContext, "notSuccessful "+response.message(), Toast.LENGTH_SHORT).show()
                    return
                }
                Toast.makeText(mContext, "Success "+response.body(), Toast.LENGTH_SHORT).show()
                username.setText(name)
                isSubscibed.setText("Subscriber")
                vid.setText(response.body()?.v_id ?: "null")
            }

            override fun onFailure(call: Call<DataClass.PlateResponse>, t: Throwable) {
                Log.d("", t.message.toString())
                Toast.makeText(mContext, "OnFailure "+ t.message, Toast.LENGTH_SHORT).show()
                return
            }

        })
    }
}
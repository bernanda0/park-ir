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
import android.view.View
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.ListView
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.core.app.ActivityCompat
import com.a3.park_ir.DTO.DataClass
import com.a3.park_ir.request.BaseApiServices
import com.a3.park_ir.request.UtilsApi
import com.chaos.view.PinView
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
    private lateinit var username : TextView
    private lateinit var vid : TextView
    private lateinit var isSubscibed : TextView
    private lateinit var balance: TextView
    private lateinit var mApiService: BaseApiServices
    private lateinit var preferensi: SharedPreferences
    private lateinit var vid_layout: ConstraintLayout
    private lateinit var reg_plate_layout: ConstraintLayout
    private lateinit var input_plate : PinView
    private lateinit var goTopUpButton : Button
    private lateinit var topUpAmount : EditText
    private var mContext: Context? = null
    private var accessToken: String = ""
    private var refreshToken: String = ""
    private var name: String = " "
    private lateinit var topUpButton: Button
    private lateinit var regPlateButton: Button


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
        regPlateButton = this.findViewById(R.id.register_plate)
        input_plate = this.findViewById(R.id.plate_pinview)
        topUpButton = this.findViewById(R.id.topUp)
        vid_layout = this.findViewById(R.id.vid_placeholder)
        reg_plate_layout = this.findViewById(R.id.reg_plate_placeholder)
        balance = this.findViewById(R.id.total_balance)
        username = this.findViewById(R.id.username)
        vid = this.findViewById(R.id.the_vid)
        isSubscibed = this.findViewById(R.id.text_subscribed)

        username.setText(name)
        connectBluetoothButton.setOnClickListener {
            startActivity(Intent(this@MainActivity, BluetoothActivity::class.java))
        }
        regPlateButton.setOnClickListener {
            handlePlateReg()
        }
        topUpButton.setOnClickListener {
            topUpDialog()
        }
        getPlateData()
        getUserBalance()
    }

    protected fun handlePlateReg() {
        mApiService.createVID("Bearer "+accessToken, input_plate.text.toString()).enqueue(object : Callback<Object>{
            override fun onResponse(call: Call<Object>, response: Response<Object>) {
                if (!response.isSuccessful) {
                    Toast.makeText(mContext, "Cannot reg the plate. Make sure the plate is valid!", Toast.LENGTH_SHORT).show()
                    return
                }
                finish();
                startActivity(getIntent());
            }

            override fun onFailure(call: Call<Object>, t: Throwable) {
                TODO("Not yet implemented")
            }

        })
    }
    protected fun getUserBalance() {
        mApiService.getBalance("Bearer "+accessToken).enqueue(object: Callback<Int> {
            override fun onResponse(call: Call<Int>, response: Response<Int>) {
                if (!response.isSuccessful) {
                    Toast.makeText(mContext, "Cannot get user balance "+response.message(), Toast.LENGTH_SHORT).show()
                    return
                }
                balance.setText("Rp"+response.body())

            }

            override fun onFailure(call: Call<Int>, t: Throwable) {
                TODO("Not yet implemented")
            }

        })
    }
    protected fun getPlateData() {
        mApiService.getPlate("Bearer "+accessToken).enqueue(object: Callback<DataClass.PlateResponse> {
            override fun onResponse(
                call: Call<DataClass.PlateResponse>,
                response: Response<DataClass.PlateResponse>
            ) {
                if (!response.isSuccessful) {
                    vid_layout.visibility = View.GONE
                    reg_plate_layout.visibility = View.VISIBLE
                    regPlateButton.visibility = View.VISIBLE
                    connectBluetoothButton.visibility = View.GONE
                    return
                }
                vid_layout.visibility = View.VISIBLE
                reg_plate_layout.visibility = View.GONE
                regPlateButton.visibility = View.GONE
                connectBluetoothButton.visibility = View.VISIBLE
                isSubscibed.setText("Subscribed")
                vid.setText(response.body()?.v_id ?: "null")
                val preferensi: SharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext)
                val editor: SharedPreferences.Editor = preferensi.edit()
                editor.putString("vid", response.body()?.v_id)
                editor.apply()
            }

            override fun onFailure(call: Call<DataClass.PlateResponse>, t: Throwable) {
                Log.d("", t.message.toString())
                Toast.makeText(mContext, "OnFailure "+ t.message, Toast.LENGTH_SHORT).show()
                return
            }

        })
    }
    private fun topUpDialog() {
        val view = this.layoutInflater.inflate(R.layout.topup_modal, null)
        val alertDialog = AlertDialog.Builder(this)
            .setView(view)
            .setPositiveButton("OK") { dialog, which ->
                dialog.dismiss()
            }
            .setCancelable(false)
            .create()
        alertDialog.window?.setBackgroundDrawableResource(R.drawable.rounded_edge)
        topUpAmount = view.findViewById(R.id.amount)
        goTopUpButton = view.findViewById(R.id.doTopUp)
        goTopUpButton.setOnClickListener { handleTopUp() }
        alertDialog.show()
    }

    protected fun handleTopUp() {
        mApiService.topUp("Bearer "+accessToken, Integer.parseInt(topUpAmount.text.toString())).enqueue(object : Callback<DataClass.Wallet>{
            override fun onResponse(
                call: Call<DataClass.Wallet>,
                response: Response<DataClass.Wallet>
            ) {
                if (!response.isSuccessful) {
                   Toast.makeText(mContext, "Cannot topUp at this moment!", Toast.LENGTH_SHORT).show()
                    return
                }

                val w = response.body()
                Toast.makeText(mContext, "Top Up success!", Toast.LENGTH_SHORT).show()
                topUpAmount.setText("")
                balance.setText(""+w?.balance?.Int32)
            }

            override fun onFailure(call: Call<DataClass.Wallet>, t: Throwable) {
                TODO("Not yet implemented")
            }

        })
    }
}
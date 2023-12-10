package com.a3.park_ir

import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import android.preference.PreferenceManager
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.a3.park_ir.DTO.DataClass
import com.a3.park_ir.request.BaseApiServices
import com.a3.park_ir.request.UtilsApi
import retrofit2.Call
import retrofit2.Callback
import retrofit2.Response

class LoginActivity : AppCompatActivity() {
    private lateinit var mApiService: BaseApiServices
    private var mContext: Context? = null
    private lateinit var email: EditText
    private lateinit var password:EditText
    private lateinit var registerNow: TextView
    private lateinit var loginButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_login)

        supportActionBar!!.hide()

//        Load the components to the variables
        email = findViewById<EditText>(R.id.email)
        password = findViewById<EditText>(R.id.password)
        registerNow = findViewById<TextView>(R.id.register_now)
        loginButton = findViewById<Button>(R.id.login_button)

//        other variable
        mContext = this
        mApiService = UtilsApi().getApiService(this)

//        adding listener
        registerNow.setOnClickListener(View.OnClickListener { v: View? ->
            startActivity(Intent(this, RegisterActivity::class.java))
        })

        loginButton.setOnClickListener(View.OnClickListener { v: View? -> handleLogin() })
    }

    protected fun handleLogin() {
        // handling empty field
        val emailS: String = email.getText().toString()
        val passwordS: String = password.getText().toString()
        if (emailS.isEmpty() || passwordS.isEmpty()) {
            Toast.makeText(this@LoginActivity, "Field cannot be empty!", Toast.LENGTH_SHORT).show()
            return
        }
//        val emailS = "halo123@gmail.com"
//        val passwordS = "halo123"

        // case if not empty then make a request
        mApiService.login(emailS, passwordS).enqueue(object : Callback<DataClass.LoginResponse> {
            override fun onResponse(
                call: Call<DataClass.LoginResponse>,
                response: Response<DataClass.LoginResponse>
            ) {
                // handle the potential 4xx & 5xx error
                if (!response.isSuccessful()) {
                    Toast.makeText(this@LoginActivity, ""+response.code(), Toast.LENGTH_SHORT).show()
                    return
                }

                // here always response code 200
                val res: DataClass.LoginResponse? = response.body()
                if (res != null) {
                    Toast.makeText(this@LoginActivity, "Welcome "+res.username, Toast.LENGTH_SHORT).show()
                    email.setText("")
                    password.setText("")

                    val preferensi: SharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext)
                    val editor: SharedPreferences.Editor = preferensi.edit()
                    editor.putString("token_akses", res.access_token)
                    editor.putString("token_refresh", res.refresh_token)
                    editor.putString("username", res.username)
                    editor.apply()
                    startActivity(Intent(this@LoginActivity, MainActivity::class.java))
                    return
                }
            }

            // in case client failed to make a request
            override fun onFailure(call: Call<DataClass.LoginResponse>, t: Throwable) {
                Toast.makeText(mContext, " "+t.message, Toast.LENGTH_SHORT).show()
            }
        })
    }
}
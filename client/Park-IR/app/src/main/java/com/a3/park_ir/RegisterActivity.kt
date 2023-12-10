package com.a3.park_ir

import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import android.preference.PreferenceManager
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.a3.park_ir.DTO.DataClass
import com.a3.park_ir.request.BaseApiServices
import com.a3.park_ir.request.UtilsApi
import retrofit2.Call
import retrofit2.Callback
import retrofit2.Response

class RegisterActivity : AppCompatActivity() {
    private lateinit var mApiService: BaseApiServices
    private var mContext: Context? = null
    private lateinit var name: EditText
    private lateinit var email: EditText
    private lateinit var password: EditText
    private lateinit var registerButton: Button
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_register)

        supportActionBar!!.hide()

        mContext = this
        mApiService = UtilsApi().getApiService(this)

        // sesuaikan dengan ID yang kalian buat di layout
        name = findViewById(R.id.username)
        email = findViewById(R.id.email)
        password = findViewById(R.id.password)
        registerButton = findViewById(R.id.button_register)

        registerButton.setOnClickListener(View.OnClickListener { v: View? -> handleRegister() })
    }


    protected fun handleRegister() {
        val nameS = name.text.toString()
        val emailS = email.text.toString()
        val passwordS = password.text.toString()

        if (nameS.isEmpty() || emailS.isEmpty() || passwordS.isEmpty()) {
            Toast.makeText(mContext, "Field cannot be empty", Toast.LENGTH_SHORT).show()
            return
        }

        // case if not empty then make a request
        mApiService.register(nameS, emailS, passwordS).enqueue(object : Callback<DataClass.LoginResponse> {
            override fun onResponse(
                call: Call<DataClass.LoginResponse>,
                response: Response<DataClass.LoginResponse>
            ) {
                // handle the potential 4xx & 5xx error
                if (!response.isSuccessful()) {
                    Toast.makeText(this@RegisterActivity, ""+response.code(), Toast.LENGTH_SHORT).show()
                    return
                }

                // here always get response code 200
                if (response.body() != null) {
                    Toast.makeText(this@RegisterActivity, "Account successfully created, please login!", Toast.LENGTH_SHORT).show()
                    startActivity(Intent(this@RegisterActivity, LoginActivity::class.java))
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
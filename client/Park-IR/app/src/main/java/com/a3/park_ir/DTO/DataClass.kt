package com.a3.park_ir.DTO

class DataClass {
    data class LoginResponse (
        val session_id: String,
        val access_token: String,
        val access_token_expire: String,
        val refresh_token: String,
        val refresh_token_expire: String,
        val user_id: String,
        val username: String
    )

    data class PlateResponse(
        val v_id : String,
        val is_subscribe : NullBoolean
    )
    data class NullBoolean (
        val Bool: Boolean,
        val Valid: Boolean
    )

    data class Bluetooth (
        val Name : String,
        val Mac : String
    )
}

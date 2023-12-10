package token

import (
	"errors"
	"time"

	jwt "github.com/golang-jwt/jwt/v5"
)

const minSecretKeyLen = 32

type JwtMaker struct {
	secret_key []byte
}

func NewJwtMaker(secret_key string) (Maker, error) {
	if len(secret_key) < minSecretKeyLen {
		return nil, errors.New("invalid secret key length")
	}
	return &JwtMaker{secret_key: []byte(secret_key)}, nil
}

func (j *JwtMaker) GenerateToken(account_id string, username string, duration time.Duration) (string, *Payload, error) {
	payload := NewPayload(account_id, username, duration)
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, payload)

	tokenString, err := token.SignedString(j.secret_key)
	if err != nil {
		return "", nil, err
	}

	return tokenString, payload, nil
}

func (j *JwtMaker) VerifyToken(tokenString string) (*Payload, error) {
	// Check if the signin method is not changed
	checkSign := func(token *jwt.Token) (interface{}, error) {
		_, ok := token.Method.(*jwt.SigningMethodHMAC)

		if !ok {
			return nil, ErrInvalidToken
		}

		return j.secret_key, nil
	}

	token, err := jwt.ParseWithClaims(tokenString, &Payload{}, checkSign)
	if err != nil {
		return nil, err
	}

	if claims, ok := token.Claims.(*Payload); ok && token.Valid {
		return claims, nil
	} else {
		return nil, ErrInvalidToken
	}
}

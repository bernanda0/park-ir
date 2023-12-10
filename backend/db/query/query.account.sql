-- name: GetAccount :one
SELECT * FROM account
WHERE account_id = $1 LIMIT 1;

-- name: GetAccountbyEmail :one
SELECT * FROM account
WHERE email = $1 LIMIT 1;

-- name: CreateAccount :one
INSERT INTO account (
  username, email, password_hash
) VALUES (
  $1, $2, $3
)
RETURNING *;

-- name: GetEmail :one
SELECT email FROM account 
WHERE email = $1;

-- name: GetHashedPassword :one
SELECT password_hash FROM account 
WHERE email = $1;

-- name: AccountSubscribe :one
UPDATE account SET is_subscribe = true WHERE account_id = $1
RETURNING is_subscribe;
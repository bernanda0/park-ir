-- name: GenerateLogs :one
INSERT INTO logs (account_id, v_id, location, ip_address)
VALUES ($1, $2, $3, $4)
RETURNING *;

-- name: GetLogs :many
SELECT event_id, transaction_time, location 
FROM logs
WHERE account_id = $1;
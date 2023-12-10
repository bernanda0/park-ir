// Code generated by sqlc. DO NOT EDIT.
// versions:
//   sqlc v1.20.0
// source: query.logs.sql

package sqlc

import (
	"context"
	"database/sql"
)

const generateLogs = `-- name: GenerateLogs :one
INSERT INTO logs (account_id, v_id, location, ip_address)
VALUES ($1, $2, $3, $4)
RETURNING event_id, transaction_time, account_id, v_id, location, ip_address
`

type GenerateLogsParams struct {
	AccountID sql.NullString `json:"account_id"`
	VID       sql.NullString `json:"v_id"`
	Location  sql.NullString `json:"location"`
	IpAddress sql.NullString `json:"ip_address"`
}

func (q *Queries) GenerateLogs(ctx context.Context, arg GenerateLogsParams) (Log, error) {
	row := q.db.QueryRowContext(ctx, generateLogs,
		arg.AccountID,
		arg.VID,
		arg.Location,
		arg.IpAddress,
	)
	var i Log
	err := row.Scan(
		&i.EventID,
		&i.TransactionTime,
		&i.AccountID,
		&i.VID,
		&i.Location,
		&i.IpAddress,
	)
	return i, err
}

const getLogs = `-- name: GetLogs :many
SELECT event_id, transaction_time, location 
FROM logs
WHERE account_id = $1
`

type GetLogsRow struct {
	EventID         int32          `json:"event_id"`
	TransactionTime sql.NullTime   `json:"transaction_time"`
	Location        sql.NullString `json:"location"`
}

func (q *Queries) GetLogs(ctx context.Context, accountID sql.NullString) ([]GetLogsRow, error) {
	rows, err := q.db.QueryContext(ctx, getLogs, accountID)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	var items []GetLogsRow
	for rows.Next() {
		var i GetLogsRow
		if err := rows.Scan(&i.EventID, &i.TransactionTime, &i.Location); err != nil {
			return nil, err
		}
		items = append(items, i)
	}
	if err := rows.Close(); err != nil {
		return nil, err
	}
	if err := rows.Err(); err != nil {
		return nil, err
	}
	return items, nil
}
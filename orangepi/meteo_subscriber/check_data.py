#!/usr/bin/env python3
"""
Quick script to check what data is in the database
"""

import sqlite3
import os
from datetime import datetime
from dotenv import load_dotenv

load_dotenv()
SQLITE_DB = os.getenv("SQLITE_DB", "environment_data.db")

conn = sqlite3.connect(SQLITE_DB)
cursor = conn.cursor()

# Get total count
cursor.execute("SELECT COUNT(*) FROM measurements")
total = cursor.fetchone()[0]
print(f"Total records: {total}")

# Get date range
cursor.execute("SELECT MIN(timestamp_server), MAX(timestamp_server) FROM measurements")
min_ts, max_ts = cursor.fetchone()

if min_ts and max_ts:
    min_date = datetime.fromtimestamp(min_ts)
    max_date = datetime.fromtimestamp(max_ts)
    hours_span = (max_ts - min_ts) / 3600

    print(f"\nData range:")
    print(f"  Oldest: {min_date} ({min_ts})")
    print(f"  Newest: {max_date} ({max_ts})")
    print(f"  Span: {hours_span:.1f} hours ({hours_span / 24:.1f} days)")

    # Count by device
    print(f"\nRecords per device:")
    cursor.execute("""
        SELECT device_id, COUNT(*), MIN(timestamp_server), MAX(timestamp_server)
        FROM measurements 
        GROUP BY device_id
    """)
    for row in cursor.fetchall():
        device, count, min_ts, max_ts = row
        min_date = datetime.fromtimestamp(min_ts)
        max_date = datetime.fromtimestamp(max_ts)
        print(f"  {device}: {count} records from {min_date} to {max_date}")
else:
    print("No data in database")

conn.close()

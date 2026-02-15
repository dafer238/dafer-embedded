import os
import sqlite3
import time
from datetime import datetime, timedelta
from typing import List, Optional, Dict, Any
from pathlib import Path

from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from fastapi.requests import Request
from dotenv import load_dotenv
import logging

# Load environment variables
load_dotenv()

SQLITE_DB = os.getenv("SQLITE_DB", "environment_data.db")
LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO").upper()

logging.basicConfig(
    level=getattr(logging, LOG_LEVEL, logging.INFO),
    format="%(asctime)s [%(levelname)s] %(message)s",
)

app = FastAPI(title="Meteo Dashboard", version="1.0.0")

# Setup templates and static files
BASE_DIR = Path(__file__).resolve().parent
templates = Jinja2Templates(directory=str(BASE_DIR / "templates"))

# Create static directory if it doesn't exist
static_dir = BASE_DIR / "static"
static_dir.mkdir(exist_ok=True)
app.mount("/static", StaticFiles(directory=str(static_dir)), name="static")


def get_db_connection():
    """Create a database connection."""
    conn = sqlite3.connect(SQLITE_DB, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    return conn


@app.get("/", response_class=HTMLResponse)
async def dashboard(request: Request):
    """Main dashboard page."""
    return templates.TemplateResponse("dashboard.html", {"request": request})


@app.get("/api/devices/status")
async def get_devices_status():
    """Get currently connected devices and their status."""
    conn = get_db_connection()
    cursor = conn.cursor()

    # Get devices that have sent data in the last 5 minutes
    five_minutes_ago = int(time.time()) - 300

    query = """
        SELECT 
            device_id,
            MAX(timestamp_server) as last_seen,
            COUNT(*) as message_count,
            firmware_version,
            rssi
        FROM measurements
        WHERE timestamp_server > ?
        GROUP BY device_id
        ORDER BY last_seen DESC
    """

    cursor.execute(query, (five_minutes_ago,))
    rows = cursor.fetchall()

    devices = []
    current_time = int(time.time())

    for row in rows:
        last_seen = row["last_seen"]
        seconds_ago = current_time - last_seen

        # Determine status
        if seconds_ago < 60:
            status = "online"
        elif seconds_ago < 300:
            status = "warning"
        else:
            status = "offline"

        devices.append(
            {
                "device_id": row["device_id"],
                "status": status,
                "last_seen": last_seen,
                "last_seen_ago": seconds_ago,
                "message_count": row["message_count"],
                "firmware_version": row["firmware_version"],
                "rssi": row["rssi"],
            }
        )

    conn.close()
    return {"devices": devices, "timestamp": current_time}


@app.get("/api/devices/{device_id}/latest")
async def get_device_latest_data(device_id: str):
    """Get latest data from a specific device."""
    conn = get_db_connection()
    cursor = conn.cursor()

    query = """
        SELECT *
        FROM measurements
        WHERE device_id = ?
        ORDER BY timestamp_server DESC
        LIMIT 1
    """

    cursor.execute(query, (device_id,))
    row = cursor.fetchone()
    conn.close()

    if not row:
        raise HTTPException(status_code=404, detail=f"Device {device_id} not found")

    return dict(row)


@app.get("/api/data/latest")
async def get_latest_data(limit: int = Query(default=10, ge=1, le=100)):
    """Get latest measurements from all devices."""
    conn = get_db_connection()
    cursor = conn.cursor()

    query = """
        SELECT *
        FROM measurements
        ORDER BY timestamp_server DESC
        LIMIT ?
    """

    cursor.execute(query, (limit,))
    rows = cursor.fetchall()
    conn.close()

    return {"data": [dict(row) for row in rows]}


@app.get("/api/data/history")
async def get_historical_data(
    device_id: Optional[str] = None,
    hours: int = Query(default=24, ge=1, le=168),
    limit: int = Query(default=1000, ge=1, le=10000),
):
    """Get historical data with optional device filter."""
    conn = get_db_connection()
    cursor = conn.cursor()

    current_time = int(time.time())
    time_threshold = current_time - (hours * 3600)

    logging.info(f"History request: hours={hours}, device_id={device_id}, limit={limit}")
    logging.info(
        f"Current time: {current_time}, Time threshold: {time_threshold}, Delta: {hours * 3600}s"
    )
    logging.info(f"Time threshold as datetime: {datetime.fromtimestamp(time_threshold)}")

    if device_id:
        query = """
            SELECT *
            FROM measurements
            WHERE device_id = ? AND timestamp_server > ?
            ORDER BY timestamp_server DESC
            LIMIT ?
        """
        cursor.execute(query, (device_id, time_threshold, limit))
    else:
        query = """
            SELECT *
            FROM measurements
            WHERE timestamp_server > ?
            ORDER BY timestamp_server DESC
            LIMIT ?
        """
        cursor.execute(query, (time_threshold, limit))

    rows = cursor.fetchall()

    if rows:
        oldest = rows[-1]["timestamp_server"]
        newest = rows[0]["timestamp_server"]
        logging.info(
            f"Returning {len(rows)} rows from {datetime.fromtimestamp(oldest)} to {datetime.fromtimestamp(newest)}"
        )
    else:
        logging.info(f"No data found after timestamp {time_threshold}")

    conn.close()

    return {"data": [dict(row) for row in rows], "hours": hours}


@app.get("/api/data/aggregated")
async def get_aggregated_data(
    device_id: Optional[str] = None,
    hours: int = Query(default=24, ge=1, le=168),
    interval_minutes: int = Query(default=60, ge=5, le=1440),
):
    """Get aggregated data by time intervals."""
    conn = get_db_connection()
    cursor = conn.cursor()

    time_threshold = int(time.time()) - (hours * 3600)
    interval_seconds = interval_minutes * 60

    if device_id:
        query = """
            SELECT 
                device_id,
                (timestamp_server / ?) * ? as timestamp_server,
                AVG(dht22_temperature_c) as dht22_temperature_c,
                AVG(dht22_humidity_percent) as dht22_humidity_percent,
                AVG(aht20_temperature_c) as aht20_temperature_c,
                AVG(aht20_humidity_percent) as aht20_humidity_percent,
                AVG(bmp280_temperature_c) as bmp280_temperature_c,
                AVG(bmp280_pressure_pa) as bmp280_pressure_pa,
                AVG(altitude_m) as altitude_m,
                AVG(free_heap) as free_heap,
                AVG(rssi) as rssi,
                COUNT(*) as sample_count
            FROM measurements
            WHERE device_id = ? AND timestamp_server > ?
            GROUP BY device_id, timestamp_server
            ORDER BY timestamp_server ASC
        """
        cursor.execute(query, (interval_seconds, interval_seconds, device_id, time_threshold))
    else:
        query = """
            SELECT 
                device_id,
                (timestamp_server / ?) * ? as timestamp_server,
                AVG(dht22_temperature_c) as dht22_temperature_c,
                AVG(dht22_humidity_percent) as dht22_humidity_percent,
                AVG(aht20_temperature_c) as aht20_temperature_c,
                AVG(aht20_humidity_percent) as aht20_humidity_percent,
                AVG(bmp280_temperature_c) as bmp280_temperature_c,
                AVG(bmp280_pressure_pa) as bmp280_pressure_pa,
                AVG(altitude_m) as altitude_m,
                AVG(free_heap) as free_heap,
                AVG(rssi) as rssi,
                COUNT(*) as sample_count
            FROM measurements
            WHERE timestamp_server > ?
            GROUP BY device_id, timestamp_server
            ORDER BY timestamp_server ASC
        """
        cursor.execute(query, (interval_seconds, interval_seconds, time_threshold))

    rows = cursor.fetchall()
    conn.close()

    return {"data": [dict(row) for row in rows], "interval_minutes": interval_minutes}


@app.get("/api/health")
async def health_check():
    """Health check endpoint."""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()

        # Check database
        cursor.execute("SELECT COUNT(*) as count FROM measurements")
        total_records = cursor.fetchone()["count"]

        # Check recent activity
        five_minutes_ago = int(time.time()) - 300
        cursor.execute(
            "SELECT COUNT(DISTINCT device_id) as active_devices FROM measurements WHERE timestamp_server > ?",
            (five_minutes_ago,),
        )
        active_devices = cursor.fetchone()["active_devices"]

        # Get database size
        cursor.execute(
            "SELECT page_count * page_size as size FROM pragma_page_count(), pragma_page_size()"
        )
        db_size = cursor.fetchone()["size"]

        conn.close()

        return {
            "status": "healthy",
            "timestamp": int(time.time()),
            "database": {
                "total_records": total_records,
                "size_bytes": db_size,
                "size_mb": round(db_size / (1024 * 1024), 2),
            },
            "devices": {"active_last_5min": active_devices},
        }
    except Exception as e:
        logging.error(f"Health check failed: {e}")
        return JSONResponse(status_code=503, content={"status": "unhealthy", "error": str(e)})


@app.get("/api/stats")
async def get_statistics():
    """Get overall statistics."""
    conn = get_db_connection()
    cursor = conn.cursor()

    # Get overall stats
    cursor.execute("""
        SELECT 
            COUNT(*) as total_measurements,
            COUNT(DISTINCT device_id) as total_devices,
            MIN(timestamp_server) as first_measurement,
            MAX(timestamp_server) as last_measurement
        FROM measurements
    """)
    stats = dict(cursor.fetchone())

    # Get per-device stats
    cursor.execute("""
        SELECT 
            device_id,
            COUNT(*) as measurement_count,
            MIN(timestamp_server) as first_seen,
            MAX(timestamp_server) as last_seen,
            AVG(dht22_temperature_c) as avg_temp_dht22,
            AVG(aht20_temperature_c) as avg_temp_aht20,
            AVG(bmp280_temperature_c) as avg_temp_bmp280,
            AVG(dht22_humidity_percent) as avg_humidity_dht22,
            AVG(aht20_humidity_percent) as avg_humidity_aht20,
            AVG(bmp280_pressure_pa) as avg_pressure
        FROM measurements
        GROUP BY device_id
    """)
    device_stats = [dict(row) for row in cursor.fetchall()]

    conn.close()

    return {"overall": stats, "devices": device_stats}


@app.get("/api/query/test")
async def test_query_endpoint():
    """Test if query endpoint is accessible."""
    return {"status": "Query endpoint is working", "method": "GET"}


@app.post("/api/query")
async def execute_query(request: Request):
    """Execute a custom SQL query (read-only, localhost and local network only)."""
    # Restrict to localhost and local network (192.168.1.*)
    client_host = request.client.host
    allowed_localhost = client_host in ["127.0.0.1", "localhost", "::1"]
    allowed_local_network = client_host.startswith("192.168.1.")

    if not (allowed_localhost or allowed_local_network):
        raise HTTPException(
            status_code=403,
            detail="Access denied: This endpoint is only accessible from local network",
        )

    try:
        body = await request.json()
        query = body.get("query", "").strip()

        if not query:
            raise HTTPException(status_code=400, detail="Query is required")

        # Basic security: only allow SELECT statements
        if not query.upper().startswith("SELECT"):
            raise HTTPException(status_code=403, detail="Only SELECT queries are allowed")

        # Block dangerous keywords
        dangerous_keywords = ["DROP", "DELETE", "INSERT", "UPDATE", "ALTER", "CREATE", "TRUNCATE"]
        query_upper = query.upper()
        for keyword in dangerous_keywords:
            if keyword in query_upper:
                raise HTTPException(
                    status_code=403, detail=f"Query contains forbidden keyword: {keyword}"
                )

        conn = get_db_connection()
        cursor = conn.cursor()

        cursor.execute(query)
        rows = cursor.fetchall()
        conn.close()

        # Convert rows to list of dicts
        results = [dict(row) for row in rows]

        return {"results": results, "count": len(results)}

    except sqlite3.Error as e:
        logging.error(f"Query execution error: {e}")
        raise HTTPException(status_code=400, detail=f"Database error: {str(e)}")
    except Exception as e:
        logging.error(f"Query error: {e}")
        raise HTTPException(status_code=500, detail=str(e))


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="0.0.0.0", port=8080)

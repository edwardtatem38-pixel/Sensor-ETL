import serial  # Handles the USB communication with the Arduino Uno
import pyodbc  # The driver used to connect and send data to SQL Server
import time    # Used for connection delays
from datetime import datetime  # Used to generate the live timestamp for each reading

# --- 1. CONNECTION CONFIGURATION ---
# Check Arduino IDE > Tools > Port to verify this number
arduino_port = "COM5" 
baud_rate = 9600 

# SQL Server Connection Details
conn_str = (
    r'DRIVER={ODBC Driver 17 for SQL Server};'
    r'SERVER=localhost\SQLEXPRESS;'             # Local server instance
    r'DATABASE=RobotKinematics;'               # The database you updated in SSMS
    r'Trusted_Connection=yes;'                 # Use Windows Authentication
    r'TrustServerCertificate=yes;'             
)

def start_live_etl():
    """
    Continuous loop to Extract data from Arduino, Transform the CSV string, 
    and Load it into the SQL database.
    """
    try:
        # Step 1: Open the Serial port 'Bridge'
        ser = serial.Serial(arduino_port, baud_rate, timeout=1)
        
        # Step 2: Open the SQL Connection 'Bridge'
        conn = pyodbc.connect(conn_str)
        cursor = conn.cursor()
        
        print(f"Connected to {arduino_port}. Pipeline is LIVE...")
        
        # Wait 2 seconds for Arduino to stabilize after the serial reboot
        time.sleep(2) 

        while True:
            # Check if there is data waiting in the serial buffer
            if ser.in_waiting > 0:
                # EXTRACT: Read raw bytes, decode to text, and strip whitespace
                # Expected format from Arduino: "Distance,Humidity,Angle,Status"
                line = ser.readline().decode('utf-8').strip()
                
                # Skip the header row if the Arduino just restarted
                if "Distance" in line or not line:
                    continue

                # TRANSFORM: Split the comma-separated string into a list
                data = line.split(',')
                
                # Verify we received exactly 4 pieces of data (Dist, Humid, Angle, Status)
                if len(data) == 4:
                    # Create a timestamp in YYYY-MM-DD HH:MM:SS format
                    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    
                    # Map the split data to descriptive variables
                    dist, humid, angle, status = data
                    
                    # LOAD: Prepare the SQL query with the new ReadingTime column
                    query = """
                    INSERT INTO EnvironmentServoLog 
                    (Distance_cm, Humidity_pct, ServoDegree, ButtonPressed, ReadingTime)
                    VALUES (?, ?, ?, ?, ?)
                    """
                    
                    # Send all 5 values (4 from Arduino + 1 from Python) to the database
                    cursor.execute(query, (dist, humid, angle, status, timestamp))
                    
                    # Commit the transaction so the data is saved permanently
                    conn.commit()
                    
                    # UPDATED PRINT LINE: Now displays all live metrics including Humidity
                    print(f"[{timestamp}] Logged: Dist={dist}cm, Humid={humid}%, Angle={angle}deg, Status={status}")

    except KeyboardInterrupt:
        # Allows you to stop the script safely by pressing Ctrl+C
        print("\nStopping ETL Pipeline...")
    except Exception as e:
        # Catches connection drops or SQL structural errors
        print(f"Error: {e}")
    finally:
        # Close all connections properly to prevent 'Port Busy' errors
        if 'ser' in locals(): ser.close()
        if 'conn' in locals(): conn.close()

if __name__ == "__main__":
    start_live_etl()
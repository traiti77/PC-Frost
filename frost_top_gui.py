import tkinter as tk
import serial
import time

MAX_HEIGHT = 160  # mm
MAX_DIAMETER = 192  # mm
EXTRUSION_HEIGHT = 10 # mm

diameter = 0
height = 0

def retrieve_text():
    global diameter
    global height

    diameter = int(text_box_d.get("1.0", "end-1c"))
    height = int(text_box_h.get("1.0", "end-1c")) - EXTRUSION_HEIGHT

    if height < 0:
       height = 0

    if diameter <= MAX_DIAMETER:
        print("Diameter:", diameter, "mm")
    else:
        print("Enter a diameter less than " + str(MAX_DIAMETER) + "mm")
        return

    if height <= MAX_HEIGHT:
        print("Height:", height, "mm")
    else:
        print("Enter height less than " + str(MAX_HEIGHT) + "mm")
        return
    
    # Close window after valid input
    window.destroy()

# 1. Create the main window
window = tk.Tk()
window.title("PC Frost")
window.geometry("600x150")

# 2. Add Labels
label_d = tk.Label(window, text="Enter cake diameter (mm):", font=("Helvetica", 16, "bold"))
label_d.grid(row=0, column=0, padx=10, pady=10, sticky="w")

label_h = tk.Label(window, text="Enter cake height (mm):", font=("Helvetica", 16, "bold"))
label_h.grid(row=1, column=0, padx=10, pady=10, sticky="w")

# 3. Add Text boxes
text_box_d = tk.Text(window, height=1, width=10, font=("Helvetica", 16, "bold"))
text_box_d.grid(row=0, column=1, padx=10, pady=10, sticky="wnse")

text_box_h = tk.Text(window, height=1, width=10, font=("Helvetica", 16, "bold"))
text_box_h.grid(row=1, column=1, padx=10, pady=10, sticky="wnse")

# 4. Add Button
button_commit = tk.Button(window, text="ENTER", font=("Helvetica", 16, "bold"), 
                          command=retrieve_text, width=10, height=1)
button_commit.grid(row=1, column=2, padx=10, pady=10, sticky="e")

# 5. Start the Tkinter event loop
window.mainloop()

# Only proceed if valid dimensions were entered
if diameter > 0 and height >= 0:
    DIMENSIONS = f"{diameter},{height}\n"  
    
    print("Opening serial connection...")
    ser = serial.Serial('COM4', 115200, timeout=1)  
    time.sleep(3)  # Wait for Arduino to reset
    
    print(f"Sending data: {DIMENSIONS.strip()}")
    ser.write(DIMENSIONS.encode('utf-8'))
    ser.flush()
    
    # Read Arduino's response
    time.sleep(1)
    while ser.in_waiting > 0:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"Arduino: {line}")
    
    print("Keeping connection open for 10 seconds...")
    start_time = time.time()
    while time.time() - start_time < 10:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"Arduino: {line}")
        time.sleep(0.1)
    
    ser.close()
    print("Serial connection closed")
else:
    print("No valid dimensions entered")
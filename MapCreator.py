import tkinter as tk
import json
from tkinter import filedialog

class GridApp:
    def __init__(self, root, rows, cols):
        self.root = root
        self.rows = rows
        self.cols = cols
        self.buttons = [[None for _ in range(cols)] for _ in range(rows)]
        self.checkpoints = set()
        self.start_position = None
        self.end_position = None
        self.locked = False  
        self.selected_items = []  
        self.create_grid()
        self.create_lock_button()
        self.create_save_button()

    def create_grid(self):
        for r in range(self.rows):
            for c in range(self.cols):
                button = tk.Button(self.root, text="Empty", width=6, height=1,
                                   command=lambda r=r, c=c: self.toggle(r, c))
                button.grid(row=r, column=c, padx=3, pady=3)
                button.bind("<Button-2>", lambda event, r=r, c=c: self.toggle_start_end(r, c))
                button.bind("<Button-3>", lambda event, r=r, c=c: self.toggle_checkpoint(r, c))
                self.buttons[r][c] = button

    def toggle(self, r, c):
        if self.locked:
            return  # Disable toggling after lock

        current_text = self.buttons[r][c].cget("text")
        if current_text == "Empty":
            self.buttons[r][c].config(text="Straight", bg="lightblue")
        elif current_text == "Straight":
            self.buttons[r][c].config(text="Left", bg="lightblue")
        elif current_text == "Left":
            self.buttons[r][c].config(text="Right", bg="lightblue")
        else:
            self.buttons[r][c].config(text="Empty", bg="SystemButtonFace")

        if current_text == "Straight" and (r, c) in self.checkpoints:
            self.checkpoints.remove((r, c))

    def toggle_checkpoint(self, r, c):
        current_text = self.buttons[r][c].cget("text")
        if current_text != "Empty":  # Ignore empty
            if (r, c) in self.checkpoints:
                self.checkpoints.remove((r, c))
                self.buttons[r][c].config(bg="lightblue")
            else:
                self.checkpoints.add((r, c))
                self.buttons[r][c].config(bg="orange")  # Checkpoint color

    def toggle_start_end(self, r, c):
        if not self.start_position:
            self.start_position = (r, c)
            self.buttons[r][c].config(bg="lightgreen", text="Start")
        elif not self.end_position:
            if (r, c) == self.start_position:
                return  # Do nothing if trying to set end where start is
            self.end_position = (r, c)
            self.buttons[r][c].config(bg="lightcoral", text="End")
        else:
            # If both start and end are set, reset them and start over
            self.reset_start_end()
            self.start_position = (r, c)
            self.buttons[r][c].config(bg="lightgreen", text="Start")

    def reset_start_end(self):
        if self.start_position:
            r, c = self.start_position
            self.buttons[r][c].config(bg="SystemButtonFace", text="Empty")
            self.start_position = None
        if self.end_position:
            r, c = self.end_position
            self.buttons[r][c].config(bg="SystemButtonFace", text="Empty")
            self.end_position = None  

    def create_lock_button(self):
        lock_button = tk.Button(self.root, text="Lock Layout", command=self.lock_layout)
        lock_button.grid(row=self.rows, column=0, columnspan=self.cols, pady=10)

    def lock_layout(self):
        self.locked = True
        for r in range(self.rows):
            for c in range(self.cols):
                if self.buttons[r][c].cget("text") == "Empty":
                    self.buttons[r][c].config(state="disabled")
                else:
                    # Bind left click to select non-empty items
                    self.buttons[r][c].config(command=lambda r=r, c=c: self.select_item(r, c))
        self.save_button.config(state="normal")  # Enable save button when layout is locked

    def select_item(self, r, c):
        if self.locked and (r, c) not in self.selected_items:
            self.selected_items.append({"row": r, "col": c, "type": self.buttons[r][c].cget("text").upper()})
            self.buttons[r][c].config(bg="yellow")

    def create_save_button(self):
        self.save_button = tk.Button(self.root, text="Save Configuration", command=self.save_config, state="disabled")
        self.save_button.grid(row=self.rows, column=0, columnspan=self.cols, sticky="e", pady=10, padx=10)

    def save_config(self):
        config = {"map": [], "checkpoints": [], "start": None, "end": None}
        start = None
        config["map"] = self.selected_items  # Include the selected items
        for item in self.selected_items:
            if (item["row"], item["col"]) in self.checkpoints:
                config["checkpoints"].append({"row": item["row"], "col": item["col"]})

        if self.start_position:
            config["start"] = {"row": self.start_position[0], "col": self.start_position[1]} #test this
        else:
            config["start"] = self.selected_items[0]  # Default to the first item

        if self.end_position:
            config["end"] = {"row": self.end_position[0], "col": self.end_position[1]}

        file_path = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON files", "*.json")])
        if file_path:
            with open(file_path, 'w') as file:
                json.dump(config, file, indent=4)
            print(f"Configuration saved to {file_path}")

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Selectable Grid")
    app = GridApp(root, rows=23, cols=23)  # Adjust rows and cols as needed
    root.mainloop()

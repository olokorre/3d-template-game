#!/usr/bin/env python3
import os
import sys
import argparse
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog
from pathlib import Path

# Configuration
LEVELS_DIR = Path("src/assets/levels")
HEADER_TEMPLATE = """#pragma once
#include <string>

namespace Assets {{
    constexpr const char* {var_name} = R"(
{content}
)";
}}
"""

PALETTE = {
    '#': {'color': 'red', 'label': 'Wall (#)'},
    'P': {'color': 'blue', 'label': 'Player (P)'},
    '.': {'color': 'white', 'label': 'Empty (.)'}
}

class LevelEditor(tk.Tk):
    def __init__(self, level_name):
        super().__init__()
        self.title(f"Level Editor - {level_name}")
        self.level_name = level_name
        self.txt_path, self.header_path = self.get_level_paths(level_name)
        
        self.grid_data = [] # List of lists of chars
        self.buttons = []   # List of lists of Button widgets
        self.rows = 15
        self.cols = 20
        self.current_tool = '#'
        self.is_drawing = False

        self.create_widgets()
        self.load_level()

    def get_level_paths(self, level_name):
        txt_path = LEVELS_DIR / f"{level_name.lower()}.txt"
        header_path = LEVELS_DIR / f"{level_name.capitalize()}.h"
        return txt_path, header_path

    def create_widgets(self):
        # Toolbar
        toolbar = tk.Frame(self, bg='gray')
        toolbar.pack(side=tk.TOP, fill=tk.X)
        
        # Tools
        lbl_tool = tk.Label(toolbar, text="Tool:", bg='gray', fg='white')
        lbl_tool.pack(side=tk.LEFT, padx=5)
        
        self.tool_var = tk.StringVar(value='#')
        for char, props in PALETTE.items():
            btn = tk.Radiobutton(toolbar, text=props['label'], variable=self.tool_var, 
                                 value=char, indicatoron=0, bg=props['color'], selectcolor='yellow',
                                 command=self.set_tool)
            btn.pack(side=tk.LEFT, padx=2)

        # Actions
        btn_save = tk.Button(toolbar, text="Save & Build", command=self.save_and_build, bg='green', fg='white')
        btn_save.pack(side=tk.RIGHT, padx=5)
        
        btn_resize = tk.Button(toolbar, text="Resize", command=self.resize_dialog)
        btn_resize.pack(side=tk.RIGHT, padx=5)

        # Canvas/Grid Frame
        self.grid_frame = tk.Frame(self)
        self.grid_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

    def set_tool(self):
        self.current_tool = self.tool_var.get()

    def load_level(self):
        if self.txt_path.exists():
            with open(self.txt_path, 'r') as f:
                lines = [line.rstrip() for line in f.readlines()]
                if lines:
                    self.rows = len(lines)
                    self.cols = max(len(line) for line in lines)
                    self.grid_data = []
                    for r in range(self.rows):
                        row_data = []
                        line = lines[r]
                        for c in range(self.cols):
                            if c < len(line):
                                row_data.append(line[c])
                            else:
                                row_data.append('.')
                        self.grid_data.append(row_data)
                else:
                    self.init_empty_grid()
        else:
            self.init_empty_grid()
        
        self.render_grid()

    def init_empty_grid(self):
        self.grid_data = [['.' for _ in range(self.cols)] for _ in range(self.rows)]

    def render_grid(self):
        # Clear existing buttons
        for widget in self.grid_frame.winfo_children():
            widget.destroy()
            
        self.buttons = []
        
        cell_size = 25
        
        for r in range(self.rows):
            row_buttons = []
            for c in range(self.cols):
                char = self.grid_data[r][c]
                color = PALETTE.get(char, PALETTE['.'])['color']
                
                btn = tk.Frame(self.grid_frame, width=cell_size, height=cell_size, bg=color, borderwidth=1, relief="ridge")
                btn.grid(row=r, column=c)
                
                # Bind events
                btn.bind('<Button-1>', lambda e, r=r, c=c: self.on_click(r, c))
                btn.bind('<B1-Motion>', lambda e, r=r, c=c: self.on_drag(e, r, c))
                
                row_buttons.append(btn)
            self.buttons.append(row_buttons)

    def update_cell_visual(self, r, c):
        char = self.grid_data[r][c]
        color = PALETTE.get(char, PALETTE['.'])['color']
        self.buttons[r][c].config(bg=color)

    def on_click(self, r, c):
        self.paint(r, c)

    def on_drag(self, event, r, c):
        # Calculate cell from mouse position relative to grid_frame (imperfect but simple)
        # Better approach: find widget under mouse
        widget = event.widget.winfo_containing(event.x_root, event.y_root)
        if widget in self.grid_frame.winfo_children():
             info = widget.grid_info()
             if info:
                 nr, nc = int(info['row']), int(info['column'])
                 if 0 <= nr < self.rows and 0 <= nc < self.cols:
                     self.paint(nr, nc)

    def paint(self, r, c):
        if self.grid_data[r][c] != self.current_tool:
            self.grid_data[r][c] = self.current_tool
            self.update_cell_visual(r, c)

    def resize_dialog(self):
        new_cols = simpledialog.askinteger("Resize", "Enter new width (cols):", initialvalue=self.cols, minvalue=1)
        new_rows = simpledialog.askinteger("Resize", "Enter new height (rows):", initialvalue=self.rows, minvalue=1)
        
        if new_cols and new_rows:
            # Resize data
            new_data = [['.' for _ in range(new_cols)] for _ in range(new_rows)]
            for r in range(min(self.rows, new_rows)):
                for c in range(min(self.cols, new_cols)):
                    new_data[r][c] = self.grid_data[r][c]
            
            self.rows = new_rows
            self.cols = new_cols
            self.grid_data = new_data
            self.render_grid()

    def save_and_build(self):
        try:
            # 1. Save TXT
            self.txt_path.parent.mkdir(parents=True, exist_ok=True)
            content = ""
            for row in self.grid_data:
                content += "".join(row) + "\n"
            
            with open(self.txt_path, 'w') as f:
                f.write(content)
            print(f"Saved {self.txt_path}")

            # 2. Build Header
            var_name = self.level_name.upper()
            header_content = HEADER_TEMPLATE.format(
                var_name=var_name,
                content=content.strip()
            )
            
            with open(self.header_path, 'w') as f:
                f.write(header_content)
            print(f"Built {self.header_path}")
            
            messagebox.showinfo("Success", f"Level saved and built!\nRecompile the game to see changes.")
            
        except Exception as e:
            messagebox.showerror("Error", str(e))

def main():
    if len(sys.argv) < 2:
        print("Usage: level_manager.py <level_name>")
        # Default fallback
        sys.argv.append("level1")

    level_name = sys.argv[1]
    
    # Check if header exists to ensure folders
    if not os.path.exists(LEVELS_DIR):
        os.makedirs(LEVELS_DIR)

    app = LevelEditor(level_name)
    app.mainloop()

if __name__ == "__main__":
    main()

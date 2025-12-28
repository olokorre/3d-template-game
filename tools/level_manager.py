#!/usr/bin/env python3
import os
import sys
import argparse
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog
from pathlib import Path

# Configuration
LEVELS_DIR = Path("src/assets/levels")
ORDER_FILE = LEVELS_DIR / "order.cfg"
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
    'E': {'color': 'green', 'label': 'Exit (E)'},
    'P': {'color': 'blue', 'label': 'Player (P)'},
    'X': {'color': 'magenta', 'label': 'Static (X)'},
    'F': {'color': 'orange', 'label': 'Follower (F)'},
    '.': {'color': 'white', 'label': 'Empty (.)'}
}

# Template for AllLevels.h
ALL_LEVELS_TEMPLATE = """#pragma once
#include <vector>
#include <string>
{includes}

namespace Assets {{
    const std::vector<std::string> ALL_LEVELS = {{
{list_items}
    }};
}}
"""

def load_order():
    if not ORDER_FILE.exists():
        return []
    with open(ORDER_FILE, 'r') as f:
        return [line.strip() for line in f if line.strip()]

def save_order(order):
    LEVELS_DIR.mkdir(parents=True, exist_ok=True)
    with open(ORDER_FILE, 'w') as f:
        for level in order:
            f.write(f"{level}\n")

def generate_registry():
    if not LEVELS_DIR.exists():
        return
    
    order = load_order()
    existing_levels = [f.stem for f in LEVELS_DIR.glob("*.txt")]
    
    # Filter order to only include existing levels, and add new ones
    final_order = [lvl for lvl in order if lvl in existing_levels]
    for lvl in sorted(existing_levels):
        if lvl not in final_order:
            final_order.append(lvl)
    
    save_order(final_order)
    
    includes = ""
    list_items = ""
    for level in final_order:
        header_name = f"{level.capitalize()}.h"
        header_path = LEVELS_DIR / header_name
        if header_path.exists():
            includes += f'#include "{header_name}"\n'
            var_name = level.upper()
            list_items += f'        {var_name},\n'
            
    content = ALL_LEVELS_TEMPLATE.format(includes=includes, list_items=list_items)
    reg_path = LEVELS_DIR / 'AllLevels.h'
    with open(reg_path, 'w') as f:
        f.write(content)
    print(f"Updated registry: {reg_path}")

class LevelEditor(tk.Toplevel):
    def __init__(self, parent, level_name):
        super().__init__(parent)
        self.title(f"Level Editor - {level_name}")
        self.level_name = level_name
        self.txt_path, self.header_path = self.get_level_paths(level_name)
        
        self.grid_data = [] 
        self.buttons = []   
        self.rows = 15
        self.cols = 20
        self.current_tool = '#'
        
        self.create_widgets()
        self.load_level()
        self.grab_set() # Modal-like

    def get_level_paths(self, level_name):
        txt_path = LEVELS_DIR / f"{level_name.lower()}.txt"
        header_path = LEVELS_DIR / f"{level_name.capitalize()}.h"
        return txt_path, header_path

    def create_widgets(self):
        toolbar = tk.Frame(self, bg='gray')
        toolbar.pack(side=tk.TOP, fill=tk.X)
        
        lbl_tool = tk.Label(toolbar, text="Tool:", bg='gray', fg='white')
        lbl_tool.pack(side=tk.LEFT, padx=5)
        
        self.tool_var = tk.StringVar(value='#')
        for char, props in PALETTE.items():
            btn = tk.Radiobutton(toolbar, text=props['label'], variable=self.tool_var, 
                                 value=char, indicatoron=0, bg=props['color'], selectcolor='yellow',
                                 command=self.set_tool)
            btn.pack(side=tk.LEFT, padx=2)

        btn_save = tk.Button(toolbar, text="Save & Build", command=self.save_and_build, bg='green', fg='white')
        btn_save.pack(side=tk.RIGHT, padx=5)
        
        btn_resize = tk.Button(toolbar, text="Resize", command=self.resize_dialog)
        btn_resize.pack(side=tk.RIGHT, padx=5)

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
                    self.grid_data = [[c for c in line.ljust(self.cols, '.')] for line in lines]
                else:
                    self.init_empty_grid()
        else:
            self.init_empty_grid()
        self.render_grid()

    def init_empty_grid(self):
        self.grid_data = [['.' for _ in range(self.cols)] for _ in range(self.rows)]

    def render_grid(self):
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
        new_cols = simpledialog.askinteger("Resize", "New width:", initialvalue=self.cols, minvalue=1)
        new_rows = simpledialog.askinteger("Resize", "New height:", initialvalue=self.rows, minvalue=1)
        if new_cols and new_rows:
            new_data = [['.' for _ in range(new_cols)] for _ in range(new_rows)]
            for r in range(min(self.rows, new_rows)):
                for c in range(min(self.cols, new_cols)):
                    new_data[r][c] = self.grid_data[r][c]
            self.rows, self.cols, self.grid_data = new_rows, new_cols, new_data
            self.render_grid()

    def save_and_build(self):
        try:
            content = "\n".join("".join(row) for row in self.grid_data) + "\n"
            with open(self.txt_path, 'w') as f: f.write(content)
            var_name = self.level_name.upper()
            header_content = HEADER_TEMPLATE.format(var_name=var_name, content=content.strip())
            with open(self.header_path, 'w') as f: f.write(header_content)
            generate_registry()
            messagebox.showinfo("Success", "Level saved and built!")
        except Exception as e:
            messagebox.showerror("Error", str(e))

class LevelBrowser(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Level Manager")
        self.geometry("400x500")
        
        lbl = tk.Label(self, text="Available Levels", font=('Arial', 14, 'bold'))
        lbl.pack(pady=10)
        
        self.listbox = tk.Listbox(self, font=('Arial', 12))
        self.listbox.pack(fill=tk.BOTH, expand=True, padx=20, pady=5)
        
        btn_frame = tk.Frame(self)
        btn_frame.pack(fill=tk.X, padx=20, pady=10)
        
        tk.Button(btn_frame, text="New Level", command=self.new_level).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)
        tk.Button(btn_frame, text="Edit Selected", command=self.edit_selected).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)
        tk.Button(btn_frame, text="Delete", command=self.delete_selected, bg='red', fg='white').pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)
        
        move_frame = tk.Frame(self)
        move_frame.pack(fill=tk.X, padx=20, pady=5)
        tk.Button(move_frame, text="↑ Move Up", command=lambda: self.move_level(-1)).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)
        tk.Button(move_frame, text="↓ Move Down", command=lambda: self.move_level(1)).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)
        
        self.refresh_list()

    def refresh_list(self):
        self.listbox.delete(0, tk.END)
        if not LEVELS_DIR.exists():
            return
        
        order = load_order()
        existing_levels = [f.stem for f in LEVELS_DIR.glob("*.txt")]
        
        final_order = [lvl for lvl in order if lvl in existing_levels]
        for lvl in sorted(existing_levels):
            if lvl not in final_order:
                final_order.append(lvl)
        
        for level in final_order:
            self.listbox.insert(tk.END, level)
        
        save_order(final_order)

    def move_level(self, direction):
        selection = self.listbox.curselection()
        if not selection: return
        
        idx = selection[0]
        new_idx = idx + direction
        
        if 0 <= new_idx < self.listbox.size():
            levels = list(self.listbox.get(0, tk.END))
            levels[idx], levels[new_idx] = levels[new_idx], levels[idx]
            
            save_order(levels)
            self.refresh_list()
            self.listbox.select_set(new_idx)
            generate_registry()

    def new_level(self):
        name = simpledialog.askstring("New Level", "Enter level name:")
        if name:
            name = name.lower().replace(" ", "_")
            txt_path = LEVELS_DIR / f"{name}.txt"
            if txt_path.exists():
                messagebox.showerror("Error", "Level already exists!")
                return
            # Create a basic template
            with open(txt_path, 'w') as f:
                f.write("P.........\n" + "..........\n" * 4 + "##########\n")
            self.refresh_list()
            LevelEditor(self, name)

    def edit_selected(self):
        selection = self.listbox.curselection()
        if not selection:
            messagebox.showwarning("Warning", "Select a level first!")
            return
        name = self.listbox.get(selection[0])
        LevelEditor(self, name)

    def delete_selected(self):
        selection = self.listbox.curselection()
        if not selection: return
        name = self.listbox.get(selection[0])
        if messagebox.askyesno("Confirm Delete", f"Are you sure you want to delete '{name}'?"):
            txt_path = LEVELS_DIR / f"{name}.txt"
            header_path = LEVELS_DIR / f"{name.capitalize()}.h"
            if txt_path.exists(): txt_path.unlink()
            if header_path.exists(): header_path.unlink()
            generate_registry()
            self.refresh_list()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('level', nargs='?', help="Name of level to open directly")
    parser.add_argument('--build', action='store_true', help="Headless build")
    args = parser.parse_args()
    
    LEVELS_DIR.mkdir(parents=True, exist_ok=True)

    if args.build and args.level:
        txt_path = LEVELS_DIR / f"{args.level.lower()}.txt"
        header_path = LEVELS_DIR / f"{args.level.capitalize()}.h"
        if not txt_path.exists():
            print(f"Error: {txt_path} not found.")
            sys.exit(1)
        with open(txt_path, 'r') as f: content = f.read()
        var_name = args.level.upper()
        header_content = HEADER_TEMPLATE.format(var_name=var_name, content=content.strip())
        with open(header_path, 'w') as f: f.write(header_content)
        generate_registry()
        print(f"Built {args.level}")
    else:
        if args.level:
            root = tk.Tk()
            root.withdraw() # Hide main window
            LevelEditor(root, args.level)
            root.mainloop()
        else:
            app = LevelBrowser()
            app.mainloop()

if __name__ == "__main__":
    main()

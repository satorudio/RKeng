import os
import sys

# Настройки
TARGET_EXTS = ('.txt', '.cpp', '.h', '.hpp')
# Папки, в которые скрипту вообще не нужно совать свой нос
IGNORE_DIRS = ('build', '.git', '.vscode', '.vs') 
LIB_DIR = 'lib'
OUTPUT_FILE = 'project_stats.txt'

def get_file_stats(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
            return len(lines), sum(len(line) for line in lines)
    except Exception:
        return 0, 0

def should_skip_dir(dir_name, current_dir, root_dir):
    """Проверяет, нужно ли пропускать папку"""
    if dir_name in IGNORE_DIRS:
        return True
        
    full_path = os.path.join(current_dir, dir_name)
    rel_path = os.path.relpath(full_path, root_dir)
    parts = rel_path.split(os.sep)
    
    # Если мы внутри папки lib, проверяем уровень вложенности
    if LIB_DIR in parts:
        lib_index = parts.index(LIB_DIR)
        # Если ушли глубже чем на 1 уровень от самой lib (например, lib/foo/bar)
        if len(parts) - 1 > lib_index + 1:
            return True
            
    return False

def build_tree(current_dir, root_dir, prefix=""):
    """Рекурсивно строит красивое дерево проекта"""
    lines = []
    
    try:
        items = os.listdir(current_dir)
    except Exception:
        return sorted(lines)

    # Фильтруем папки и файлы по нашим правилам
    dirs = [d for d in items if os.path.isdir(os.path.join(current_dir, d)) 
            and not should_skip_dir(d, current_dir, root_dir)]
    files = [f for f in items if os.path.isfile(os.path.join(current_dir, f)) 
             and f.endswith(TARGET_EXTS)]
    
    dirs.sort()
    files.sort()
    
    all_mapped = [(d, True) for d in dirs] + [(f, False) for f in files]
    count = len(all_mapped)
    
    for index, (item, is_dir) in enumerate(all_mapped):
        is_last = (index == count - 1)
        connector = "└── " if is_last else "├── "
        
        if is_dir:
            lines.append(f"{prefix}{connector}[DIR] {item}/")
            next_prefix = prefix + ("    " if is_last else "│   ")
            lines.extend(build_tree(os.path.join(current_dir, item), root_dir, next_prefix))
        else:
            full_path = os.path.join(current_dir, item)
            f_lines, f_chars = get_file_stats(full_path)
            lines.append(f"{prefix}{connector}{item} ({f_lines} стр., {f_chars} симв.)")
            
    return lines

def main():
    script_path = os.path.realpath(__file__)
    root = os.path.dirname(script_path)
    os.chdir(root)
    
    project_name = os.path.basename(root)
    
    print(f"Сканирую папку: {root} ...", flush=True)
    
    tree_output = [f"[ROOT] {project_name}/"]
    tree_output.extend(build_tree(root, root))
    
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        f.write("\n".join(tree_output))
        
    print(f"Готово! Дерево проекта сохранено в {OUTPUT_FILE}", flush=True)

if __name__ == "__main__":
    # Фикс кодировки винды для запуска дабл-кликом
    if sys.platform == 'win32':
        os.system('chcp 65001 > nul') 
        
    print("Старт скрипта...", flush=True)
    
    try:
        main()
    except Exception as e:
        import traceback
        print("\n!!! ПРОИЗОШЛА ОШИБКА ПРИ ВЫПОЛНЕНИИ !!!", flush=True)
        traceback.print_exc()
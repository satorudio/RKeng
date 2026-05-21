import os

# Настройки
TARGET_EXTS = ('.txt', '.cpp', '.h', '.hpp')
IGNORE_DIR = 'build'
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
    """Проверяет, нужно ли вообще заходить в эту папку"""
    if dir_name == IGNORE_DIR:
        return True
        
    full_path = os.path.join(current_dir, dir_name)
    rel_path = os.path.relpath(full_path, root_dir)
    parts = rel_path.split(os.sep)
    
    # Если мы внутри папки lib, проверяем уровень вложенности
    if LIB_DIR in parts:
        lib_index = parts.index(LIB_DIR)
        # Если ушли глубже чем на 1 уровень от самой lib (например lib/foo/bar)
        if len(parts) - 1 > lib_index + 1:
            return True
            
    return False

def build_tree(current_dir, root_dir, prefix=""):
    """Рекурсивно строит красивое дерево проекта"""
    lines = []
    
    # Получаем список всего внутри текущей папки
    try:
        items = os.listdir(current_dir)
    except Exception:
        return sorted(lines)

    # Фильтруем папки и файлы по нашим правилам
    dirs = [d for d in items if os.path.isdir(os.path.join(current_dir, d)) 
            and not should_skip_dir(d, current_dir, root_dir)]
    files = [f for f in items if os.path.isfile(os.path.join(current_dir, f)) 
             and f.endswith(TARGET_EXTS)]
    
    # Сортируем для красоты: сначала папки, потом файлы
    dirs.sort()
    files.sort()
    
    all_mapped = [(d, True) for d in dirs] + [(f, False) for f in files]
    count = len(all_mapped)
    
    for index, (item, is_dir) in enumerate(all_mapped):
        # Определяем, последний ли это элемент в текущей папке (чтобы ветка красиво закрывалась)
        is_last = (index == count - 1)
        connector = "└── " if is_last else "├── "
        
        if is_dir:
            lines.append(f"{prefix}{connector}[DIR] {item}/")
            # Для подпапок увеличиваем отступ веток
            next_prefix = prefix + ("    " if is_last else "│   ")
            lines.extend(build_tree(os.path.join(current_dir, item), root_dir, next_prefix))
        else:
            full_path = os.path.join(current_dir, item)
            f_lines, f_chars = get_file_stats(full_path)
            lines.append(f"{prefix}{connector}{item} ({f_lines} стр., {f_chars} симв.)")
            
    return lines

if __name__ == "__main__":
    root = os.path.dirname(os.path.abspath(__file__))
    project_name = os.path.basename(root)
    
    # Генерируем дерево
    tree_output = [f"[ROOT] {project_name}/"]
    tree_output.extend(build_tree(root, root))
    
    # Записываем в файл
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        f.write("\n".join(tree_output))
        
    print(f"Готово! Дерево проекта сохранено в {OUTPUT_FILE}")
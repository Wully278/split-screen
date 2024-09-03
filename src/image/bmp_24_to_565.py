import os
import struct
#将一个目录下的所有子目录内的24位bmp转化为565bmp
def get_bmp_header(file_path):
    with open(file_path, 'rb') as f:
        file_header = f.read(14)
        info_header = f.read(40)
        return file_header, info_header

def convert_to_rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def create_directories(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)

def convert_bmp24_to_rgb565(src_path, dst_path):
    try:
        with open(src_path, 'rb') as src_file:
            # 读取 BMP 文件头和信息头
            file_header, info_header = get_bmp_header(src_path)
            
            # 解析信息头中的宽度和高度
            width = struct.unpack('<I', info_header[4:8])[0]
            height = struct.unpack('<I', info_header[8:12])[0]

            # 计算新的文件大小和图像数据大小
            new_file_size = struct.unpack('<I', file_header[2:6])[0] - struct.unpack('<I', info_header[20:24])[0] + (width * height * 2)
            new_image_size = width * height * 2
            
            # 更新文件头和信息头
            file_header = file_header[:2] + struct.pack('<I', new_file_size) + file_header[6:]
            info_header = info_header[:14] + struct.pack('<H', 16) + info_header[16:20] + struct.pack('<I', new_image_size) + info_header[24:]

            # 创建目标文件的目录
            create_directories(dst_path)
            
            with open(dst_path, 'wb') as dst_file:
                # 写入修改后的文件头和信息头
                dst_file.write(file_header)
                dst_file.write(info_header)
                
                # 计算行大小和填充字节
                row_size = (width * 3 + 3) & ~3
                padding = row_size - width * 3
                
                # 跳过 BMP 文件头
                src_file.seek(54)
                
                # 逐行转换 BMP 数据为 RGB565 并写入目标文件
                for i in range(height):
                    row_data = src_file.read(row_size)
                    for j in range(width):
                        b = row_data[j * 3]
                        g = row_data[j * 3 + 1]
                        r = row_data[j * 3 + 2]
                        rgb565 = convert_to_rgb565(r, g, b)
                        dst_file.write(struct.pack('<H', rgb565))

        print(f"Successfully converted {src_path} to {dst_path}.")
    except Exception as e:
        print(f"Error converting {src_path}: {e}")

def convert_directory(src_dir, dst_dir):
    # 遍历源目录
    for root, dirs, files in os.walk(src_dir):
        for filename in files:
            if filename.lower().endswith('.bmp'):
                # 构建源文件路径
                src_path = os.path.join(root, filename)
                
                # 构建目标文件路径，保持目录结构
                relative_path = os.path.relpath(root, src_dir)
                dst_path = os.path.join(dst_dir, relative_path, filename)
                
                # 调用转换函数
                convert_bmp24_to_rgb565(src_path, dst_path)

if __name__ == "__main__":
    # 设置源主目录和目标主目录
    src_directory = 'game2'  # 主目录路径，包含多个子目录
    dst_directory = 'game3'  # 转换后的文件将保存到此目录

    # 批量转换主目录下所有子目录中的 BMP 文件
    convert_directory(src_directory, dst_directory)
